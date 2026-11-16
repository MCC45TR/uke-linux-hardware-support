// SPDX-License-Identifier: MIT
// Actual policy/transport implementation, private files and a real Linux PTY.
#include "backend.hpp"
#include "sysfs.hpp"
#include "setup-cdc.hpp"
#include <array>
#include <cassert>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <cstdarg>
#include <fcntl.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <json/json.h>
namespace fs=std::filesystem;
static fs::path fixture;
static uid_t caller=0,owner=0,tty_owner=0;
static std::string board("xiaomi,uke\0qcom,sm7675\0",23);
static int helper_status=0,helper_calls=0,terminal=-1;
static fs::path mapped(const char* path){assert(path[0]=='/');return fixture/std::string(path+1);}
static void text(const fs::path& path,const std::string& value,mode_t mode=0600) {
    fs::create_directories(path.parent_path());std::ofstream out(path);out<<value;out.close();assert(out.good());assert(!chmod(path.c_str(),mode));
}
static int native_open(const char* path,int flags,...) {
    mode_t mode=0;
    if(flags&O_CREAT){va_list args;va_start(args,flags);mode=va_arg(args,int);va_end(args);}
    if(std::string_view(path)=="/dev/ttyGS0") {
        assert((flags&(O_NOFOLLOW|O_NOCTTY|O_NONBLOCK))==(O_NOFOLLOW|O_NOCTTY|O_NONBLOCK));
        assert(terminal>=0);return dup(terminal);
    }
    return ::open(mapped(path).c_str(),flags,mode);
}
static int native_fstat(int fd,struct stat* st) {
    const int result=::fstat(fd,st);if(!result)st->st_uid=isatty(fd)?tty_owner:owner;return result;
}
static int native_lstat(const char* path,struct stat* st){return ::lstat(mapped(path).c_str(),st);}
static int native_mkdir(const char* path,mode_t mode){return ::mkdir(mapped(path).c_str(),mode);}
static int native_unlink(const char* path){return ::unlink(mapped(path).c_str());}
static uid_t native_uid(){return caller;}
#define open native_open
#define fstat native_fstat
#define lstat native_lstat
#define mkdir native_mkdir
#define unlink native_unlink
#define geteuid native_uid
#include "../src/setup-cdc.cpp"
#undef open
#undef fstat
#undef lstat
#undef mkdir
#undef unlink
#undef geteuid
namespace uke {
std::string read(const std::string& path,size_t limit) {
    if(path=="/sys/firmware/devicetree/base/compatible")return board;
    std::ifstream in(mapped(path.c_str()),std::ios::binary);
    std::string value{std::istreambuf_iterator<char>(in),{}};assert(value.size()<limit);return value;
}
Result cdc_set(bool enabled,bool wait) {
    assert(wait); // Setup entry and completion serialize with boot swap writers.
    helper_calls++;if(helper_status)return {helper_status,"synthetic refusal"};
    const auto profile=mapped("/run/senemos-uke/usb-gadget-profile");
    if(enabled)text(profile,"manual-acm\n");else fs::remove(profile);
    text(mapped("/sys/kernel/config/usb_gadget/uke_debug/UDC"),enabled?"fixture.udc\n":"\n");
    return {0,"{\"synthetic\":true}\n"};
}
// Preserve the real shared-library call boundary. GCC 16 AArch64 otherwise
// misdiagnoses IPA of this fixture's asserted vector stub (as in cdc-policy).
#if defined(__GNUC__) && !defined(__clang__)
[[gnu::noipa]]
#endif
ProcessResult execute_bounded(const std::vector<std::string>& args,unsigned seconds,size_t limit) {
    assert(!args.empty());std::ofstream(fixture/"commands",std::ios::app)<<args[0]<<'\n';
    if(args[0]=="/usr/bin/journalctl") {
        assert((args==std::vector<std::string>{"/usr/bin/journalctl","-b","-k","-n","128","--no-pager","-o","short-monotonic"}));
        assert(seconds==5&&limit==16384);return {0,"line \"one\"\nline \\two\n"};
    }
    if(args.size()==4&&args[0]=="/usr/bin/systemctl"&&args[2]=="--reboot-argument=bootloader") {
        assert((args==std::vector<std::string>{"/usr/bin/systemctl","--no-block","--reboot-argument=bootloader","reboot"}));
        assert(seconds==8&&limit==4096);return {23,"synthetic reboot refusal"};
    }
    assert(args[0]=="/usr/bin/systemctl"&&seconds<=20&&limit==4096);
    const std::vector<std::string> stop{"/usr/bin/systemctl","stop","uke-control-setup-console.service"};
    const std::vector<std::string> disable{"/usr/bin/systemctl","disable","uke-control-setup-cdc.service","uke-control-setup-cdc-finish.path"};
    const std::vector<std::string> end{"/usr/bin/systemctl","--no-block","stop","uke-control-setup-cdc.service","uke-control-setup-cdc-finish.path"};
    assert(args==stop||args==disable||args==end);
    // No service callback may run while the production policy lock is held.
    const int fd=::open(mapped("/var/lib/uke-control/cdc-policy.lock").c_str(),O_RDWR);
    assert(fd>=0&&!flock(fd,LOCK_EX|LOCK_NB));close(fd);return {0,""};
}
}
template<class Call>static void refuses(Call call) {
    bool rejected=false;try{call();}catch(const std::exception&){rejected=true;}assert(rejected);
}
static int run(const char* mode) {char* arg=const_cast<char*>(mode);return uke::setup_cdc::command(1,&arg);}
static void transmit(int fd,const std::string& line) {
    size_t sent=0;while(sent<line.size()){const auto n=write(fd,line.data()+sent,line.size()-sent);assert(n>0);sent+=size_t(n);}
}
static Json::Value packet(int fd) {
    std::string line;char ch=0;
    while(line.size()<32768) {
        pollfd wait{fd,POLLIN,0};assert(poll(&wait,1,5000)>0);
        const auto count=::read(fd,&ch,1);if(count<0&&errno==EAGAIN)continue;
        assert(count==1);if(ch=='\n')break;line+=ch;
    }
    Json::Value value;Json::CharReaderBuilder builder;std::string errors;
    const auto parser=std::unique_ptr<Json::CharReader>(builder.newCharReader());
    assert(parser->parse(line.data(),line.data()+line.size(),&value,&errors));return value;
}
int main() {
    char pattern[]="/tmp/uke-setup-cdc-test-XXXXXXXX";const char* made=mkdtemp(pattern);assert(made);fixture=made;
    fs::create_directories(mapped("/etc/uke-control"));fs::create_directories(mapped("/var/lib/uke-control"));fs::create_directories(mapped("/run"));
    text(mapped("/etc/uke-control/first-setup-cdc"),"v1\n",0644);
    caller=1000;refuses([]{run("apply");});caller=0;
    owner=1000;refuses([]{run("apply");});owner=0;
    const auto good_board=board;board="not-uke";refuses([]{run("apply");});board=good_board;
    board="prefix"+good_board;refuses([]{run("apply");});board=good_board;
    const auto marker=mapped("/etc/uke-control/first-setup-cdc");
    chmod(marker.c_str(),0666);refuses([]{run("apply");});chmod(marker.c_str(),0644);
    fs::create_hard_link(marker,fixture/"marker-link");refuses([]{run("apply");});fs::remove(fixture/"marker-link");
    fs::rename(marker,fixture/"marker-original");fs::create_symlink(fixture/"marker-original",marker);refuses([]{run("apply");});fs::remove(marker);fs::rename(fixture/"marker-original",marker);
    assert(!helper_calls);refuses([]{run("finish");});
    helper_status=69;assert(!run("apply"));assert(uke::setup_cdc::claimed());helper_status=0;
    assert(!run("apply")&&uke::setup_cdc::ready());
    text(mapped("/sys/class/udc/fixture.udc/state"),"configured\n",0444);
    const int master=posix_openpt(O_RDWR|O_NOCTTY|O_NONBLOCK);assert(master>=0&&!grantpt(master)&&!unlockpt(master));
    terminal=::open(ptsname(master),O_RDWR|O_NOCTTY|O_NONBLOCK);assert(terminal>=0);
    struct stat st{};assert(!::fstat(terminal,&st));
    text(mapped("/sys/class/tty/ttyGS0/dev"),std::to_string(major(st.st_rdev))+":"+std::to_string(minor(st.st_rdev))+"\n",0444);
    const pid_t child=fork();assert(child>=0);
    if(!child){try{_exit(run("console"));}catch(...){_exit(77);}}
    assert(packet(master)["root_shell"]==false);
    transmit(master,"status\r");assert(packet(master)["setup_active"]==true);
    transmit(master,"logs; reboot bootloader\r");assert(packet(master)["error"]=="unknown_command");
    transmit(master,"logs\r");assert(packet(master)["kernel_log"]=="line \"one\"\nline \\two\n");
    transmit(master,"reboot bootloader\r");assert(packet(master)["requested"]=="reboot bootloader");assert(packet(master)["status"]==23);
    int status=0;assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==23);
    close(master);close(terminal);terminal=-1;
    text(mapped("/etc/plasma-setup-done"),"complete\n",0644);
    assert(run("ready")==1);refuses([]{run("console");});assert(!run("finish")&&!uke::setup_cdc::claimed());
    const int calls=helper_calls;assert(!run("apply")&&helper_calls==calls);
    fs::remove(mapped("/etc/plasma-setup-done"));text(mapped("/var/lib/uke-control/cdc-on"),"on\n",0600);
    assert(!run("apply")&&!uke::setup_cdc::claimed());
    text(mapped("/etc/plasma-setup-done"),"complete\n",0644);
    const int saved_calls=helper_calls;assert(!run("finish")&&helper_calls==saved_calls);
    assert(uke::read("/var/lib/uke-control/cdc-on",8)=="on\n");
    fs::remove_all(fixture);
    std::cout<<"PASS: actual setup policy guards, exact commands, Linux PTY JSON transport, fixed reboot refusal, completion cleanup and saved CDC preservation; synthetic only\n";
}
