// SPDX-License-Identifier: MIT
#include "chroot.hpp"
#include "cli.hpp"
#include <algorithm>
#include <csignal>
#include <fcntl.h>
#include <iostream>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
namespace {
void need(bool ok,const std::string& reason){if(!ok)throw std::runtime_error(reason);}
const char* warning="This runs target-controlled Fedora/Arch code as UID 0. Chroot shares the host kernel and is NOT a security boundary. Only enter a trusted system. The root is read-only; networking, host block devices and privileged capabilities are unavailable. Background jobs end with this session.";
struct Lock {
    int fd=-1;
    Lock(){fd=open("/run/lock/uke-control-chroot.lock",O_CREAT|O_RDWR|O_NOFOLLOW|O_CLOEXEC,0600);struct stat st{};
        need(fd>=0 && !fstat(fd,&st) && S_ISREG(st.st_mode) && st.st_uid==0 && st.st_nlink==1 && (st.st_mode&0777)==0600,"Invalid maintenance lock");
        need(flock(fd,LOCK_EX|LOCK_NB)==0,"Another chroot inspection/session is active");}
    ~Lock(){if(fd>=0)close(fd);}
};
void confirmation(const std::string& path){
    need(isatty(0)&&isatty(1),"Chroot requires an interactive terminal");
    std::cout<<warning<<"\nType ENTER "<<path<<" to authorize this target: "<<std::flush;
    std::string answer;need(bool(std::getline(std::cin,answer)) && answer=="ENTER "+path,"Target trust confirmation declined");
}
}
int main(int argc,char** argv) {
    try {
        using namespace uke::chroot_manager;
        if(argc==2 && std::string_view(argv[1])=="--help") {
            std::cout<<"chroot-manager scan /dev/DEVICE [--json]\nchroot-manager enter /dev/PARTITION TOKEN --trust-target-root-code\nchroot-manager manage\n"<<warning<<'\n';return 0;
        }
        const std::string action=argc>1?argv[1]:"";
        const bool scanning=action=="scan" && (argc==3 || (argc==4 && std::string_view(argv[3])=="--json"));
        const bool entering=action=="enter" && argc==5 && std::string_view(argv[4])=="--trust-target-root-code";
        const bool managing=action=="manage" && argc==2;
        need(scanning||entering||managing,"Invalid chroot command; no arbitrary command or writable-root option is accepted");
        need(getuid()==0 && geteuid()==0,"Administrator authorization is required through the installed pkexec policy");
        umask(0077);Lock lock;
        if(scanning){uke::cli::print(scan(argv[2]),argc==4,"Chroot inspection");return 0;}
        if(entering){confirmation(argv[2]);return enter(argv[2],argv[3]);}
        need(isatty(0)&&isatty(1),"Manager requires a terminal");
        std::cout<<"Read-only Fedora / Arch chroot manager\nSelect a physical disk or partition, e.g. /dev/sda.\nInspection mounts untrusted filesystem metadata with recovery disabled.\nDevice: "<<std::flush;
        std::string selected;need(bool(std::getline(std::cin,selected)),"No device selected");
        const auto devices=candidates(selected);std::vector<Target> ready;
        for(const auto& path:devices) {
            try{auto target=inspect(path);ready.push_back(target);std::cout<<ready.size()<<") "<<path<<" — "<<target.system<<" / "<<target.filesystem<<'\n';}
            catch(const std::exception& error){std::cout<<path<<": unavailable — "<<error.what()<<'\n';}
        }
        need(!ready.empty(),"No admitted Fedora/Arch root filesystem was found");
        std::cout<<"Choose the displayed root number (empty cancels): "<<std::flush;std::string value;need(bool(std::getline(std::cin,value)),"Selection cancelled");
        need(!value.empty()&&value.size()<4&&std::all_of(value.begin(),value.end(),[](char c){return c>='0'&&c<='9';}),"Invalid root number");
        const auto index=std::stoul(value);need(index>0&&index<=ready.size(),"Root number is out of range");
        const auto& target=ready[index-1];confirmation(target.path);return enter(target.path,target.token);
    }catch(const std::exception& error){std::cerr<<"uke-control chroot: "<<error.what()<<'\n';return 1;}
}
