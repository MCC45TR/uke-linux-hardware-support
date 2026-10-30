// Host-only syscall fixture for the actual kernel torch request client.
#pragma once
#include "lights.hpp"
#include "../shared/uke-torch.h"
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <pwd.h>
#include <selinux/selinux.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <systemd/sd-bus.h>
#include <unistd.h>
#include <utility>
#include <vector>
namespace torch_fixture {
inline unsigned uid=983,closed=0;
inline bool wrong_owner=false,wrong_label=false,wrong_type=false,fail_request=false,fail_status=false;
inline uke_torch_status response{UKE_TORCH_ABI,100,0,1,UKE_TORCH_MAX_UA,0,0,0};
inline std::vector<std::pair<unsigned long,unsigned>> requests;
inline passwd* user(const char* name){assert(std::string(name)=="uke-rgb");static passwd p{};p.pw_uid=983;return &p;}
inline uid_t euid(){return uid;}inline gid_t egid(){return 983;}
inline int opened(const char* name,int flags,...){assert(std::string(name)=="/dev/uke-torch" && flags==(O_RDWR|O_NOFOLLOW|O_CLOEXEC));return 82;}
inline int stat(int fd,struct stat* out){assert(fd==82);out->st_mode=(wrong_type?S_IFREG:S_IFCHR)|0660;out->st_uid=0;out->st_gid=wrong_owner?42:983;return 0;}
inline int filecon(int fd,char** label){assert(fd==82);*label=strdup(wrong_label?"system_u:object_r:device_t:s0":"system_u:object_r:uke_torch_device_t:s0");return 1;}
inline int ioctl(int fd,unsigned long command,const void* data){
    assert(fd==82);
    if(command==UKE_TORCH_STATUS){if(fail_status){errno=ENODEV;return -1;}*const_cast<uke_torch_status*>(static_cast<const uke_torch_status*>(data))=response;return 0;}
    assert(command==UKE_TORCH_SET || command==UKE_TORCH_ENABLE);
    const auto& value=*static_cast<const uke_torch_request*>(data);
    assert(value.abi==UKE_TORCH_ABI && !value.reserved[0] && !value.reserved[1]);
    requests.emplace_back(command,value.value);if(fail_request){errno=EIO;return -1;}return 0;
}
inline int close(int fd){assert(fd==82);++closed;return 0;}
}
#define getpwnam torch_fixture::user
#define geteuid torch_fixture::euid
#define getegid torch_fixture::egid
#define open torch_fixture::opened
#define fstat torch_fixture::stat
#define fgetfilecon torch_fixture::filecon
#define ioctl torch_fixture::ioctl
#define close torch_fixture::close
#include "../src/torch.cpp"
#undef getpwnam
#undef geteuid
#undef getegid
#undef open
#undef fstat
#undef fgetfilecon
#undef ioctl
#undef close
