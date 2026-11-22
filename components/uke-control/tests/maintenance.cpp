// SPDX-License-Identifier: MIT
// All paths and subprocesses belong to this disposable fixture.
#include "maintenance.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <signal.h>
#include <sys/wait.h>
#include <thread>
namespace fs=std::filesystem;
template<class F>void refused(F&& call){bool denied=false;try{call();}catch(const std::exception&){denied=true;}assert(denied);}
int main(int argc,char** argv) {
    if(argc==3&&std::string_view(argv[1])=="--worker") {
        const int fd=std::stoi(argv[2]);assert(write(fd,"R",1)==1);close(fd);
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));return 0;
    }
    char pattern[]="/tmp/uke-maintenance-fixture-XXXXXX";assert(mkdtemp(pattern));const fs::path root=pattern;
    const auto meminfo=root/"meminfo";
    for(const auto* text:{"MemAvailable: 524287 kB\n","MemAvailable: -1 kB\n","MemAvailable: 600000 kB\nMemAvailable: 600000 kB\n","MemAvailable: 600000 MB\n","MemTotal: 600000 kB\n"}) {
        std::ofstream(meminfo)<<text;refused([&]{uke::require_staging_budget(root,meminfo);});
    }
    std::ofstream(meminfo)<<"MemAvailable: 600000 kB\n";
    if(fs::space(root).available>=512ULL*1024*1024)uke::require_staging_budget(root,meminfo);
    for(unsigned i=0;i<10;++i) {
        const auto stage=root/("stage-"+std::to_string(i));
        {fs::create_directories(stage/"tree");std::ofstream(stage/"tree/file")<<"temporary";uke::Scratch scratch{stage};}
        assert(!fs::exists(stage));
    }
    const auto retained=root/"published";fs::create_directories(retained);std::ofstream(retained/"previous")<<"recovery";
    {uke::Scratch scratch{retained,true};}assert(fs::exists(retained/"previous"));
    fs::create_directories(root/"source");fs::create_directories(root/"destination");
    refused([&]{uke::publish_noreplace(root/"source",root/"destination");});assert(fs::exists(root/"source")&&fs::exists(root/"destination"));
    fs::remove(root/"destination");uke::publish_noreplace(root/"source",root/"destination");assert(!fs::exists(root/"source"));
    if(geteuid()==0) {
        const auto path=(root/"lock").string();
        {uke::Maintenance lease(path.c_str());refused([&]{uke::Maintenance second(path.c_str());});}
        fs::create_symlink(root/"lock",root/"symlink");refused([&]{uke::Maintenance lease((root/"symlink").c_str());});
        fs::create_hard_link(root/"lock",root/"hardlink");refused([&]{uke::Maintenance lease(path.c_str());});fs::remove(root/"hardlink");
        assert(!chmod(path.c_str(),0666));refused([&]{uke::Maintenance lease(path.c_str());});assert(!chmod(path.c_str(),0600));
        int ready[2];assert(!pipe(ready));const auto controller=fork();assert(controller>=0);
        if(!controller) {
            close(ready[0]);uke::Maintenance lease(path.c_str());const auto worker=fork();assert(worker>=0);
            if(!worker){const auto fd=std::to_string(ready[1]);execl("/proc/self/exe","test-maintenance","--worker",fd.c_str(),static_cast<char*>(nullptr));_exit(127);}
            close(ready[1]);for(;;)pause();
        }
        close(ready[1]);char byte=0;assert(read(ready[0],&byte,1)==1&&byte=='R');close(ready[0]);
        assert(!kill(controller,SIGTERM));int status=0;assert(waitpid(controller,&status,0)==controller);
        refused([&]{uke::Maintenance lease(path.c_str());});
        std::this_thread::sleep_for(std::chrono::milliseconds(1700));
        {uke::Maintenance lease(path.c_str());}
    }
    fs::remove_all(root);
}
