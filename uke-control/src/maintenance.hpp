// SPDX-License-Identifier: MIT
#pragma once
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace uke {
inline void require_staging_budget(const std::filesystem::path& runtime="/run",const std::filesystem::path& meminfo="/proc/meminfo") {
    std::ifstream input(meminfo);std::string line;uint64_t available_kib=0;unsigned found=0;
    while(std::getline(input,line))if(line.starts_with("MemAvailable:")) {
        std::istringstream fields(line);std::string key,number,unit,extra;
        if(!(fields>>key>>number>>unit)||unit!="kB"||(fields>>extra)||number.empty()||number.find_first_not_of("0123456789")!=std::string::npos)
            throw std::runtime_error("Invalid RAM staging budget");
        available_kib=std::stoull(number);++found;
    }
    if(input.bad()||found!=1||available_kib<512*1024||std::filesystem::space(runtime).available<512ULL*1024*1024)
        throw std::runtime_error("Offline staging requires at least 512 MiB available RAM and tmpfs space");
}
inline void publish_noreplace(const std::filesystem::path& from,const std::filesystem::path& to) {
    if(renameat2(AT_FDCWD,from.c_str(),AT_FDCWD,to.c_str(),RENAME_NOREPLACE))
        throw std::runtime_error("Maintenance publication refused; an existing destination is preserved");
}
class Maintenance {
    int fd_=-1;
public:
    explicit Maintenance(const char* path="/run/uke-control-maintenance.lock") {
        // Direct native tools retain the reservation if their controller dies.
        fd_=open(path,O_CREAT|O_RDWR|O_NOFOLLOW|O_NONBLOCK,0600);
        struct stat st{};
        if(fd_<0||fstat(fd_,&st)||!S_ISREG(st.st_mode)||st.st_uid!=0||st.st_nlink!=1||
           (st.st_mode&0777)!=0600||flock(fd_,LOCK_EX|LOCK_NB)) {
            if(fd_>=0)close(fd_);
            throw std::runtime_error("Maintenance is busy or its reservation is untrusted");
        }
    }
    Maintenance(const Maintenance&)=delete;
    Maintenance& operator=(const Maintenance&)=delete;
    ~Maintenance(){if(fd_>=0)close(fd_);}
};
struct Scratch {
    std::filesystem::path path;
    bool published=false;
    ~Scratch(){if(!published){std::error_code error;std::filesystem::remove_all(path,error);}}
};
}
