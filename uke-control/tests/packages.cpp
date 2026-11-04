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
// configure() deliberately checks the real host release. This optional fixture
// must not turn an unsupported build host into a runtime-admission test failure.
static bool rawhide_repository_fixture_host(const std::string& release) {
    try{return fedora_target(release)=="rawhide";}
    catch(const std::runtime_error&){return false;}
}
int main() {
    {
        char pattern[]="/tmp/uke-package-pipe-XXXXXX";assert(mkdtemp(pattern));
        const fs::path root=pattern,receipt=root/"receipt.json";
        const std::string payload="{\"payload\":\""+std::string(20000,'x')+"\"}";
        std::cout.flush();const auto child=fork();assert(child>=0);
        if(child==0) {
            ignore_broken_pipes();
            const auto output=open(receipt.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
            assert(output>=0&&dup2(output,STDOUT_FILENO)>=0);close(output);
            {
                CriticalSignals critical;
                int descriptors[2];assert(pipe(descriptors)==0);close(descriptors[0]);
                assert(write(descriptors[1],"x",1)==-1&&errno==EPIPE);close(descriptors[1]);
                uke::cli::print(payload,true,"Receipt");
            }
            // Deliberately bypass global iostream destruction: the coordinator
            // must flush its complete receipt before restoring deferred signals.
            _exit(0);
        }
        int status=0;assert(waitpid(child,&status,0)==child&&WIFEXITED(status)&&WEXITSTATUS(status)==0);
        assert(read_file(receipt)==payload+'\n');fs::remove_all(root);
    }
    const std::string container_release="ID=fedora\nVERSION_ID=46\nVERSION=\"46 (Container Image Prerelease)\"\n";
    bool container_refused=false;try{fedora_target(container_release);}catch(const std::runtime_error&){container_refused=true;}
    assert(container_refused);
    assert(!rawhide_repository_fixture_host(container_release));
    assert(!rawhide_repository_fixture_host("ID=fedora\nVERSION_ID=45\n"));
    assert(rawhide_repository_fixture_host("ID=fedora\nVERSION_ID=46\nVERSION=\"46 (Rawhide Prerelease)\"\n"));
    {
        char pattern[]="/tmp/uke-kernel-transaction-XXXXXX";assert(mkdtemp(pattern));
        const fs::path root=pattern,helper=root/"updater",arguments=root/"arguments";
        std::ofstream(helper)<<"#!/bin/sh\nprintf '%s\\n' \"$@\" > '"<<arguments.string()<<"'\n";
        assert(chmod(helper.c_str(),0700)==0);
        // Exercise the real fork/exec/wait path with a COPR-only helper, without boot or policy access.
        for(const bool json:{false,true}) {
            boot_kernel("7.3.0-60",json,helper);
            assert(read_file(arguments)=="transaction\n/usr/lib/senemos/kernel-updates/7.3.0-60\n");
        }
        const auto rejected=[&](const fs::path& path){
            bool caught=false;try{boot_kernel("7.3.0-60",false,path);}catch(const std::exception&){caught=true;}assert(caught);
        };
        assert(chmod(helper.c_str(),0777)==0);rejected(helper);
        assert(chmod(helper.c_str(),0700)==0);
        fs::create_symlink(helper,root/"link");rejected(root/"link");rejected(root/"missing");
        std::ofstream(helper)<<"#!/bin/sh\nexit 23\n";
        rejected(helper);
        fs::remove_all(root);
    }
    {
        libdnf5::Base base;auto& c=base.get_config();
        c.get_installonlypkgs_option().set(std::vector<std::string>{"","custom-kernel","installonlypkg(kernel)"});
        c.get_installonly_limit_option().set(2);c.get_protect_running_kernel_option().set(false);
        retain_kernels(base);retain_kernels(base);
        assert((c.get_installonlypkgs_option().get_value()==std::vector<std::string>{"custom-kernel","installonlypkg(kernel)",kernel}));
        assert(c.get_installonly_limit_option().get_value()==0&&c.get_protect_running_kernel_option().get_value());
    }
    if(rawhide_repository_fixture_host(read_file("/etc/os-release"))) {
        char pattern[]="/tmp/uke-channel-repository-XXXXXX";assert(mkdtemp(pattern));const fs::path root=pattern;
        {
            libdnf5::Base base;base.get_config().get_installroot_option().set((root/"root").string());
            base.get_config().get_cachedir_option().set((root/"cache").string());base.get_config().get_plugins_option().set(false);base.setup();
            auto repo=base.get_repo_sack()->create_repo(prefix+project("unstable"));
            repo->get_config().get_excludepkgs_option().set(std::vector<std::string>{"custom-block",prerelease_excludes[0]});
            const auto configured=configure(base,"unstable");
            assert(infer_channel(enabled_managed_repos(base))=="unstable");
            const auto& exclusions=repo->get_config().get_excludepkgs_option().get_value();
            assert(std::find(exclusions.begin(),exclusions.end(),"custom-block")!=exclusions.end());
            assert(std::find(exclusions.begin(),exclusions.end(),prerelease_excludes[0])==exclusions.end());
            // Read the persisted value through the real append-option parser too.
            libdnf5::OptionStringAppendList persisted(std::vector<std::string>{"custom-block",prerelease_excludes[0]});
            std::istringstream text(configured.overrides);std::string line;bool selected=false,checked=false;
            while(std::getline(text,line)) {
                if(line.starts_with('['))selected=line=='['+prefix+project("unstable")+']';
                if(selected&&line.starts_with("excludepkgs=")){persisted.set(priority,line.substr(12));checked=true;}
            }
            assert(checked&&persisted.get_value()==std::vector<std::string>{"custom-block"});
        }
        fs::remove_all(root);
    } else std::cerr<<"Skipping optional Rawhide repository fixture: host is not an admitted Rawhide target.\n";
    assert(options({"brunch","stable"}).selected=="stable");assert(options({"branch","test","--dry-run"}).dry);
    assert(options({"brunch","unstable","--yes"}).yes);assert(options({"brunch","sync"}).sync);
    assert(options({"update"}).update);assert(options({"update","--check"}).dry);
    assert(options({"update","status"}).boot_status);
    assert(!options({"update","status"}).update);
    assert(options({"update","status","--json"}).json);
    for(auto input:{std::initializer_list<const char*>{"update","status","--yes"},{"update","status","--check"},{"update","status","--dry-run"},{"update","status","bad"}}) {
        bool refused=false;try{options(input);}catch(const uke::cli::UsageError&){refused=true;}assert(refused);
    }
    assert(options({"channel","set","stable","--json","--yes"}).json);
    assert(options({"channel","status","--json"}).status);
    assert(options({"channel","set","test","--dry-run","--json"}).dry);
    assert(options({"update","--check","--json"}).json);
    for(auto input:{std::initializer_list<const char*>{"channel","set"},{"channel","set","bad"},{"channel","set","stable","--json"},{"channel","sync","--json"},{"update","--json"},{"channel","status","--json","--json"}}) {
        bool rejected=false;try{options(input);}catch(const uke::cli::UsageError&){rejected=true;}assert(rejected);
    }
    for(auto args:{std::initializer_list<const char*>{"brunch"},{"brunch","bad"},{"brunch","stable","--nodeps"},{"brunch","sync","--yes","--yes"},{"brunch","status","--yes"},{"update","--allowerasing"}}) {
        bool rejected=false;try{options(args);}catch(const std::exception&){rejected=true;}assert(rejected);
    }
    assert(project("unstable")=="uke-linux-unstable"&&project("stable")=="uke-linux"&&project("test")=="uke-linux-test");
    assert(signing_key(project("unstable"))=="2204A661EA630A5303500085538797E72A3E4ECA");
    assert(signing_key(project("unstable"))!=signing_key(project("test")));
    bool unknown_key=false;try{signing_key("unrelated");}catch(const std::exception&){unknown_key=true;}assert(unknown_key);
    assert(private_package("uke-local-touch-data"));
    assert(private_package("uke-video-firmware-local"));
    assert(!private_package("uke-video-firmware-local-extra"));
    assert(!private_package("uke-local-sensor-data")&&!private_package("uke-local-touch-data-extra"));
    assert(kernel_userspace("kernel-headers")&&kernel_userspace("kernel-tools"));
    assert(!kernel_userspace("kernel-core")&&!kernel_userspace("kernel-modules"));
    assert(managed_repo(prefix+"uke-linux-test:custom:experiment"));assert(!managed_repo(prefix+"unrelated"));
    assert(infer_channel({prefix+"uke-linux-test"})=="test");
    assert(infer_channel({"uke-linux"})=="stable");
    assert(infer_channel({prefix+"uke-linux-unstable"})=="unstable");
    assert(infer_channel({prefix+"uke-linux-test",prefix+"uke-linux-unstable"}).empty());
    assert(managed_repo(prefix+"uke-linux-unstable:custom:experiment"));
    assert(managed_source("unstable-alias",{"https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/"}));
    assert(!managed_source("unrelated",{"https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable-fake/"}));
    assert(infer_channel({}).empty());
    assert(infer_channel({prefix+"uke-linux",prefix+"uke-linux-test"}).empty());
    assert(infer_channel({prefix+"uke-linux-test:custom:experiment"}).empty());
    assert(infer_channel({"uke-linux",prefix+"uke-linux"}).empty());
    assert(managed_source("legacy-candidate",{"https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-test/custom/"}));
    assert(source_channel("uke-selected",{"https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-$basearch/"})=="unstable");
    assert(source_channel("alias",{"https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-test/custom/"}).empty());
    assert(source_channel("alias",{"https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-test-fake/fedora-rawhide-aarch64/"}).empty());
    assert(source_channel("alias",{"https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-test/fedora-rawhide-aarch64/","https://example.invalid/"}).empty());
    assert(source_channel("alias",{"https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux/fedora-rawhide-aarch64/","https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-test/fedora-rawhide-aarch64/"}).empty());
    {
        libdnf5::Base base;base.get_config().get_plugins_option().set(false);base.setup();
        const auto add=[&](const std::string& name,const std::string& selection) {
            auto repo=base.get_repo_sack()->create_repo(name);
            repo->get_config().get_baseurl_option().set(std::vector<std::string>{"https://download.copr.fedorainfracloud.org/results/mcc45tr/"+project(selection)+"/fedora-rawhide-aarch64/"});
            return repo;
        };
        add("uke-selected","unstable");add("uke-policy","unstable");
        assert(infer_channel(base)=="unstable");
        auto other=add("different-channel","test");assert(infer_channel(base).empty());
        other->get_config().get_enabled_option().set(false);assert(infer_channel(base)=="unstable");
        auto custom=base.get_repo_sack()->create_repo("custom");
        custom->get_config().get_baseurl_option().set(std::vector<std::string>{"https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-test/custom/"});
        assert(infer_channel(base).empty());
    }
    {
        std::ostringstream output,errors;auto* old_output=std::cout.rdbuf(output.rdbuf());auto* old_errors=std::cerr.rdbuf(errors.rdbuf());
        Downloads downloads;auto* transfer=downloads.add_new_download(nullptr,"fixture-package",1024);
        assert(downloads.progress(transfer,1024,512)==0);
        assert(downloads.end(transfer,Downloads::TransferStatus::SUCCESSFUL,nullptr)==0);
        std::cout.rdbuf(old_output);std::cerr.rdbuf(old_errors);
        assert(output.str().empty()&&errors.str().find("Downloaded fixture-package")!=std::string::npos);
        assert(std::string(operation::next("complete"))=="none");
    }
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
        const auto helper=root/"updater";
        assert(!boot_helper_ready(helper));
        std::ofstream(helper)<<"#!/bin/sh\nexit 0\n";assert(!chmod(helper.c_str(),0755));assert(boot_helper_ready(helper));
        assert(!chmod(helper.c_str(),0775));assert(!boot_helper_ready(helper));assert(!chmod(helper.c_str(),0755));
        fs::create_symlink(helper,root/"updater-link");assert(!boot_helper_ready(root/"updater-link"));
        fs::create_hard_link(helper,root/"updater-hardlink");assert(!boot_helper_ready(helper));fs::remove(root/"updater-hardlink");
        const auto previous_mask=umask(0077);
        atomic_write(root/"channel","test\n");assert(saved(root/"channel")=="test");
        struct stat mode{};assert(stat((root/"channel").c_str(),&mode)==0&&(mode.st_mode&0777)==0644);
        umask(previous_mask);
        atomic_write(root/"channel","stable\n");assert(saved(root/"channel")=="stable");
        fs::create_symlink(root/"channel",root/"symlink");rejected([&]{atomic_write(root/"symlink","bad");});rejected([&]{saved(root/"symlink");});
        fs::create_hard_link(root/"channel",root/"hardlink");rejected([&]{atomic_write(root/"channel","bad");});rejected([&]{saved(root/"channel");});fs::remove(root/"hardlink");
        atomic_write(root/"pending","unstable\n",0600);assert(saved(root/"pending")=="unstable");
        std::ofstream(root/"channel.new")<<"interrupted";rejected([&]{atomic_write(root/"channel","bad");});
        std::ofstream(root/"invalid")<<"stable\nunstable\n";rejected([&]{saved(root/"invalid");});
        std::ofstream(root/"empty");rejected([&]{saved(root/"empty");});
        std::ofstream(root/"oversized")<<std::string(65,'x');rejected([&]{saved(root/"oversized");});
        assert(mkfifo((root/"fifo").c_str(),0600)==0);rejected([&]{saved(root/"fifo");});
        assert(saved(root/"absent").empty());
        {
            libdnf5::Base base;base.get_config().get_installroot_option().set((root/"repo-root").string());
            base.get_config().get_cachedir_option().set((root/"cache").string());base.get_config().get_plugins_option().set(false);base.setup();
            auto stable=base.get_repo_sack()->create_repo(prefix+"uke-linux"),test=base.get_repo_sack()->create_repo(prefix+"uke-linux-test");
            stable->get_config().get_enabled_option().set(false);test->get_config().get_enabled_option().set(true);
            assert(infer_channel(enabled_managed_repos(base))=="test");
            stable->get_config().get_enabled_option().set(true);assert(infer_channel(enabled_managed_repos(base)).empty());
            stable->get_config().get_enabled_option().set(false);test->get_config().get_enabled_option().set(false);
            auto alias=base.get_repo_sack()->create_repo("legacy-candidate");
            alias->get_config().get_baseurl_option().set(std::vector<std::string>{"https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-test/custom/"});
            alias->get_config().get_enabled_option().set(true);assert(infer_channel(enabled_managed_repos(base)).empty());
        }
        assert(read_file(root/"channel")=="stable\n");fs::remove_all(root);
    }
}
