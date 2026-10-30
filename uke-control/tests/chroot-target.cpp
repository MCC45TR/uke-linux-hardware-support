// SPDX-License-Identifier: MIT
// Trusted, static native mock for session testing. Not shipped to the tablet.
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <linux/capability.h>
#include <sys/prctl.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <unistd.h>
int main(){
    assert(getuid()==0 && getpid()==2);
    assert(prctl(PR_GET_NO_NEW_PRIVS,0,0,0,0)==1);
    __user_cap_header_struct header{_LINUX_CAPABILITY_VERSION_3,0};__user_cap_data_struct data[2]{};
    assert(syscall(SYS_capget,&header,data)==0);
    for(const auto& word:data)assert(!word.effective&&!word.permitted&&!word.inheritable);
    struct statvfs filesystem{};assert(statvfs("/",&filesystem)==0 && (filesystem.f_flag&ST_RDONLY));
    assert(statvfs("/proc",&filesystem)==0 && (filesystem.f_flag&ST_RDONLY));
    assert(open("/proc/sys/kernel/hostname",O_WRONLY)<0 && errno==EROFS);
    assert(open("/etc/should-not-exist",O_CREAT|O_WRONLY,0600)<0 && errno==EROFS);
    assert(open("/proc/1/mem",O_RDONLY)<0 && (errno==EACCES||errno==EPERM));
    assert(open("/dev/sda",O_RDONLY)<0 && errno==ENOENT);
    int writable=open("/tmp/scratch",O_CREAT|O_WRONLY,0600);assert(writable>=0);close(writable);
    auto child=fork();assert(child>=0);if(!child){assert(setsid()>=0);for(;;)pause();}
    std::puts("UKE_CHROOT_SESSION_FIXTURE_PASS");return 19;
}
