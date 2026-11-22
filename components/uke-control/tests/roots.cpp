// SPDX-License-Identifier: MIT
// Actual read-only inspection, synthetic lsblk and a disposable mounted directory.
#include "../src/overview.cpp"
#include <cassert>
#include <fstream>
namespace { Json::Value fixture_inventory; }
namespace uke {
ProcessResult execute_bounded(const std::vector<std::string>& args,unsigned seconds,size_t limit) {
    assert(args[0]=="/usr/bin/lsblk"&&seconds==5&&limit==1024*1024);
    ProcessResult result;result.status=0;Json::StreamWriterBuilder writer;result.output=Json::writeString(writer,fixture_inventory);return result;
}
}
template<class F>void refused(F&& call){bool failed=false;try{call();}catch(const std::exception&){failed=true;}assert(failed);}
int main() {
    namespace fs=std::filesystem;using namespace uke::overview;
    char pattern[]="/tmp/uke-roots-XXXXXX";assert(mkdtemp(pattern));const fs::path root=pattern;
    fs::create_directories(root/"etc");fs::create_directories(root/"usr/lib");fs::create_directories(root/"bin");
    std::ofstream(root/"usr/lib/os-release")<<"NAME=Fixture\n";
    fs::create_symlink("/usr/lib/os-release",root/"etc/os-release");
    std::string elf(64,'\0');elf.replace(0,4,"\177ELF");elf[4]=2;elf[5]=1;elf[18]=char(183);std::ofstream(root/"bin/sh",std::ios::binary)<<elf;
    struct stat st{};assert(!stat(root.c_str(),&st));
    Json::Value target;target["type"]="part";target["partuuid"]="abcd-1234";target["maj:min"]=std::to_string(major(st.st_dev))+":"+std::to_string(minor(st.st_dev));
    target["mountpoints"]=Json::Value(Json::arrayValue);target["mountpoints"].append(root.string());
    const auto set=[&]{fixture_inventory["blockdevices"]=Json::Value(Json::arrayValue);fixture_inventory["blockdevices"].append(target);};set();
    auto inspected=inspect_root("abcd-1234");assert(inspected["os_release"]=="NAME=Fixture\n"&&inspected["shell_architecture"]=="aarch64");
    assert(inspected["mounted_by_command"]==false&&inspected["target_code_executed"]==false);
    std::ofstream(root/"usr/lib/os-release",std::ios::binary)<<"NAME=Bad\xff\n";
    assert(inspect_root("abcd-1234")["os_release"].asString()=="NAME=Bad\xef\xbf\xbd\n");
    std::ofstream(root/"usr/lib/os-release")<<"NAME=Fixture\n";
    fs::remove(root/"etc/os-release");fs::create_symlink("../../../../etc/passwd",root/"etc/os-release");
    // Absolute/parent symlinks resolve inside the selected root; host files cannot escape into the result.
    assert(inspect_root("abcd-1234")["os_release"]=="NAME=Fixture\n");
    fs::remove(root/"bin/sh");assert(!mkfifo((root/"bin/sh").c_str(),0600));refused([&]{inspect_root("abcd-1234");});fs::remove(root/"bin/sh");
    target["maj:min"]="999:999";set();refused([&]{inspect_root("abcd-1234");});
    target["mountpoints"]=Json::Value(Json::arrayValue);set();assert(inspect_root("abcd-1234")["inspection"]=="not_mounted");
    target["type"]="disk";set();refused([&]{inspect_root("abcd-1234");});
    target["type"]="part";set();fixture_inventory["blockdevices"].append(target);refused([&]{inspect_root("abcd-1234");});
    refused([&]{inspect_root("../../etc/passwd");});fs::remove_all(root);
}
