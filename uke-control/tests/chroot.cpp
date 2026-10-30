// SPDX-License-Identifier: MIT
#include "chroot.hpp"
#include <cassert>
#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
namespace fs=std::filesystem;
using namespace uke::chroot_manager;
void refuses(const std::function<void()>& f){bool failed=false;try{f();}catch(...){failed=true;}assert(failed);}
void put(const fs::path& path,const std::string& text){fs::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);out<<text;assert(out.good());}
int main() {
    assert(system_id("NAME=Fedora\nID=fedora\n")=="fedora");assert(system_id("ID=\"arch\"\n")=="arch");
    for(const auto& text:{"ID=ubuntu\n","ID=fedora\nID=arch\n","ID=$(id)\n","ID=\"fedora;id\"\n","NAME=arch\n"})refuses([&]{system_id(text);});
    assert(no_recovery_options("ext4")=="noload");assert(no_recovery_options("f2fs")=="norecovery");
    for(const auto& type:{"crypto_LUKS","btrfs","LVM2_member","xfs",""})refuses([&]{no_recovery_options(type);});
    Target target{"/dev/sda1","8:1:2:12345:2","ext4","uuid","fedora",""};
    const auto token=fingerprint(target,"ID=fedora\n");assert(token.size()==64);
    auto stale=target;stale.identity="8:1:3:12345:3";assert(fingerprint(stale,"ID=fedora\n")!=token);
    assert(fingerprint(target,"ID=arch\n")!=token);
    char pattern[]="/tmp/uke-chroot-files-XXXXXX";auto made=mkdtemp(pattern);assert(made);fs::path root=made;
    int fd=open(root.c_str(),O_PATH|O_DIRECTORY);assert(fd>=0);
    try {
        put(root/"usr/lib/os-release","ID=fedora\n");fs::create_directories(root/"etc");fs::create_symlink("../usr/lib/os-release",root/"etc/os-release");
        assert(system_id(target_release(fd))=="fedora");
        fs::remove(root/"etc/os-release");fs::create_symlink("/proc/self/environ",root/"etc/os-release");refuses([&]{target_release(fd);});
        fs::remove(root/"etc/os-release");mkfifo((root/"etc/os-release").c_str(),0600);refuses([&]{target_release(fd);});
        fs::remove(root/"etc/os-release");put(root/"etc/os-release",std::string(16385,'x'));refuses([&]{target_release(fd);});
        Elf64_Ehdr elf{};memcpy(elf.e_ident,ELFMAG,SELFMAG);elf.e_ident[EI_CLASS]=ELFCLASS64;elf.e_ident[EI_DATA]=ELFDATA2LSB;
#if defined(__aarch64__)
        elf.e_machine=EM_AARCH64;
#else
        elf.e_machine=EM_X86_64;
#endif
        put(root/"usr/bin/bash",std::string(reinterpret_cast<char*>(&elf),sizeof(elf)));chmod((root/"usr/bin/bash").c_str(),0755);
        fs::create_symlink("usr/bin",root/"bin");require_native_shell(fd); // inspected only, never executed
        chmod((root/"usr/bin/bash").c_str(),04755);refuses([&]{require_native_shell(fd);});chmod((root/"usr/bin/bash").c_str(),0755);
        elf.e_machine=EM_ARM;put(root/"usr/bin/bash",std::string(reinterpret_cast<char*>(&elf),sizeof(elf)));refuses([&]{require_native_shell(fd);});
        close(fd);fs::remove_all(root);
    }catch(...){close(fd);fs::remove_all(root);throw;}
    std::cout<<"Chroot policy: Fedora/Arch identity, duplicate/injection refusal, no-recovery allowlist, stale tokens, openat2 containment, FIFO bounds and native non-setid ELF inspection passed; no target code executed\n";
}
