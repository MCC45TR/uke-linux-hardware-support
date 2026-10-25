// Host-only syscall/provider fixture for the actual torch actuator.
#pragma once
#include "lights.hpp"
#include <cassert>
#include <fcntl.h>
#include <grp.h>
#include <linux/magic.h>
#include <pwd.h>
#include <selinux/selinux.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <systemd/sd-bus.h>
#include <unistd.h>
#include <vector>
namespace torch_fixture {
inline unsigned uid=983,brightness=0,fail_level=9;
inline bool wrong_provider=false,wrong_label=false,wrong_owner=false,wrong_fs=false,bad_readback=false;
inline std::vector<unsigned> writes;
inline unsigned metadata_writes=0;
inline passwd* user(const char* name){assert(std::string(name)=="uke-rgb");static passwd p{};p.pw_uid=983;return &p;}
inline group* group_lookup(const char* name){assert(std::string(name)=="uke-rgb");static group g{};g.gr_gid=983;return &g;}
inline uid_t euid(){return uid;}inline gid_t egid(){return 983;}
inline int opened(const char* name,int flags,...){
    if(std::string(name)=="/run/uke-control-torch/lock"){assert(flags&O_NOFOLLOW);return 81;}
    assert(std::string(name)=="/sys/devices/torch/brightness" && (flags&O_NOFOLLOW));return 82;
}
inline int statfs(int fd,struct statfs* out){assert(fd==82);out->f_type=wrong_fs?0:SYSFS_MAGIC;return 0;}
inline int stat(int fd,struct stat* out){assert(fd==81 || fd==82);out->st_mode=S_IFREG|(fd==81?0600:0664);out->st_uid=fd==81?uid:0;out->st_gid=wrong_owner?42:983;out->st_nlink=1;return 0;}
inline int filecon(int fd,char** label){assert(fd==82);*label=strdup(wrong_label?"system_u:object_r:sysfs_t:s0":"system_u:object_r:uke_torch_brightness_t:s0");return 1;}
inline int lock(int fd,int flags){assert(fd==81 && flags==(LOCK_EX|LOCK_NB));return 0;}
inline off_t seek(int fd,off_t offset,int origin){assert(fd==82 && !offset && origin==SEEK_SET);return 0;}
inline ssize_t write(int fd,const void* data,size_t size){
    assert(fd==82 && size==2);const auto* text=static_cast<const char*>(data);assert(text[0]>='0' && text[0]<='2' && text[1]=='\n');
    const unsigned value=text[0]-'0';writes.push_back(value);if(value==fail_level){errno=EIO;return -1;}brightness=value;return size;
}
inline int close(int fd){assert(fd==81 || fd==82);return 0;}
inline int enforcing(){return 1;}
inline int setcon(int fd,const char*){assert(fd==82);++metadata_writes;return 0;}
inline int owner(int fd,uid_t user,gid_t group){assert(fd==82 && user==0 && (group==0 || group==983));++metadata_writes;return 0;}
inline int mode(int fd,mode_t mode){assert(fd==82 && (mode==0644 || mode==0664));++metadata_writes;return 0;}
}
namespace uke::lights {
inline Source fake_discover(bool rgb){assert(!rgb);Source s;s.path=torch_fixture::wrong_provider?"/sys/devices/changed":"/sys/devices/torch";s.brightness=torch_fixture::bad_readback?99:torch_fixture::brightness;return s;}
inline std::string fake_snapshot(){return "{\"schema\":\"org.uke.control.lights.v1\",\"flashlight\":{\"available\":true,\"on\":"+std::string(torch_fixture::brightness?"true":"false")+",\"brightness_percent\":"+std::to_string(torch_fixture::brightness*50)+",\"max_microamp\":20000},\"rgb\":{\"available\":false,\"on\":null,\"brightness_percent\":null}}";}
}
#define getpwnam torch_fixture::user
#define getgrnam torch_fixture::group_lookup
#define geteuid torch_fixture::euid
#define getegid torch_fixture::egid
#define open torch_fixture::opened
#define fstatfs torch_fixture::statfs
#define fstat torch_fixture::stat
#define fgetfilecon torch_fixture::filecon
#define flock torch_fixture::lock
#define lseek torch_fixture::seek
#define write torch_fixture::write
#define close torch_fixture::close
#define discover fake_discover
#define snapshot fake_snapshot
#define is_selinux_enabled torch_fixture::enforcing
#define security_getenforce torch_fixture::enforcing
#define fsetfilecon torch_fixture::setcon
#define fchown torch_fixture::owner
#define fchmod torch_fixture::mode
#include "../src/torch.cpp"
#undef getpwnam
#undef getgrnam
#undef geteuid
#undef getegid
#undef open
#undef fstatfs
#undef fstat
#undef fgetfilecon
#undef flock
#undef lseek
#undef write
#undef close
#undef discover
#undef snapshot
#undef is_selinux_enabled
#undef security_getenforce
#undef fsetfilecon
#undef fchown
#undef fchmod
