// SPDX-License-Identifier: MIT
// COPR source worker: real signed inputs, disposable RPM databases, no transactions.
#define UKE_PACKAGES_TEST
#include "../src/packages.cpp"
#include <cassert>
using namespace uke::packages;
int main(int argc,char** argv) {
    assert(argc==4);const fs::path fixture=argv[1],root=argv[2];const std::string mode=argv[3];
    const bool reverse=mode=="reverse",weak=mode!="weak-off";
    const std::string selected=reverse?"test":"unstable";
    libdnf5::Base base;auto& config=base.get_config();
    config.get_installroot_option().set(root.string());config.get_use_host_config_option().set(false);
    config.get_plugins_option().set(false);config.get_reposdir_option().set(std::vector<std::string>{});
    config.get_config_file_path_option().set((fixture/"empty.conf").string());
    config.get_cachedir_option().set((fixture/"cache").string());
    config.get_optional_metadata_types_option().set(std::set<std::string>{"filelists"});
    config.get_install_weak_deps_option().set(weak);config.get_best_option().set(true);
    config.get_allow_vendor_change_option().set(false);retain_kernels(base);
    base.get_vars()->set("releasever","46");base.get_vars()->set("basearch","aarch64");base.setup();
    for(const auto& id:{prefix+project(selected),std::string("fedora"),std::string("rawhide")}) {
        auto repo=base.get_repo_sack()->create_repo(id);auto& c=repo->get_config();
        if(id=="rawhide")c.get_metalink_option().set("https://mirrors.fedoraproject.org/metalink?repo=rawhide&arch=aarch64");
        else c.get_baseurl_option().set(std::vector<std::string>{"file://"+(fixture/(id=="fedora"?"originals":reverse?"forks":mode=="desktop-only"?"candidate-desktop":"candidate")).string()});
        c.get_metadata_expire_option().set(3600);c.get_skip_if_unavailable_option().set(false);
        c.get_priority_option().set(id=="rawhide"?99:1);c.get_pkg_gpgcheck_option().set(true);
    }
    need(base.lock_system_repo(libdnf5::utils::LockAccess::WRITE,libdnf5::utils::LockBlocking::NON_BLOCKING),"Fixture lock failed");
    base.get_repo_sack()->load_repos();
    if(mode=="missing") {
        libdnf5::rpm::PackageQuery blocked(base);blocked.filter_available();blocked.filter_name("libstdc++");
        base.get_rpm_package_sack()->add_user_excludes(blocked);
    }
    const std::set<std::string> ids{prefix+project(selected)};
    if(mode=="missing"||mode=="unreviewed"||mode=="legacy-caller") {
        bool refused=false;try{const auto p=plan(base,selected,ids,".fc46");resolve(base,p,selected,false);}
        catch(const std::exception& e){std::cout<<"Expected refusal: "<<e.what()<<'\n';refused=true;}assert(refused);
    } else {
        auto p=plan(base,selected,ids,".fc46");
        assert(p.originals==!reverse);
        assert(p.cxx==(!reverse&&mode!="desktop-only"));
        if(mode=="fresh") {
            libdnf5::rpm::PackageQuery cohort(base);cohort.filter_available();cohort.filter_repo_id(prefix+project(selected));cohort.filter_name("uke-core-meta");
            assert(cohort.size()==1);p.targets.emplace(key(*cohort.begin()),*cohort.begin());p.scope.insert(key(*cohort.begin()));
        }
        libdnf5::rpm::PackageQuery installed(base);installed.filter_installed();
        size_t old_forks=0;for(const auto& pkg:installed)if(fedora_return(pkg.get_name(),p.cxx))++old_forks;
        if(!reverse)assert(p.returns.size()==old_forks);
        auto tx=resolve(base,p,selected,mode=="update");size_t inbound=0;
        for(const auto& item:tx.get_transaction_packages())if(libdnf5::transaction::transaction_item_action_is_inbound(item.get_action())) {
            const auto pkg=item.get_package();++inbound;
            if(!reverse)assert(!fedora_return(pkg.get_name(),p.cxx));
            for(const auto& entry:fedora_returns)if(pkg.get_name()==entry.original&&!reverse&&fedora_original(pkg.get_name(),p.cxx))assert(original_fedora(pkg,entry,pkg.get_arch()));
        }
        if(reverse)for(const auto& entry:fedora_returns)assert(p.targets.contains(std::string(entry.fork)+(std::string(entry.original)=="libwacom-data"?".noarch":".aarch64")));
        if(mode=="fresh")assert(inbound>0);
        if(mode=="desktop-only")assert(p.returns.size()==12&&p.targets.contains("uke-libstdcxx.aarch64"));
        std::cout<<"Inbound packages: "<<inbound<<'\n';
    }
    std::cout<<"UKE_FEDORA_RETURN_SOLVER mode="<<mode<<" real_signed_pairs=13 weak_deps="<<weak<<" rpm_transaction_executed=false hardware_tested=false\n";
}
