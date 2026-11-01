// SPDX-License-Identifier: MIT
// COPR-only dependency resolution; never runs a package transaction or device code.
#include <libdnf5/base/base.hpp>
#include <libdnf5/base/goal.hpp>
#include <libdnf5/repo/repo_sack.hpp>
#include <libdnf5/rpm/package_query.hpp>
#include <libdnf5/transaction/transaction_item_action.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
namespace fs=std::filesystem;
static void need(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv) try {
    need(argc==5,"Usage: solver FIXTURE ROOT MODE SELECTION");
    const fs::path fixture=fs::canonical(argv[1]),root=fs::canonical(argv[2]);
    need(root.string().starts_with(fixture.string()+"/roots/"),"Require a disposable fixture root");
    const std::string mode=argv[3],selection=argv[4];
    const bool reverse=mode=="rollback",weak=mode!="weak-off";
    libdnf5::Base base;auto& config=base.get_config();
    config.get_installroot_option().set(root.string());
    config.get_use_host_config_option().set(false);config.get_plugins_option().set(false);
    config.get_config_file_path_option().set((fixture/"empty.conf").string());
    config.get_reposdir_option().set(std::vector<std::string>{});
    config.get_cachedir_option().set((fixture/"cache").string());
    config.get_install_weak_deps_option().set(weak);
    config.get_best_option().set(true);config.get_installonly_limit_option().set(0);
    base.get_vars()->set("releasever","46");base.setup();
    const std::string target=reverse?"baseline":"candidate";
    for(const auto& id:{std::string("baseline"),std::string("candidate"),std::string("uke"),std::string("rawhide")}) {
        auto repo=base.get_repo_sack()->create_repo(id);auto& c=repo->get_config();
        if(id=="rawhide")c.get_metalink_option().set("https://mirrors.fedoraproject.org/metalink?repo=rawhide&arch=aarch64");
        else c.get_baseurl_option().set(std::vector<std::string>{id=="uke"?"https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/":"file://"+(fixture/id).string()});
        c.get_skip_if_unavailable_option().set(false);
        c.get_metadata_expire_option().set(3600);
        if(id=="candidate" || id=="baseline")c.get_priority_option().set(id==target?1:99);
    }
    base.get_repo_sack()->load_repos();
    libdnf5::rpm::PackageQuery installed(base);installed.filter_installed();
    libdnf5::rpm::PackageQuery available(base);available.filter_available();available.filter_repo_id(std::vector<std::string>{target});
    std::map<std::string,libdnf5::rpm::Package> wanted;
    std::ifstream names(fixture/"runtime-names");need(bool(names),"Missing runtime package inventory");
    for(std::string name;std::getline(names,name);){
        if(selection=="core" && name!="uke-core-meta")continue;
        if(selection=="kde" && name!="uke-core-meta" && name!="uke-kde-plasma-meta")continue;
        if(selection=="all" && name=="uke-cutefish-meta")continue;
        auto q=available;q.filter_name(name);need(q.size()==1,"Missing or ambiguous target: "+name);
        wanted.emplace(name,*q.begin());
    }
    need(!wanted.empty(),"Empty target selection");
    libdnf5::Goal goal(base);
    for(const auto& [name,pkg]:wanted){
        libdnf5::GoalJobSettings settings;settings.set_to_repo_ids({target});
        settings.set_to_vendors({pkg.get_vendor()});goal.add_rpm_install(pkg.get_nevra(),settings);
    }
    auto tx=goal.resolve();
    if(tx.get_problems()!=libdnf5::GoalProblem::NO_PROBLEM){
        for(const auto& line:tx.get_resolve_logs_as_strings())std::cerr<<line<<'\n';
        throw std::runtime_error("Dependency resolution failed: "+mode+"/"+selection);
    }
    std::set<std::string> inbound,retained;unsigned count=0;
    for(const auto& pkg:installed)retained.insert(pkg.get_nevra());
    for(const auto& item:tx.get_transaction_packages()){
        const auto pkg=item.get_package();const auto action=item.get_action();
        need(action!=libdnf5::transaction::TransactionItemAction::REMOVE,"Unexpected removal: "+pkg.get_nevra());
        if(libdnf5::transaction::transaction_item_action_is_inbound(action)){
            inbound.insert(pkg.get_nevra());++count;
            if(wanted.contains(pkg.get_name()))need(pkg.get_repo_id()==target,"Target escaped its exact repository");
            if(selection=="core" && !weak)need(pkg.get_name()!="uke-plasma-controls" && pkg.get_name()!="uke-plymonth" && pkg.get_name()!="uke-cutefish-meta","Optional selection entered minimal core");
        }
    }
    for(const auto& [name,pkg]:wanted)need(inbound.contains(pkg.get_nevra())||retained.contains(pkg.get_nevra()),"Target not selected: "+name);
    std::cout<<"UKE_INTEGRATION_SOLVER mode="<<mode<<" selection="<<selection<<" targets="<<wanted.size()<<" inbound="<<count<<" weak="<<weak<<" transaction_executed=false hardware_tested=false\n";
    return 0;
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
