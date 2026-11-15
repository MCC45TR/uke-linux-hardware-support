// SPDX-License-Identifier: MIT
// Included after the original contract fixture's descriptor/write helpers.
static void check_factory_mcu() {
    Fd source(open("/usr/lib/firmware/qcom/uke/nanosic/MCU_Upgrade.bin",O_RDONLY|O_CLOEXEC|O_NOFOLLOW));
    struct stat st{};
    assert(!fstat(source.value,&st)&&S_ISREG(st.st_mode)&&st.st_size==82740&&st.st_uid==0&&!(st.st_mode&0022)&&st.st_nlink==1);
    const auto expected=read_fd(source.value,st.st_size);
    assert(admit_mcu(expected));
    char pattern[]="/tmp/uke-factory-mcu-XXXXXXXX";
    const char *created=mkdtemp(pattern);assert(created);
    const fs::path dir=created,path=dir/"MCU_Upgrade.bin";
    assert(!factory_mcu(path));
    {
        Fd out(open(path.c_str(),O_RDWR|O_CREAT|O_EXCL|O_CLOEXEC,0600));
        write_image(out.value,expected);
        assert(factory_mcu(path)==expected);
        auto corrupt=expected;corrupt[0]^=1;
        write_image(out.value,corrupt);refused([&]{factory_mcu(path);});
        write_image(out.value,expected);assert(!ftruncate(out.value,82739));
        refused([&]{factory_mcu(path);});
        assert(!ftruncate(out.value,82741));refused([&]{factory_mcu(path);});
        assert(!ftruncate(out.value,0));refused([&]{factory_mcu(path);});
        write_image(out.value,expected);
        assert(!fchmod(out.value,0666));refused([&]{factory_mcu(path);});
        assert(!fchmod(out.value,0600));
        fs::create_hard_link(path,dir/"alias");refused([&]{factory_mcu(path);});
        fs::remove(dir/"alias");assert(factory_mcu(path)==expected);
    }
    fs::create_symlink(path,dir/"link");refused([&]{factory_mcu(dir/"link");});
    fs::create_symlink(dir/"absent",dir/"dangling");refused([&]{factory_mcu(dir/"dangling");});
    fs::create_directory_symlink(dir,dir/"parent-link");refused([&]{factory_mcu(dir/"parent-link/MCU_Upgrade.bin");});
    fs::create_directory(dir/"directory");refused([&]{factory_mcu(dir/"directory");});
    assert(!chmod(dir.c_str(),0777));refused([&]{factory_mcu(path);});
    assert(!chmod(dir.c_str(),0700));assert(factory_mcu(path)==expected);
    fs::remove_all(dir);
    std::cout<<"PASS: actual exact-hash factory MCU reader, missing-only fallback, corrupt/short/oversize/empty/mode/hardlink/symlink/dangling/parent/directory refusals; no controller or stock partition access\n";
}
