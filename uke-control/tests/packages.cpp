// SPDX-License-Identifier: MIT
#define UKE_PACKAGES_TEST
#include "../src/packages.cpp"
#include <cassert>
#include <fnmatch.h>
using namespace uke::packages;
static Options options(std::initializer_list<const char*> input) {
    std::vector<char*> args;for(auto arg:input)args.push_back(const_cast<char*>(arg));
    return parse(int(args.size()),args.data());
}
int main() {
    assert(options({"brunch","stable"}).selected=="stable");assert(options({"branch","test","--dry-run"}).dry);
    assert(options({"brunch","unstable","--yes"}).yes);assert(options({"brunch","sync"}).sync);
    assert(options({"update"}).update);assert(options({"update","--check"}).dry);
    for(auto args:{std::initializer_list<const char*>{"brunch"},{"brunch","bad"},{"brunch","stable","--nodeps"},{"brunch","sync","--yes","--yes"},{"brunch","status","--yes"},{"update","--allowerasing"}}) {
        bool rejected=false;try{options(args);}catch(const std::exception&){rejected=true;}assert(rejected);
    }
    assert(project("unstable")=="uke-linux-test"&&project("stable")=="uke-linux");
    assert(managed_repo(prefix+"uke-linux-test:custom:experiment"));assert(!managed_repo(prefix+"unrelated"));
    for(const auto& arch:{"aarch64","noarch","x86_64"}) {
        const auto stable=kernel+"-core-7.2.11-1.fc46."+arch;
        const auto rc=kernel+"-core-7.4~rc7-1.fc46."+arch;
        assert(fnmatch(prerelease_excludes[0].c_str(),stable.c_str(),0)==FNM_NOMATCH);
        assert(fnmatch(prerelease_excludes[0].c_str(),rc.c_str(),0)==0);
    }
    bool denied=false;try{atomic_write("/definitely-missing-parent/record","test");}catch(const std::exception&){denied=true;}assert(denied);
    if(geteuid()==0) {
        char dir[]="/tmp/uke-channel-state-XXXXXX";assert(mkdtemp(dir));const fs::path root=dir;
        auto rejected=[](auto&& fn){bool caught=false;try{fn();}catch(const std::exception&){caught=true;}assert(caught);};
        atomic_write(root/"channel","test\n");assert(saved(root/"channel")=="test");
        atomic_write(root/"channel","stable\n");assert(saved(root/"channel")=="stable");
        fs::create_symlink(root/"channel",root/"symlink");rejected([&]{atomic_write(root/"symlink","bad");});
        fs::create_hard_link(root/"channel",root/"hardlink");rejected([&]{atomic_write(root/"channel","bad");});fs::remove(root/"hardlink");
        atomic_write(root/"pending","unstable\n",0600);assert(saved(root/"pending")=="unstable");
        std::ofstream(root/"channel.new")<<"interrupted";rejected([&]{atomic_write(root/"channel","bad");});
        std::ofstream(root/"invalid")<<"stable\nunstable\n";rejected([&]{saved(root/"invalid");});
        assert(read_file(root/"channel")=="stable\n");fs::remove_all(root);
    }
}
