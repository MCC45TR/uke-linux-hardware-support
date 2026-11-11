// SPDX-License-Identifier: MIT
// Human terminal interface; the boot owner performs every metadata write.
#define UKE_UPDATE_STATUS_TEST
#include "update-status.cpp"
#include <sys/wait.h>
static void kernel_helper(const std::string &target,const std::string &release,bool apply) {
    const char *args[]={"/usr/bin/uke-kernel-update",apply?"select-slot":"check-select-slot",target.c_str(),release.c_str(),nullptr};
    const auto child=fork();uke::update_status::need(child>=0,"Cannot launch the guarded boot helper");
    if(!child){execv(args[0],const_cast<char *const *>(args));_exit(127);}
    int status=0;while(waitpid(child,&status,0)<0)uke::update_status::need(errno==EINTR,"Cannot wait for guarded slot selection");
    uke::update_status::need(WIFEXITED(status) && WEXITSTATUS(status)==0,"The guarded boot helper refused or could not finish selection; inspect its explanation and update status before retrying");
}
int main(int argc,char **argv) {
    using namespace uke::update_status;
    try {
        need((argc==2 || (argc==3 && std::string(argv[2])=="--dry-run")),"Usage: uke-control set-slot a|b|next [--dry-run]");
        const std::string requested=argv[1];const bool preview=argc==3;
        need(requested=="a" || requested=="b" || requested=="next","Choose slot a, b or next");
        need(geteuid()==0,"Administrator access is required: sudo uke-control set-slot a|b|next");
        const auto status=inspect();
        need(!status["current_slot"].isNull() && !status["next_boot_slot"].isNull(),"Current and next boot slots must be provable; inspect uke-control update status");
        const auto target=requested=="next"?(status["current_slot"].asString()=="a"?"b":"a"):requested;
        const unsigned index=target=="b"?1:0;
        need(!status["slots"][index]["kernel_release"].isNull(),"Target slot kernel cannot be identified");
        const auto release=status["slots"][index]["kernel_release"].asString();
        print(status,false);
        std::cout<<"\nRequested next boot: slot "<<(index?'B':'A')<<" with kernel "<<release<<"\n"
                 <<"Checking the existing image, installed bundle and recovery state…\n"<<std::flush;
        kernel_helper(target,release,false);
        if(preview){std::cout<<"Preview only. No slot was changed and no reboot was requested.\n";return 0;}
        const auto fresh=inspect();
        if(!fresh["next_boot_slot"].isNull() && fresh["next_boot_slot"].asString()==target) {
            std::cout<<"Slot "<<(index?'B':'A')<<" is already selected. No change or reboot.\n";return 0;
        }
        need(isatty(STDIN_FILENO) && isatty(STDOUT_FILENO),"Slot changes require an interactive terminal; use --dry-run to inspect them");
        const std::string confirmation=std::string("SLOT ")+(index?'B':'A');
        std::cout<<"\nThis selects the next boot. Later package policy may stage another kernel.\n"
                 <<"The command will not reboot the device.\nType "<<confirmation<<" to select this kernel, or press Enter to cancel: "<<std::flush;
        std::string answer;need(bool(std::getline(std::cin,answer)) && answer==confirmation,"Slot selection cancelled; no change was requested");
        kernel_helper(target,release,true);
        std::cout<<"\n";print(inspect(),false);return 0;
    } catch(const std::exception &e) {std::cerr<<"uke-control set-slot: "<<e.what()<<'\n';return 1;}
}
