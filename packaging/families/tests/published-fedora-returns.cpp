// SPDX-License-Identifier: MIT
// COPR source worker only; actual published RPMs, no downloaded payload execution.
#define UKE_PACKAGES_TEST
#include "../../../uke-control/src/packages.cpp"
#include <cassert>
using namespace uke::packages;
int main(int argc,char** argv) {
    assert(argc==5);const fs::path fixture=argv[1],root=argv[2];const std::string mode=argv[3],selection=argv[4];
    const bool reverse=mode=="reverse";const std::string selected=reverse?"test":"unstable";
    libdnf5::Base base;auto& c=base.get_config();
    c.get_installroot_option().set(root.string());c.get_use_host_config_option().set(false);c.get_plugins_option().set(false);
    c.get_reposdir_option().set(std::vector<std::string>{});c.get_config_file_path_option().set((fixture/"empty.conf").string());
    c.get_cachedir_option().set((fixture/"cache").string());c.get_optional_metadata_types_option().set(std::set<std::string>{"filelists"});
    c.get_install_weak_deps_option().set(mode!="weak-off");c.get_best_option().set(true);c.get_allow_vendor_change_option().set(false);
    retain_kernels(base);base.get_vars()->set("releasever","46");base.get_vars()->set("basearch","aarch64");base.setup();
    const auto target=prefix+project(selected);
    for(const auto& id:{target,std::string("fedora"),std::string("rawhide"),std::string("uke-dependencies")}) {
        auto repo=base.get_repo_sack()->create_repo(id);auto& config=repo->get_config();
        if(id=="rawhide")config.get_metalink_option().set("https://mirrors.fedoraproject.org/metalink?repo=rawhide&arch=aarch64");
        else if(id=="uke-dependencies")config.get_baseurl_option().set(std::vector<std::string>{"https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/"});
        else config.get_baseurl_option().set(std::vector<std::string>{"file://"+(fixture/(id=="fedora"?"originals":reverse?"baseline":"candidate")).string()});
        config.get_skip_if_unavailable_option().set(false);config.get_metadata_expire_option().set(3600);
        config.get_priority_option().set(id==target||id=="fedora"?1:99);config.get_pkg_gpgcheck_option().set(true);
    }
    need(base.lock_system_repo(libdnf5::utils::LockAccess::WRITE,libdnf5::utils::LockBlocking::NON_BLOCKING),"Fixture lock failed");
    base.get_repo_sack()->load_repos();auto p=plan(base,selected,{target},".fc46");
    assert(p.originals==!reverse);assert(p.cxx==!reverse);
    if(mode=="fresh") {
        libdnf5::rpm::PackageQuery available(base);available.filter_available();available.filter_repo_id(target);
        std::set<std::string> metas;std::istringstream names(read_file(fixture/"meta-names"));
        for(std::string name;std::getline(names,name);)metas.insert(name);
        for(const auto& pkg:available)if(pkg.get_name()=="uke-core-meta"||(selection=="kde"&&pkg.get_name()=="uke-kde-plasma-meta")||
            (selection=="all"&&metas.contains(pkg.get_name()))) {p.targets.insert_or_assign(key(pkg),pkg);p.scope.insert(key(pkg));}
    }
    libdnf5::rpm::PackageQuery installed(base);installed.filter_installed();size_t expected=0;
    for(const auto& pkg:installed)if(fedora_return(pkg.get_name()))++expected;
    if(!reverse)assert(p.returns.size()==expected);
    if(mode=="desktop-upgrade")assert(expected==1&&p.returns.size()==1);
    auto tx=resolve(base,p,selected,false);size_t inbound=0,outbound=0;
    bool gnu_original=false,gnu_fork=false;
    for(const auto& item:tx.get_transaction_packages())if(libdnf5::transaction::transaction_item_action_is_inbound(item.get_action())) {
        if(item.get_package().get_name()=="libstdc++")gnu_original=true;
        if(item.get_package().get_name()=="uke-libstdcxx")gnu_fork=true;
    }
    for(const auto& item:tx.get_transaction_packages()) {
        const auto pkg=item.get_package();const auto action=item.get_action();
        if(libdnf5::transaction::transaction_item_action_is_outbound(action)) {
            ++outbound;assert(!kernel_package(pkg.get_name())&&!private_package(pkg.get_name()));
            if(pkg.get_name()=="uke-libstdcxx")assert(gnu_original&&!reverse);
        }
        if(libdnf5::transaction::transaction_item_action_is_inbound(action)) {
            ++inbound;assert(!kernel_package(pkg.get_name()));if(!reverse)assert(!fedora_return(pkg.get_name()));
            if(pkg.get_name()=="libstdc++")assert(p.cxx&&original_fedora(pkg,fedora_returns.back(),"aarch64"));
        }
    }
    assert(reverse?gnu_fork:gnu_original);
    if(reverse)for(const auto& entry:fedora_returns)
        assert(p.targets.contains(std::string(entry.fork)+(std::string_view(entry.original)=="libwacom-data"?".noarch":".aarch64")));
    std::cout<<"UKE_PUBLISHED_FEDORA_RETURN mode="<<mode<<" selection="<<selection<<" returns="<<p.returns.size()<<" inbound="<<inbound<<" outbound="<<outbound
        <<" console_gnu_original="<<(!reverse)<<" boot_power_callers=true rpm_transaction_executed=false hardware_tested=false\n";
}
