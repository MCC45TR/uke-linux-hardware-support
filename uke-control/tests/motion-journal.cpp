// SPDX-License-Identifier: MIT
#include "motion-journal.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#ifdef NDEBUG
#error Assertions are required
#endif
using namespace uke::motion;
namespace fs=std::filesystem;
int main(){
    char pattern[]="/tmp/uke-motion-journal-XXXXXX";const char* root=mkdtemp(pattern);assert(root);
    const fs::path dir(root),file=dir/"ownership.json",temporary=dir/"ownership.json.new";
    Journal journal(file,geteuid());
    assert(journal.load().status==Journal::Status::missing&&journal.discard_pending());
    assert(journal.save("first"));assert(journal.load().status==Journal::Status::valid&&journal.load().text=="first");
    assert(!journal.save("second"));assert(journal.load().text=="first"&&!fs::exists(temporary));
    assert(journal.clear()&&journal.clear());
    // Crashes before the atomic rename have not changed hardware configuration.
    {std::ofstream pending(temporary);pending<<"partial";}assert(chmod(temporary.c_str(),0600)==0);
    assert(!journal.save("blocked"));assert(journal.discard_pending()&&!fs::exists(temporary));assert(journal.save("recovered"));
    assert(chmod(file.c_str(),0644)==0);assert(journal.load().status==Journal::Status::invalid&&!journal.clear());assert(chmod(file.c_str(),0600)==0);
    assert(link(file.c_str(),(dir/"linked").c_str())==0);assert(journal.load().status==Journal::Status::invalid&&!journal.clear());assert(unlink((dir/"linked").c_str())==0);
    assert(journal.clear());
    {std::ofstream outside(dir/"outside");outside<<"outside";}
    assert(symlink("outside",file.c_str())==0);assert(journal.load().status==Journal::Status::invalid&&!journal.clear()&&!journal.save("new"));
    assert(unlink(file.c_str())==0);assert(symlink("outside",temporary.c_str())==0);assert(!journal.discard_pending()&&!journal.save("new"));assert(unlink(temporary.c_str())==0);
    assert(journal.save("owner"));Journal wrong_owner(file,geteuid()+1);assert(wrong_owner.load().status==Journal::Status::invalid&&!wrong_owner.clear());
    assert(chmod(dir.c_str(),0755)==0);assert(journal.load().status==Journal::Status::invalid&&!journal.clear()&&!journal.save("new"));assert(chmod(dir.c_str(),0700)==0);
    assert(journal.clear());assert(!journal.save("")&&!journal.save(std::string(16385,'x')));
    assert(journal.save(std::string(16384,'x'))&&journal.load().text.size()==16384);assert(journal.clear());
    assert(fs::remove_all(dir)==2);
    std::cout<<"Actual journal files: atomic no-replace, precommit cleanup, owner/mode/hardlink/symlink refusal and bounded input passed; private temporary files only\n";
}
