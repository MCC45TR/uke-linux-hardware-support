// SPDX-License-Identifier: MIT
// Signed offline solver fixtures. No RPM transaction or device action is run.
#define UKE_PACKAGES_TEST
#include "../src/packages.cpp"
#include <cassert>

using namespace uke::packages;
int main(int argc,char** argv) {
    assert(argc==4);const fs::path fixture=argv[1],root=fixture/"root",repository=argv[2];
    const std::string scenario=argv[3];
    libdnf5::Base base;auto& config=base.get_config();
    config.get_installroot_option().set(root.string());config.get_use_host_config_option().set(false);
    config.get_config_file_path_option().set((fixture/"empty.conf").string());
    config.get_reposdir_option().set(std::vector<std::string>{});config.get_plugins_option().set(false);
    config.get_cachedir_option().set((fixture/"cache").string());
    config.get_allow_vendor_change_option().set(false);config.get_pkg_gpgcheck_option().set(true);
    retain_kernels(base);base.get_vars()->set("releasever","46");base.setup();
    const auto add_repo=[&](const std::string& id,const fs::path& path) {
        auto repo=base.get_repo_sack()->create_repo(id);
        repo->get_config().get_baseurl_option().set(std::vector<std::string>{"file://"+path.string()});
        repo->get_config().get_pkg_gpgcheck_option().set(true);return repo;
    };
    const auto target=add_repo(prefix+project("stable"),repository);
    add_repo("foreign",fixture/"foreign");
    if(scenario=="locked-kernel")target->get_config().get_excludepkgs_option().set(std::vector<std::string>{kernel+"*"});
    base.get_repo_sack()->load_repos();
    const std::set<std::string> ids{prefix+project("stable")};
    if(scenario=="exact-sync"||scenario=="incompatible-manager") {
        bool refused=false;
        try{plan(base,"stable",ids,{},scenario=="exact-sync");}
        catch(const std::exception& error){std::cout<<error.what()<<'\n';refused=true;}
        assert(refused);return 0;
    }
    const auto selected=plan(base,"stable",ids,{},false);
    assert(!selected.synchronize&&selected.retained_keys.contains("uke-extra.noarch"));
    assert(selected.retained_keys.contains("uke-control.noarch")&&selected.retained_keys.contains("uke-shared.noarch"));
    assert(selected.retained.at("uke-extra-1-1.fc46.noarch")=="not_available_in_channel");
    assert(selected.retained.at("uke-control-3-1.fc46.noarch")=="newer_installed");
    assert(selected.krel.empty()&&selected.selected_krel=="2.0-fixture");
    assert(selected.kernel_retention_reason==(scenario=="locked-kernel"?"no_admitted_candidate":"installed_kernel_current_or_newer"));
    if(scenario=="dependency-conflict") {
        bool refused=false;try{resolve(base,selected,"stable",true);}catch(const std::exception& error){std::cout<<error.what()<<'\n';refused=true;}
        assert(refused);return 0;
    }
    auto transaction=resolve(base,selected,"stable",scenario!="channel-set");
    unsigned inbound=0;
    for(const auto& item:transaction.get_transaction_packages()) {
        const auto& package=item.get_package();
        assert(package.get_name()=="uke-app");
        assert(item.get_action()!=Action::DOWNGRADE&&item.get_action()!=Action::REMOVE);
        if(libdnf5::transaction::transaction_item_action_is_inbound(item.get_action())) {
            assert(package.get_nevra()=="uke-app-2-1.fc46.noarch");++inbound;
        }
    }
    assert(inbound==1);transaction.download();
    assert(transaction.check_gpg_signatures()==(scenario!="unsigned"));
    std::cout<<"Signed package policy passed: "<<scenario<<'\n';
}
