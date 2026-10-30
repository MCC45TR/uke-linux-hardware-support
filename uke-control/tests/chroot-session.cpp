// SPDX-License-Identifier: MIT
// Actual session lifecycle with a project-owned static native mock, no distro code.
#include "../src/chroot.cpp"
#include <cassert>
using namespace uke::chroot_manager;
int main(int argc,char** argv){
    assert(argc==2);const auto fixture=fs::canonical(argv[1]);
    int master=-1;const auto test=forkpty(&master,nullptr,nullptr,nullptr);assert(test>=0);
    if(!test){try {
        signals();Mounts mounts;mounts.add("tmpfs",mounts.root,"tmpfs",MS_NOSUID|MS_NODEV,"mode=0755,size=16m");
        for(const char* path:{"bin","dev","proc","sys","run","tmp","etc"})fs::create_directory(mounts.root/path);
        fs::copy_file(fixture,mounts.root/"bin/bash");chmod((mounts.root/"bin/bash").c_str(),0755);
        const int status=session(mounts);assert(status==19);assert(mounts.cleanup());_exit(0);
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';_exit(1);}}
    std::string output;char buffer[4096];ssize_t size;while((size=::read(master,buffer,sizeof(buffer)))>0)output.append(buffer,size);close(master);
    int result=0;assert(waitpid(test,&result,0)==test);std::cout<<output;
    assert(WIFEXITED(result)&&WEXITSTATUS(result)==0);assert(output.find("UKE_CHROOT_SESSION_FIXTURE_PASS")!=std::string::npos);
    assert(!fs::exists("/run/uke-chroot/root"));
    std::cout<<"Actual interactive chroot session with trusted native mock: read-only root, no capabilities, private proc, init protection, writable tmpfs, exit code and cleanup passed; no target distribution code or block devices\n";
}
