// SPDX-License-Identifier: MIT
#pragma once
#include <cerrno>
#include <filesystem>
#include <fcntl.h>
#include <linux/fs.h>
#include <optional>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace uke::motion {
// Production supplies a fixed /run path and uid0. Alternate directories/uids
// are C++ test dependencies only, never bus arguments or environment options.
class Journal {
public:
    enum class Status {missing,valid,invalid};
    struct Data {Status status;std::string text;};
    explicit Journal(std::filesystem::path path,uid_t owner=0):m_path(std::move(path)),m_owner(owner){}
    bool discard_pending()const {
        Fd directory(open_directory());if(directory.value<0)return false;
        const auto name=m_path.filename().string()+".new";
        struct stat st{};
        if(::fstatat(directory.value,name.c_str(),&st,AT_SYMLINK_NOFOLLOW)<0)return errno==ENOENT;
        return trusted(st)&&::unlinkat(directory.value,name.c_str(),0)==0;
    }
    Data load()const {
        Fd directory(open_directory());if(directory.value<0)return {Status::invalid,{}};
        Fd file(::openat(directory.value,m_path.filename().c_str(),O_RDONLY|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC));
        if(file.value<0)return {errno==ENOENT?Status::missing:Status::invalid,{}};
        struct stat st{};if(::fstat(file.value,&st)<0||!trusted(st))return {Status::invalid,{}};
        std::string text(16385,'\0');const auto count=::read(file.value,text.data(),text.size());
        if(count<0||count>16384||count!=st.st_size)return {Status::invalid,{}};
        text.resize(std::size_t(count));return {Status::valid,std::move(text)};
    }
    bool save(std::string_view text)const {
        if(text.empty()||text.size()>16384)return false;
        Fd directory(open_directory());if(directory.value<0)return false;
        const auto name=m_path.filename().string(),temporary=name+".new";
        Fd file(::openat(directory.value,temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));
        if(file.value<0)return false;
        const bool written=::write(file.value,text.data(),text.size())==ssize_t(text.size())&&::fsync(file.value)==0;
        const bool renamed=written&&::syscall(SYS_renameat2,directory.value,temporary.c_str(),directory.value,name.c_str(),RENAME_NOREPLACE)==0;
        ::unlinkat(directory.value,temporary.c_str(),0);
        return renamed&&::fsync(directory.value)==0;
    }
    bool clear()const {
        Fd directory(open_directory());if(directory.value<0)return false;
        struct stat st{};const auto name=m_path.filename().string();
        if(::fstatat(directory.value,name.c_str(),&st,AT_SYMLINK_NOFOLLOW)<0)return errno==ENOENT;
        return trusted(st)&&::unlinkat(directory.value,name.c_str(),0)==0&&::fsync(directory.value)==0;
    }
private:
    struct Fd {
        int value;
        explicit Fd(int fd):value(fd){}
        ~Fd(){if(value>=0)::close(value);}
        Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;
    };
    std::filesystem::path m_path;
    uid_t m_owner;
    bool trusted(const struct stat& st)const {
        return S_ISREG(st.st_mode)&&st.st_uid==m_owner&&(st.st_mode&0777)==0600&&st.st_nlink==1&&st.st_size>=0&&st.st_size<=16384;
    }
    int open_directory()const {
        const int fd=::open(m_path.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
        if(fd<0)return -1;
        struct stat st{};
        if(::fstat(fd,&st)<0||st.st_uid!=m_owner||(st.st_mode&0777)!=0700){::close(fd);return -1;}
        return fd;
    }
};
}
