// SPDX-License-Identifier: MIT
#include "chroot.hpp"
#include "sysfs.hpp"
#include <algorithm>
#include <array>
#include <blkid/blkid.h>
#include <cerrno>
#include <charconv>
#include <csignal>
#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <grp.h>
#include <linux/capability.h>
#include <linux/fs.h>
#include <linux/openat2.h>
#include <openssl/sha.h>
#include <poll.h>
#include <pty.h>
#include <sched.h>
#include <sstream>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <sys/xattr.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
namespace uke::chroot_manager {
namespace fs=std::filesystem;
using uke::sysfs::quote;
namespace {
volatile sig_atomic_t interrupted=0,resized=0;
void signal_stop(int sig){interrupted=sig;}
void signal_resize(int){resized=1;}
void signals() {
    struct sigaction action{};action.sa_handler=signal_stop;sigemptyset(&action.sa_mask);
    for(int sig:{SIGTERM,SIGHUP,SIGINT})sigaction(sig,&action,nullptr);
    action.sa_handler=signal_resize;sigaction(SIGWINCH,&action,nullptr);
    action.sa_handler=SIG_IGN;sigaction(SIGPIPE,&action,nullptr);
}
void need(bool ok,const std::string& message){if(interrupted)throw std::runtime_error("Chroot operation interrupted");if(!ok)throw std::runtime_error(message);}
struct Fd {
    int n=-1;explicit Fd(int value=-1):n(value){}~Fd(){if(n>=0)::close(n);}
    Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;
};
std::string read_fd(int fd,size_t maximum=16384) {
    struct stat st{};need(fd>=0 && !fstat(fd,&st) && S_ISREG(st.st_mode),"Metadata is not a regular file");
    std::string out;std::array<char,4096> block{};ssize_t count;
    while((count=::read(fd,block.data(),block.size()))!=0) {
        if(count<0 && errno==EINTR)continue;
        need(count>0,"Metadata read failed");out.append(block.data(),count);
        need(out.size()<=maximum,"Metadata exceeds limit");
    }
    return out;
}
std::string file(const fs::path& path,size_t maximum=16384) {Fd fd(::open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW));return read_fd(fd.n,maximum);}
std::string trimmed(std::string text){while(!text.empty()&&(text.back()=='\n'||text.back()=='\r'))text.pop_back();return text;}
int inside(int root,const char* path,int flags,bool symlinks=false,bool optional=false) {
    open_how how{};how.flags=flags|O_CLOEXEC;how.resolve=RESOLVE_IN_ROOT|RESOLVE_NO_MAGICLINKS|RESOLVE_NO_XDEV;
    if(!symlinks)how.resolve|=RESOLVE_NO_SYMLINKS;
    int fd=int(syscall(SYS_openat2,root,path,&how,sizeof(how)));
    if(fd<0 && errno==ENOENT && optional)return -1;
    need(fd>=0,"Required target path is missing, unsafe or openat2 is unavailable: "+std::string(path));return fd;
}
std::string name(const std::string& device) {
    need(device.starts_with("/dev/") && device.size()>5 && device.size()<133,"Select an exact /dev kernel device path");
    auto value=device.substr(5);
    need(value!="." && value!=".." && std::all_of(value.begin(),value.end(),[](unsigned char c){return std::isalnum(c)||c=='_'||c=='-'||c=='.';}),"Device aliases, traversal and shell syntax are not accepted");return value;
}
std::string dev_string(dev_t dev){return std::to_string(major(dev))+":"+std::to_string(minor(dev));}
std::string unescape_mount_path(const std::string& value) {
    std::string out;
    for(size_t i=0;i<value.size();++i) {
        if(value[i]!='\\'){out+=value[i];continue;}
        need(i+3<value.size(),"Malformed swap path escape");unsigned n=0;
        for(unsigned j=1;j<=3;++j){need(value[i+j]>='0'&&value[i+j]<='7',"Malformed swap path escape");n=n*8+unsigned(value[i+j]-'0');}
        need(n!=0&&n<256,"Invalid swap path byte");out+=char(n);i+=3;
    }
    need(out.starts_with("/"),"Swap path is not absolute");return out;
}
void no_live_dependencies(const fs::path& node,const std::string& identity,int original_mounts,const fs::path& swaps_path="/proc/swaps") {
    const auto disk=node.parent_path();const auto parent_id=trimmed(file(disk/"dev"));
    // Other non-overlapping partitions may be mounted. Never inspect the live
    // target or a disk/mapping that covers it; ordinary Fedora zram is unrelated.
    for(const auto& object:{node,disk})need(fs::is_directory(object/"holders")&&fs::is_empty(object/"holders"),"Selected partition or parent has block holders");
    auto geometry=[](const fs::path& path){const auto text=trimmed(file(path));uint64_t n{};auto [p,e]=std::from_chars(text.data(),text.data()+text.size(),n);need(e==std::errc{}&&p==text.data()+text.size(),"Partition geometry unavailable");return n;};
    const auto begin=geometry(node/"start"),size=geometry(node/"size"),limit=geometry(disk/"size");
    need(size&&begin<=limit&&size<=limit-begin,"Invalid selected geometry");
    for(const auto& entry:fs::directory_iterator(disk))if(entry.path()!=node&&fs::exists(entry.path()/"partition")) {
        const auto other=geometry(entry.path()/"start"),length=geometry(entry.path()/"size");
        need(other<=limit&&length<=limit-other,"Invalid sibling geometry");
        need(!(begin<other+length&&other<begin+size),"Overlapping partition geometry is refused");
    }
    need(lseek(original_mounts,0,SEEK_SET)==0,"Cannot refresh original mount namespace");
    std::istringstream mounts(read_fd(original_mounts,1024*1024));std::string line;unsigned count=0;
    while(std::getline(mounts,line)) {
        std::istringstream row(line);std::string id,parent,dev,source,target,options,field,type,backing,super;
        need(bool(row>>id>>parent>>dev>>source>>target>>options),"Mount inventory is malformed");
        while(row>>field&&field!="-"){}
        need(field=="-" && bool(row>>type>>backing>>super),"Mount inventory lacks filesystem fields");
        const auto colon=dev.find(':');need(colon!=std::string::npos,"Mount device identity is malformed");
        for(const auto& part:{dev.substr(0,colon),dev.substr(colon+1)})need(!part.empty()&&std::all_of(part.begin(),part.end(),[](char c){return c>='0'&&c<='9';}),"Mount device identity is malformed");
        ++count;
        need(dev!=identity&&dev!=parent_id,"Live/root/boot/mounted target is refused");
    }
    need(count>0,"Mount inventory is empty");
    std::istringstream swaps(file(swaps_path,1024*1024));need(bool(std::getline(swaps,line))&&line.starts_with("Filename"),"Swap inventory unavailable");
    while(std::getline(swaps,line)) {
        if(line.empty())continue;
        std::istringstream row(line);std::string path,type,size_text,used,priority,extra;
        need(bool(row>>path>>type>>size_text>>used>>priority)&&!(row>>extra),"Malformed active swap inventory");
        struct stat st{};need(stat(unescape_mount_path(path).c_str(),&st)==0,"Active swap dependency identity unavailable");
        need((type=="partition"&&S_ISBLK(st.st_mode))||(type=="file"&&S_ISREG(st.st_mode)),"Unsupported active swap dependency");
        const auto id=dev_string(type=="partition"?st.st_rdev:st.st_dev);
        need(id!=identity&&id!=parent_id,"Active swap target is refused");
    }
}
void probe_filesystem(int fd,Target& target) {
    blkid_probe probe=blkid_new_probe();need(probe,"Cannot allocate filesystem probe");
    try {
        need(blkid_probe_set_device(probe,fd,0,0)==0,"Cannot pin filesystem probe");
        blkid_probe_enable_partitions(probe,0);blkid_probe_enable_superblocks(probe,1);
        blkid_probe_set_superblocks_flags(probe,BLKID_SUBLKS_TYPE|BLKID_SUBLKS_UUID|BLKID_SUBLKS_USAGE);
        need(blkid_do_safeprobe(probe)==0,"Filesystem signature is absent or ambiguous");
        const char* value=nullptr;
        need(blkid_probe_lookup_value(probe,"USAGE",&value,nullptr)==0 && std::string_view(value)=="filesystem","Encrypted/container devices require a separate reviewed workflow");
        need(blkid_probe_lookup_value(probe,"TYPE",&value,nullptr)==0,"Filesystem type unavailable");target.filesystem=value;
        (void)no_recovery_options(target.filesystem);
        need(blkid_probe_lookup_value(probe,"UUID",&value,nullptr)==0,"Filesystem UUID is required");target.uuid=value;
        need(target.uuid.size()<=128,"Filesystem UUID is malformed");
    } catch(...) {blkid_free_probe(probe);throw;}
    blkid_free_probe(probe);
}
struct Device {
    Fd fd,original_mounts;fs::path node;Target target;
    explicit Device(const std::string& path) {
        const auto kernel_name=name(path);
        struct stat before{};need(lstat(path.c_str(),&before)==0 && S_ISBLK(before.st_mode),"Target is not a block device");
        fd.n=::open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);struct stat st{};
        need(fd.n>=0 && !fstat(fd.n,&st) && S_ISBLK(st.st_mode),"Target is not an accessible block device");
        need(st.st_ino==before.st_ino && st.st_dev==before.st_dev && st.st_rdev==before.st_rdev,"Device changed while opening");
        target.path=path;const auto dev=dev_string(st.st_rdev);
        node=fs::canonical(fs::path("/sys/dev/block")/dev);
        need(node.filename()==kernel_name && fs::exists(node/"partition"),"Choose a real partition, not a whole disk, mapping or alias");
        need(trimmed(file(node/"dev"))==dev,"Kernel device identity differs");
        original_mounts.n=::open("/proc/self/mountinfo",O_RDONLY|O_CLOEXEC);need(original_mounts.n>=0,"Cannot pin original mount namespace inventory");
        no_live_dependencies(node,dev,original_mounts.n);
        need(flock(fd.n,LOCK_EX|LOCK_NB)==0,"Another maintenance operation holds the device");
        target.identity=identity();
        probe_filesystem(fd.n,target);revalidate();
    }
    std::string identity() const {
        uint64_t sequence=0,bytes=0;struct stat st{};
        need(!fstat(fd.n,&st) && ioctl(fd.n,BLKGETDISKSEQ,&sequence)==0 && sequence && ioctl(fd.n,BLKGETSIZE64,&bytes)==0,"Kernel disk sequence or size unavailable");
        return dev_string(st.st_rdev)+":"+std::to_string(sequence)+":"+std::to_string(bytes)+":"+trimmed(file(node.parent_path()/"diskseq"));
    }
    void revalidate() const {
        struct stat original{},current{};need(!fstat(fd.n,&original) && !lstat(target.path.c_str(),&current) && S_ISBLK(current.st_mode) && current.st_rdev==original.st_rdev && current.st_ino==original.st_ino && current.st_dev==original.st_dev,"Device selection was replaced");
        need(identity()==target.identity,"Disk sequence/size changed; refresh the selection");
        no_live_dependencies(node,dev_string(original.st_rdev),original_mounts.n);
    }
};
struct Mounts {
    fs::path stage,root;std::vector<fs::path> mounted;Fd previous_namespace;bool private_namespace=false;
    explicit Mounts(const fs::path& runtime="/run") {
        previous_namespace.n=::open("/proc/self/ns/mnt",O_RDONLY|O_CLOEXEC);
        need(previous_namespace.n>=0,"Cannot pin original mount namespace");
        need(unshare(CLONE_NEWNS)==0,"Private mount namespace unavailable");private_namespace=true;
        try {
        need(::mount(nullptr,"/",nullptr,MS_REC|MS_PRIVATE,nullptr)==0,"Cannot disable mount propagation");
        // Overlay the runtime directory only inside this private namespace.
        // No host staging directory survives interruption or SIGKILL.
        need(::mount("tmpfs",runtime.c_str(),"tmpfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,"mode=0700,size=16m")==0,"Cannot mount private runtime tmpfs");
        mounted.push_back(runtime);stage=runtime/"uke-chroot";root=stage/"root";
        fs::create_directories(root);
        }catch(...) {cleanup(false);throw;}
    }
    void add(const char* source,const fs::path& target,const char* type,unsigned long flags,const char* options) {
        need(::mount(source,target.c_str(),type,flags,options)==0,"Mount failed: "+target.filename().string()+": "+std::strerror(errno));mounted.push_back(target);
    }
    bool cleanup(bool report=true) {
        bool ok=true;
        for(auto i=mounted.rbegin();i!=mounted.rend();++i)if(umount2(i->c_str(),0)!=0 && errno!=EINVAL && errno!=ENOENT){ok=false;if(report)std::cerr<<"Cleanup mount busy/failed: "<<i->filename()<<": "<<std::strerror(errno)<<'\n';}
        mounted.clear();
        if(private_namespace){if(setns(previous_namespace.n,CLONE_NEWNS)!=0){ok=false;if(report)std::cerr<<"Cannot restore original mount namespace\n";}else private_namespace=false;}
        return ok;
    }
    ~Mounts(){cleanup();}
};
Target mounted_target(Device& device,Mounts& mounts) {
    device.revalidate();
    const std::string source="/proc/self/fd/"+std::to_string(device.fd.n);
    mounts.add(source.c_str(),mounts.root,device.target.filesystem.c_str(),MS_RDONLY|MS_NOSUID|MS_NODEV|MS_NOEXEC,no_recovery_options(device.target.filesystem).c_str());
    device.revalidate();
    Fd root(::open(mounts.root.c_str(),O_PATH|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));need(root.n>=0,"Cannot pin mounted root");
    for(const char* dir:{"dev","proc","sys","run","tmp"}){Fd required(inside(root.n,dir,O_PATH|O_DIRECTORY));}
    const auto release=target_release(root.n);auto target=device.target;target.system=system_id(release);require_native_shell(root.n);target.token=fingerprint(target,release);return target;
}
struct Terminal {
    termios saved{};bool active=false;
    Terminal(){need(isatty(0)&&isatty(1)&&tcgetattr(0,&saved)==0,"Chroot execution requires an interactive terminal");auto raw=saved;cfmakeraw(&raw);need(tcsetattr(0,TCSAFLUSH,&raw)==0,"Cannot configure terminal relay");active=true;}
    ~Terminal(){if(active)tcsetattr(0,TCSAFLUSH,&saved);}
};
void write_all(int fd,const char* data,size_t bytes) {
    while(bytes){ssize_t n=::write(fd,data,bytes);if(n<0&&errno==EINTR)continue;need(n>0,"Terminal relay write failed");data+=n;bytes-=n;}
}
void drop_authority() {
    need(setgroups(0,nullptr)==0,"Cannot clear supplementary groups");
    for(int capability=0;capability<=CAP_LAST_CAP;++capability)need(prctl(PR_CAPBSET_DROP,capability,0,0,0)==0,"Cannot drop target capability bound");
    __user_cap_header_struct header{_LINUX_CAPABILITY_VERSION_3,0};__user_cap_data_struct data[2]{};
    need(syscall(SYS_capset,&header,data)==0 && prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)==0,"Cannot reduce target authority");
}
void bind_parent_lifetime(int parent_pidfd) {
    need(prctl(PR_SET_DUMPABLE,0)==0 && prctl(PR_SET_PDEATHSIG,SIGKILL)==0,"Cannot protect namespace supervisor lifetime");
    pollfd parent{parent_pidfd,POLLIN,0};need(poll(&parent,1,0)==0,"Manager exited during namespace setup");
}
int supervise_target(pid_t shell) {
    int result=0;while(waitpid(shell,&result,0)<0){if(errno!=EINTR)return 125;}
    kill(-1,SIGKILL);while(waitpid(-1,nullptr,0)>0 || errno==EINTR){}
    return WIFEXITED(result)?WEXITSTATUS(result):128+WTERMSIG(result);
}
// Parent remains outside the new PID namespace. Killing its PID 1 also kills
// descendants, including detached jobs; cleanup waits for namespace teardown.
int session(Mounts& mounts) {
    Fd root(::open(mounts.root.c_str(),O_PATH|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));need(root.n>=0,"Cannot pin session root");
    for(const char* dir:{"dev","proc","sys","run","tmp"}){Fd checked(inside(root.n,dir,O_PATH|O_DIRECTORY));}
    // Inspection was noexec. Execution is an explicit, authenticated operation.
    need(::mount(nullptr,mounts.root.c_str(),nullptr,MS_REMOUNT|MS_RDONLY|MS_NOSUID|MS_NODEV,nullptr)==0,"Cannot enable read-only target execution");
    mounts.add("tmpfs",mounts.root/"dev","tmpfs",MS_NOSUID|MS_NOEXEC,"mode=0755,size=1m");
    // Only these character devices are exposed; never bind the host /dev tree.
    for(const char* name:{"null","zero","random","urandom","tty"}) {
        auto destination=mounts.root/"dev"/name;Fd created(::open(destination.c_str(),O_CREAT|O_EXCL|O_WRONLY|O_CLOEXEC,0600));need(created.n>=0,"Cannot create private device bind target");
        std::string source=std::string("/dev/")+name;mounts.add(source.c_str(),destination,nullptr,MS_BIND,nullptr);
    }
    fs::create_directory(mounts.root/"dev/pts");
    mounts.add("devpts",mounts.root/"dev/pts","devpts",MS_NOSUID|MS_NOEXEC,"newinstance,ptmxmode=0666,mode=0620");
    need(::symlink("pts/ptmx",(mounts.root/"dev/ptmx").c_str())==0,"Cannot link private PTY allocator");
    for(const char* dir:{"run","tmp"})mounts.add("tmpfs",mounts.root/dir,"tmpfs",MS_NOSUID|MS_NODEV,"mode=1777,size=64m");
    mounts.add("sysfs",mounts.root/"sys","sysfs",MS_RDONLY|MS_NOSUID|MS_NODEV|MS_NOEXEC,nullptr);
    need(unshare(CLONE_NEWPID|CLONE_NEWNET)==0,"Private PID/network namespace unavailable");
    Fd parent_pidfd(int(syscall(SYS_pidfd_open,getpid(),0)));need(parent_pidfd.n>=0,"Parent lifetime tracking unavailable");
    int master=-1;winsize dimensions{};ioctl(0,TIOCGWINSZ,&dimensions);
    pid_t child=forkpty(&master,nullptr,nullptr,&dimensions);need(child>=0,"Cannot create chroot terminal");
    if(child==0) {
        try {
            bind_parent_lifetime(parent_pidfd.n);
            // /proc must be mounted by a process inside the new PID namespace.
            need(::mount("proc",(mounts.root/"proc").c_str(),"proc",MS_RDONLY|MS_NOSUID|MS_NODEV|MS_NOEXEC,nullptr)==0,"Cannot mount private process view");
            const pid_t shell=fork();need(shell>=0,"Cannot start target shell");
            if(shell>0) {
                // This trusted namespace init never executes target code.
                if(syscall(SYS_close_range,3U,~0U,0)!=0)_exit(125);
                _exit(supervise_target(shell));
            }
            need(fchdir(root.n)==0 && ::chroot(".")==0 && chdir("/")==0,"Cannot enter pinned root");
            drop_authority();
            need(syscall(SYS_close_range,3U,~0U,0)==0,"Cannot close inherited host descriptors");
            char arg0[]="/bin/bash",arg1[]="--noprofile",arg2[]="--norc",arg3[]="-i";
            char* args[]={arg0,arg1,arg2,arg3,nullptr};
            char path[]="PATH=/usr/sbin:/usr/bin:/sbin:/bin",home[]="HOME=/root",term[]="TERM=xterm-256color",prompt[]="PS1=(uke read-only chroot) \\w # ",lang[]="LANG=C.UTF-8";
            char* env[]={path,home,term,prompt,lang,nullptr};execve(arg0,args,env);throw std::runtime_error("Target bash execution failed");
        }catch(const std::exception& error){std::cerr<<error.what()<<'\n';_exit(126);}
    }
    mounts.mounted.push_back(mounts.root/"proc"); // may be absent if child setup failed
    Fd terminal(master);int status=0;bool reaped=false;
    auto reap=[&]{if(!reaped){kill(child,SIGKILL);while(waitpid(child,&status,0)<0 && errno==EINTR){}reaped=true;}};
    try {
        Terminal raw;
        while(!interrupted) {
            pid_t ended=waitpid(child,&status,WNOHANG);if(ended==child){reaped=true;break;}need(ended>=0 || errno==EINTR,"Cannot observe chroot child");
            if(resized){ioctl(0,TIOCGWINSZ,&dimensions);ioctl(master,TIOCSWINSZ,&dimensions);resized=0;}
            pollfd fds[]={{0,POLLIN,0},{master,POLLIN,0}};int result=poll(fds,2,100);if(result<0&&errno==EINTR)continue;need(result>=0,"Terminal polling failed");
            std::array<char,4096> data{};
            for(unsigned i=0;i<2;++i)if(fds[i].revents&POLLIN){ssize_t n=::read(fds[i].fd,data.data(),data.size());if(n<=0){interrupted=SIGHUP;break;}write_all(i?1:master,data.data(),n);}
            if(fds[0].revents&(POLLHUP|POLLERR))interrupted=SIGHUP;
        }
        reap();
        std::array<char,4096> tail{};ssize_t bytes;
        while((bytes=::read(master,tail.data(),tail.size()))>0)write_all(1,tail.data(),bytes);
    }catch(...){reap();throw;}
    // Child proc mount shares this private mount namespace; remove it explicitly.
    if(umount2((mounts.root/"proc").c_str(),0)!=0 && errno!=EINVAL && errno!=ENOENT)throw std::runtime_error("Private proc cleanup failed");
    return interrupted?128+interrupted:WIFEXITED(status)?WEXITSTATUS(status):128+WTERMSIG(status);
}
}
std::string no_recovery_options(const std::string& type) {
    if(type=="ext4")return "noload";
    if(type=="f2fs")return "norecovery";
    throw std::runtime_error("Only direct ext4 and F2FS partitions are admitted; encrypted, mapped, multi-device and subvolume layouts are unsupported");
}
std::string system_id(const std::string& release) {
    need(release.size()<=16384 && release.find('\0')==std::string::npos,"Malformed os-release");
    std::istringstream lines(release);std::string line,id;
    while(std::getline(lines,line))if(line.starts_with("ID=")) {
        need(id.empty(),"Duplicate os-release ID");auto value=trimmed(line.substr(3));
        if(value.size()>=2 && ((value.front()=='"'&&value.back()=='"')||(value.front()=='\''&&value.back()=='\'')))value=value.substr(1,value.size()-2);
        need(value=="fedora"||value=="arch","Root filesystem is not an admitted Fedora or Arch system");id=value;
    }
    need(!id.empty(),"Root filesystem has no Fedora/Arch identity");return id;
}
std::string target_release(int root_fd) {
    Fd etc(inside(root_fd,"etc",O_PATH|O_DIRECTORY,false,true));struct stat entry{};
    bool fallback=etc.n<0;
    if(!fallback && fstatat(etc.n,"os-release",&entry,AT_SYMLINK_NOFOLLOW)!=0){need(errno==ENOENT,"Cannot inspect os-release entry");fallback=true;}
    Fd release(inside(root_fd,fallback?"usr/lib/os-release":"etc/os-release",O_RDONLY|O_NONBLOCK,true));
    return read_fd(release.n);
}
void require_native_shell(int root_fd) {
    Fd shell(inside(root_fd,"bin/bash",O_RDONLY|O_NONBLOCK,true));struct stat st{};
    need(!fstat(shell.n,&st) && S_ISREG(st.st_mode) && (st.st_mode&0111) && !(st.st_mode&(S_ISUID|S_ISGID)),"Target bash is not an executable regular file");
    char capability{};need(fgetxattr(shell.n,"security.capability",&capability,1)<0 && (errno==ENODATA || errno==ENOTSUP),"Target shell has file capabilities or unreadable privilege metadata");
    Elf64_Ehdr header{};need(pread(shell.n,&header,sizeof(header),0)==sizeof(header) && !memcmp(header.e_ident,ELFMAG,SELFMAG) && header.e_ident[EI_CLASS]==ELFCLASS64 && header.e_ident[EI_DATA]==ELFDATA2LSB,"Target bash is not a supported ELF executable");
#if defined(__aarch64__)
    need(header.e_machine==EM_AARCH64,"Target shell architecture differs");
#elif defined(__x86_64__)
    need(header.e_machine==EM_X86_64,"Target shell architecture differs");
#else
    throw std::runtime_error("Host architecture is unsupported");
#endif
}
std::string fingerprint(const Target& target,const std::string& release) {
    const auto input=trimmed(file("/proc/sys/kernel/random/boot_id"))+"\n"+target.path+"\n"+target.identity+"\n"+target.filesystem+"\n"+target.uuid+"\n"+release;
    unsigned char digest[SHA256_DIGEST_LENGTH];SHA256(reinterpret_cast<const unsigned char*>(input.data()),input.size(),digest);
    constexpr char digits[]="0123456789abcdef";std::string out;for(auto c:digest){out+=digits[c>>4];out+=digits[c&15];}return out;
}
std::vector<std::string> candidates(const std::string& device) {
    const auto kernel_name=name(device);auto parent=fs::canonical(fs::path("/sys/class/block")/kernel_name);
    need(uke::sysfs::beneath("/sys/devices",parent),"Invalid block sysfs identity");
    if(fs::exists(parent/"partition"))return {device};
    need(fs::exists(parent/"device") && !fs::exists(parent/"dm"),"Select a physical disk or direct partition");
    std::vector<std::string> result;for(const auto& entry:fs::directory_iterator(parent))if(fs::exists(entry.path()/"partition"))result.push_back("/dev/"+entry.path().filename().string());
    need(result.size()<=128,"Too many root candidates");std::sort(result.begin(),result.end());return result;
}
Target inspect(const std::string& device) {signals();Device pinned(device);Mounts mounts;auto result=mounted_target(pinned,mounts);need(mounts.cleanup(),"Inspection cleanup failed");return result;}
int enter(const std::string& device,const std::string& expected_token) {
    signals();need(isatty(0)&&isatty(1),"Enter requires an interactive terminal");
    Device pinned(device);Mounts mounts;auto target=mounted_target(pinned,mounts);
    need(target.token==expected_token,"Stale selection: filesystem identity changed; inspect again");
    pinned.revalidate();const int result=session(mounts);need(mounts.cleanup(),"Session ended but mount cleanup failed");return result;
}
std::string scan(const std::string& device) {
    auto paths=candidates(device);std::string out="{\"schema\":\"org.uke.control.chroot.v1\",\"read_only\":true,\"candidates\":[";bool comma=false;
    for(const auto& path:paths) {
        if(comma)out+=',';
        comma=true;out+="{\"path\":"+quote(path);
        try {const auto target=inspect(path);out+=",\"ready\":true,\"system\":"+quote(target.system)+",\"filesystem\":"+quote(target.filesystem)+",\"identity\":"+quote(target.identity)+",\"token\":"+quote(target.token);}
        catch(const std::exception& error){out+=",\"ready\":false,\"reason\":"+quote(error.what());}
        out+='}';
    }
    return out+"]}";
}
}
