// SPDX-License-Identifier: MIT
// Actual libblkid path on a regular disposable image only; never a block device.
#include "../src/chroot.cpp"
#include <cassert>
using namespace uke::chroot_manager;
int main(int argc,char** argv){assert(argc==3);Fd image(::open(argv[1],O_RDONLY|O_CLOEXEC|O_NOFOLLOW));struct stat st{};assert(image.n>=0&&!fstat(image.n,&st)&&S_ISREG(st.st_mode));
    Target target;bool failed=false;try{probe_filesystem(image.n,target);}catch(...){failed=true;}
    if(std::string_view(argv[2])=="absent")assert(failed);else assert(!failed&&target.filesystem==argv[2]&&!target.uuid.empty());
    std::cout<<"Actual read-only filesystem probe on disposable "<<argv[2]<<" image passed\n";
}
