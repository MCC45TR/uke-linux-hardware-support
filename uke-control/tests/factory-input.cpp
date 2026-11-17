// SPDX-License-Identifier: MIT
// COPR only: execute each actual reader with protected temporary inputs.
#ifdef UKE_FACTORY_COLOR
#define UKE_LOCAL_FW_TEST
#include UKE_FACTORY_BODY
#define UKE_FACTORY_MAIN __wrap_main
#else
#define UKE_KERNEL_UPDATE_TEST
#include UKE_FACTORY_BODY
#define UKE_FACTORY_MAIN main
#endif
#include <cassert>
#include <functional>

static void refused(const std::function<void()> &call) {
    bool failed = false;
    try { call(); } catch (const std::exception &) { failed = true; }
    assert(failed);
}

#ifdef UKE_FACTORY_COLOR
extern "C"
#endif
int UKE_FACTORY_MAIN(int argc, char **argv) {
    assert(argc == 2 && geteuid() != 0);
    const fs::path source = argv[1];
    char pattern[] = "/tmp/uke-factory-input-XXXXXX";
    char *created = mkdtemp(pattern);
    assert(created);
    const fs::path root = created, input = root / "source";
    struct Cleanup { fs::path path; ~Cleanup() { fs::remove_all(path); } } cleanup{root};
#ifdef UKE_FACTORY_COLOR
    auto admit = [](const Bytes &bytes) {
        return digest(bytes) == o82_stock::input_hash(0x42);
    };
#else
    auto admit = [](const Bytes &bytes) { return uke_video::admitted(bytes); };
#endif
    Bytes output{17,23}, sentinel = output;
    assert(!uke_factory::read(input,admit,output) && output == sentinel);
    fs::copy_file(source,input);
    assert(!chmod(input.c_str(),0600));
    assert(uke_factory::read(input,admit,output));
    assert(output.size() == fs::file_size(source));
    const auto original = output;
    output = sentinel;
    refused([&] { uke_factory::read(input,[](const Bytes &) { return false; },output); });
    assert(output == sentinel);
    refused([&] { uke_factory::read(input,[](const Bytes &) -> bool { throw std::runtime_error("admission error"); },output); });
    assert(output == sentinel);
    // The fixture owner differs from the installed, root-owned factory input.
    refused([&] { uke_factory::read(source,admit,output); });
    assert(output == sentinel);
    assert(!chmod(input.c_str(),0620));
    refused([&] { uke_factory::read(input,admit,output); });
    assert(output == sentinel && !chmod(input.c_str(),0600));
    fs::create_hard_link(input,root / "hard");
    refused([&] { uke_factory::read(input,admit,output); });
    assert(output == sentinel);fs::remove(root / "hard");
    fs::rename(input,root / "original");
    fs::create_symlink(root / "original",input);
    refused([&] { uke_factory::read(input,admit,output); });
    assert(output == sentinel);fs::remove(input);
    fs::create_symlink(root / "missing",input);
    refused([&] { uke_factory::read(input,admit,output); });
    assert(output == sentinel);fs::remove(input);
    fs::create_directory(input);
    refused([&] { uke_factory::read(input,admit,output); });
    assert(output == sentinel);fs::remove(input);
    fs::rename(root / "original",input);
    int fd=open(input.c_str(),O_WRONLY|O_CLOEXEC);
    assert(fd>=0);
    const unsigned char changed=original[0]^1;
    assert(pwrite(fd,&changed,1,0)==1 && !close(fd));
    refused([&] { uke_factory::read(input,admit,output); });
    assert(output == sentinel);
    assert(!truncate(input.c_str(),1));
    refused([&] { uke_factory::read(input,admit,output); });
    assert(output == sentinel);
    assert(!chmod(root.c_str(),0720));
    refused([&] { uke_factory::read(input,[](const Bytes &) { return true; },output); });
    assert(output == sentinel && !chmod(root.c_str(),0700));
    std::cout << "PASS: actual factory input: protected owner/ancestor/mode, hardlink, symlink, missing, directory, altered, truncated, rejected and throwing admission; unchanged failed output; no device access\n";
    return 0;
}
