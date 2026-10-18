// SPDX-License-Identifier: MIT
#include <libdnf5/base/base.hpp>
#include <libdnf5/base/goal.hpp>
#include <libdnf5/repo/repo_query.hpp>
#include <libdnf5/repo/repo_sack.hpp>
#include <libdnf5/repo/repo_callbacks.hpp>
#include <libdnf5/rpm/rpm_signature.hpp>
#include <libdnf5/rpm/package_query.hpp>
#include <libdnf5/rpm/package_sack.hpp>
#include <libdnf5/transaction/transaction_item_action.hpp>
#include <rpm/rpmver.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

namespace uke::packages {
namespace fs=std::filesystem;
using Package=libdnf5::rpm::Package;
using Action=libdnf5::transaction::TransactionItemAction;
constexpr auto priority=libdnf5::Option::Priority::RUNTIME;
const std::string prefix="copr:copr.fedorainfracloud.org:mcc45tr:";
const std::string kernel="senemos-uke-linux-kernel-mainline";
const std::array<std::string,3> prerelease_excludes{"senemos-uke-linux-kernel-*-*rc[0-9]*.*",
    "senemos-uke-linux-kernel-*-*alpha*.*","senemos-uke-linux-kernel-*-*beta*.*"};
void need(bool ok,const std::string& why){if(!ok)throw std::runtime_error(why);}
bool channel(const std::string& s){return s=="stable"||s=="test"||s=="unstable";}
std::string project(const std::string& s){need(channel(s),"Unknown channel: "+s);return s=="stable"?"uke-linux":"uke-linux-test";}
bool managed_repo(const std::string& id) {
    for(const auto& p:{std::string("uke-linux"),std::string("uke-linux-test")})
        if(id==p||id==prefix+p||id.starts_with(prefix+p+":custom:"))return true;
    return false;
}
bool kernel_package(const std::string& name){return name.starts_with("senemos-uke-linux-kernel-");}
bool protected_package(const Package& p) {
    if(kernel_package(p.get_name())||p.get_name()=="kernel"||p.get_name().starts_with("kernel-"))return true;
    for(const auto& dep:p.get_provides())if(std::string_view(dep.get_name())=="installonlypkg(kernel)")return true;
    return false;
}
std::string kernel_release(const Package& pkg) {
    std::string result;
    for(const auto& dep:pkg.get_provides()) {
        const std::string name=dep.get_name();std::string value;
        if(name=="kernel-uname-r")value=dep.get_version();
        if(name.starts_with("kernel-uname-r(")&&name.ends_with(')'))value=name.substr(15,name.size()-16);
        if(value.empty())continue;
        need(value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._+~-")==std::string::npos,"Invalid kernel release provide");
        need(result.empty()||result==value,"Ambiguous kernel release provides");result=value;
    }
    return result;
}
std::string key(const Package& p){return p.get_name()+"."+p.get_arch();}
bool prerelease(const Package& p) {
    const auto v=p.get_version()+"-"+p.get_release();
    return v.find("rc")!=std::string::npos||v.find("alpha")!=std::string::npos||v.find("beta")!=std::string::npos;
}
bool older(const Package& a,const Package& b) {
    for(auto pair:{std::pair(a.get_epoch(),b.get_epoch()),std::pair(a.get_version(),b.get_version()),std::pair(a.get_release(),b.get_release())})
        if(auto result=rpmvercmp(pair.first.c_str(),pair.second.c_str()))return result<0;
    return false;
}
std::string read_file(const fs::path& path) {
    std::ifstream f(path);if(!f)return {};
    std::string text((std::istreambuf_iterator<char>(f)),{});need(text.size()<=1024*1024,"Oversized configuration: "+path.string());return text;
}
struct Fd {int n=-1;Fd()=default;explicit Fd(int value):n(value){};Fd(const Fd&)=delete;~Fd(){if(n>=0)close(n);}};
void trusted_directory(const fs::path& path,mode_t mode) {
    // Check every ancestor: a redirected parent must not redirect root writes.
    fs::path current="/";
    for(const auto& part:path.relative_path()) {
        current/=part;struct stat s{};
        if(lstat(current.c_str(),&s)!=0){need(errno==ENOENT&&mkdir(current.c_str(),mode)==0,"Cannot create trusted directory: "+current.string());}
        need(lstat(current.c_str(),&s)==0&&S_ISDIR(s.st_mode)&&s.st_uid==0&&!(s.st_mode&0022),"Untrusted directory: "+current.string());
    }
    need(chmod(path.c_str(),mode)==0,"Cannot protect directory");
}
void atomic_write(const fs::path& path,const std::string& value,mode_t mode=0644) {
    struct stat s{};
    if(lstat(path.c_str(),&s)==0)need(S_ISREG(s.st_mode)&&s.st_uid==0&&s.st_nlink==1&&!(s.st_mode&0022),"Untrusted file: "+path.string());
    else need(errno==ENOENT,"Cannot inspect configuration");
    const auto tmp=path.string()+".new";
    Fd out(open(tmp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,mode));
    need(out.n>=0,"Interrupted or concurrent write at "+tmp+"; inspect before retrying");
    size_t offset=0;
    while(offset<value.size()){auto n=write(out.n,value.data()+offset,value.size()-offset);if(n<0&&errno==EINTR)continue;need(n>0,"Configuration write failed");offset+=size_t(n);}
    need(fsync(out.n)==0&&rename(tmp.c_str(),path.c_str())==0,"Cannot commit configuration");
    Fd parent(open(path.parent_path().c_str(),O_DIRECTORY|O_RDONLY|O_CLOEXEC));need(parent.n>=0&&fsync(parent.n)==0,"Cannot sync configuration directory");
}
std::string saved(const fs::path& path) {
    auto s=read_file(path);if(s.empty())return {};
    if(s.ends_with('\n'))s.pop_back();
    need(channel(s),"Invalid channel record: "+path.string());return s;
}
struct Options {std::string selected;bool sync=false,update=false,dry=false,yes=false,status=false;};
Options parse(int argc,char** argv) {
    need(argc>=1,"Missing package command");
    Options o;int i=1;const std::string command=argv[0];
    if(command=="update"){o.sync=true;o.update=true;}
    else {
        need(command=="brunch"||command=="branch","Unknown package command");
        need(argc>=2,"Usage: uke-control brunch stable|test|unstable|sync|status [--dry-run] [--yes]");
        const std::string action=argv[i++];
        if(action=="sync")o.sync=true;else if(action=="status")o.status=true;
        else {need(channel(action),"Unknown channel: "+action);o.selected=action;}
    }
    for(;i<argc;i++) {
        std::string flag=argv[i];
        if(flag=="--dry-run"||(o.update&&flag=="--check")){need(!o.dry,"Duplicate preview option");o.dry=true;}
        else if(flag=="--yes"){need(!o.yes,"Duplicate --yes");o.yes=true;}
        else throw std::runtime_error("Unknown package option: "+flag);
    }
    need(!(o.status&&(o.dry||o.yes)),"status accepts no options");return o;
}
class Keys:public libdnf5::repo::RepoCallbacks {
    std::string fingerprint;
public:
    explicit Keys(bool stable):fingerprint(stable?"30DFB81A724CE0762F9DF77F844A293E5068FAFE":"DAFC3C5A881FB49C7167EE2D6F772E3D487BD13E"){}
    bool repokey_import(const libdnf5::rpm::KeyInfo& k) override {
        auto f=k.get_fingerprint();f.erase(std::remove(f.begin(),f.end(),' '),f.end());
        std::transform(f.begin(),f.end(),f.begin(),[](unsigned char c){return std::toupper(c);});
        need(f==fingerprint,"COPR key changed; review its fingerprint before importing");return true;
    }
};
struct Repositories {std::set<std::string> ids;std::string definitions,overrides;};
void unique_repository_ids(libdnf5::Base& base) {
    std::set<std::string> seen;
    for(const auto& dir:base.get_config().get_reposdir_option().get_value()) {
        std::error_code ec;fs::directory_iterator it(dir,ec),end;
        while(!ec&&it!=end) {
            if(it->path().extension()==".repo") {
                std::istringstream text(read_file(it->path()));std::string line;
                while(std::getline(text,line)) {
                    auto start=line.find_first_not_of(" \t");if(start==std::string::npos||line[start]!='[')continue;
                    auto close=line.find(']',start);need(close!=std::string::npos,"Malformed repository section");
                    const auto id=line.substr(start+1,close-start-1);
                    need(seen.insert(id).second,"Duplicate repository ID: "+id+"; repair its definitions before switching channels");
                }
            }
            it.increment(ec);
        }
    }
}
Repositories configure(libdnf5::Base& base,const std::string& selected) {
    unique_repository_ids(base);
    auto sack=base.get_repo_sack();sack->create_repos_from_system_configuration();
    Repositories result;
    for(const auto& repo:libdnf5::repo::RepoQuery(base)) {
        bool ours=managed_repo(repo->get_id());
        for(const auto& url:repo->get_config().get_baseurl_option().get_value())
            for(const auto& p:{"uke-linux/","uke-linux-test/"})
                if(url.starts_with(std::string("https://download.copr.fedorainfracloud.org/results/mcc45tr/")+p))ours=true;
        if(ours){result.ids.insert(repo->get_id());repo->get_config().get_enabled_option().set(priority,false);}
        repo->get_config().get_pkg_gpgcheck_option().set(priority,true);
    }
    for(const auto& p:{std::string("uke-linux"),std::string("uke-linux-test")}) {
        const auto id=prefix+p;
        libdnf5::repo::RepoQuery found(base);found.filter_id(id);
        const bool missing=found.empty();auto repo=missing?sack->create_repo(id):*found.begin();
        auto& c=repo->get_config();const bool enabled=p==project(selected);
        const auto url="https://download.copr.fedorainfracloud.org/results/mcc45tr/"+p+"/";
        // This package is qualified for Rawhide. Do not silently route a released Fedora to it.
        c.get_baseurl_option().set(priority,{url+"fedora-rawhide-$basearch/"});
        c.get_metalink_option().set(priority,"");c.get_mirrorlist_option().set(priority,"");
        c.get_gpgkey_option().set(priority,{url+"pubkey.gpg"});
        c.get_enabled_option().set(priority,enabled);c.get_skip_if_unavailable_option().set(priority,false);
        c.get_pkg_gpgcheck_option().set(priority,true);c.get_sslverify_option().set(priority,true);
        c.get_priority_option().set(priority,1);c.get_metadata_expire_option().set(priority,0);
        auto excludes=c.get_excludepkgs_option().get_value();for(const auto& expression:prerelease_excludes)std::erase(excludes,expression);
        // RC policy is applied below after all candidates have been examined.
        c.get_excludepkgs_option().set(priority,excludes);
        repo->set_callbacks(std::make_unique<Keys>(p=="uke-linux"));
        result.ids.insert(id);
        std::ostringstream stanza;
        stanza<<'['<<id<<"]\nenabled="<<enabled<<"\nbaseurl="<<url<<"fedora-rawhide-$basearch/\nmetalink=\nmirrorlist=\ngpgkey="<<url<<"pubkey.gpg\ngpgcheck=1\npkg_gpgcheck=1\nsslverify=1\npriority=1\nskip_if_unavailable=0\n";
        if(selected!="unstable")excludes.insert(excludes.end(),prerelease_excludes.begin(),prerelease_excludes.end());
        stanza<<"excludepkgs=";for(const auto& e:excludes)stanza<<e<<' ';stanza<<"\n\n";
        result.overrides+=stanza.str();
        if(missing)result.definitions+='['+id+"]\nname=Uke "+p+"\nbaseurl="+url+"fedora-rawhide-$basearch/\nenabled=0\ngpgcheck=1\n\n";
    }
    for(const auto& id:result.ids)if(id!=prefix+"uke-linux"&&id!=prefix+"uke-linux-test")result.overrides+='['+id+"]\nenabled=0\n\n";
    return result;
}
void verify_repositories(const std::set<std::string>& ids,const std::string& selected) {
    libdnf5::Base base;base.load_config();base.setup();unique_repository_ids(base);
    base.get_repo_sack()->create_repos_from_system_configuration();
    for(const auto& id:ids) {
        libdnf5::repo::RepoQuery q(base);q.filter_id(id);need(q.size()==1,"Persisted repository is missing or ambiguous: "+id);
        const auto& c=(*q.begin())->get_config();
        need(c.get_enabled_option().get_value()==(id==prefix+project(selected)),"A repository override conflicts with the selected channel: "+id);
        if(id==prefix+project(selected))need(c.get_pkg_gpgcheck_option().get_value()&&c.get_sslverify_option().get_value(),"Repository admission policy changed");
    }
}
struct Plan {std::map<std::string,Package> targets;std::set<std::string> scope;bool fallback=false;std::string kernel_nevra,krel;};
Plan plan(libdnf5::Base& base,const std::string& selected,const std::set<std::string>& ids) {
    Plan p;const auto repo=prefix+project(selected);
    libdnf5::rpm::PackageQuery installed(base);installed.filter_installed();
    libdnf5::rpm::PackageQuery available(base);available.filter_available();available.filter_repo_id(repo);
    std::map<std::string,std::vector<Package>> candidates;
    for(const auto& pkg:available)candidates[key(pkg)].push_back(pkg);
    std::map<std::string,unsigned> counts;
    bool has_kernel=false;std::set<std::string> kernel_arches;
    for(const auto& pkg:installed)if(kernel_package(pkg.get_name())&&pkg.get_name().ends_with("-core"))kernel_arches.insert(pkg.get_arch());
    for(const auto& pkg:installed) {
        if(kernel_package(pkg.get_name())){has_kernel=true;continue;}
        const auto k=key(pkg);
        const bool ours=ids.contains(pkg.get_from_repo_id())||managed_repo(pkg.get_from_repo_id())||pkg.get_name().starts_with("uke-")||pkg.get_name().starts_with("senemos-uke-");
        if(!ours&&!candidates.contains(k))continue;
        p.scope.insert(k);need(++counts[k]==1,"Duplicate installed package requires repair: "+k);
        need(candidates.contains(k),"Target channel has no replacement for "+k+"; nothing was removed");
        const auto& list=candidates.at(k);
        p.targets.insert_or_assign(k,*std::max_element(list.begin(),list.end(),older));
    }
    if(has_kernel) {
        std::vector<Package> stable,rc;
        for(const auto& pkg:available)if(kernel_arches.contains(pkg.get_arch())&&pkg.get_name()==kernel)
            (prerelease(pkg)?rc:stable).push_back(pkg);
        p.fallback=selected=="unstable"&&rc.empty();
        const auto& choices=selected=="unstable"&&!rc.empty()?rc:stable;
        need(!choices.empty(),"Target channel has no eligible kernel");
        const auto chosen=*std::max_element(choices.begin(),choices.end(),older);
        p.kernel_nevra=chosen.get_nevra();
        std::set<std::string> suffixes{"","-core","-modules","-dtbs","-boot"};
        for(const auto& suffix:suffixes) {
            auto q=available;q.filter_name(chosen.get_name()+suffix);q.filter_evr(chosen.get_evr());q.filter_arch(chosen.get_arch());
            need(q.size()==1,"Missing or ambiguous matched kernel part: "+chosen.get_name()+suffix+" "+chosen.get_evr());
            p.targets.insert_or_assign(key(*q.begin()),*q.begin());p.scope.insert(key(*q.begin()));
        }
        // Reusing an installed uname/module directory would destroy a retained tuple.
        for(const auto& [unused,target]:p.targets)if(target.get_name().ends_with("-core")&&kernel_package(target.get_name())) {
            p.krel=kernel_release(target);need(!p.krel.empty(),"Kernel core lacks an exact kernel-uname-r provide");
            for(const auto& old:installed)if(kernel_package(old.get_name())&&old.get_name().ends_with("-core")&&old.get_nevra()!=target.get_nevra())
                need(kernel_release(old)!=p.krel,"Candidate overwrites an installed module directory; use a distinct kernel release");
        }
    }
    return p;
}
libdnf5::base::Transaction resolve(libdnf5::Base& base,const Plan& p,const std::string& selected,bool update) {
    libdnf5::Goal goal(base);
    libdnf5::rpm::PackageQuery installed(base);installed.filter_installed();
    libdnf5::rpm::PackageSet excluded(base);
    libdnf5::rpm::PackageQuery available(base);available.filter_available();
    for(const auto& pkg:available) {
        const auto k=key(pkg);auto chosen=p.targets.find(k);
        if((kernel_package(pkg.get_name())&&(chosen==p.targets.end()||chosen->second.get_id()!=pkg.get_id()))||
           (p.scope.contains(k)&&pkg.get_repo_id()!=prefix+project(selected)))excluded.add(pkg);
    }
    base.get_rpm_package_sack()->add_user_excludes(excluded);
    for(const auto& [k,target]:p.targets) {
        bool same=false,foreign=false;
        for(const auto& old:installed)if(key(old)==k&&old.get_nevra()==target.get_nevra()){same=true;foreign=old.get_from_repo_id()!=target.get_repo_id();}
        // Existing kernels are retained byte-for-byte, including foreign-channel fallbacks.
        if(same&&protected_package(target))continue;
        if(same&&foreign)goal.add_rpm_reinstall(target);
        else if(!same)goal.add_rpm_install(target);
    }
    if(update)goal.add_rpm_upgrade();
    auto tx=goal.resolve();
    if(tx.get_problems()!=libdnf5::GoalProblem::NO_PROBLEM) {
        std::string errors;for(const auto& e:tx.get_resolve_logs_as_strings())errors+=e+'\n';
        throw std::runtime_error("Channel dependency resolution failed:\n"+errors);
    }
    std::set<std::string> selected_inbound;
    for(const auto& item:tx.get_transaction_packages()) {
        const auto pkg=item.get_package();const auto action=item.get_action();
        need(action!=Action::REMOVE,"Transaction would remove "+pkg.get_nevra()+"; refusing");
        if(protected_package(pkg))need(!libdnf5::transaction::transaction_item_action_is_outbound(action)&&action!=Action::REINSTALL,"Transaction would replace a retained kernel: "+pkg.get_nevra());
        if(libdnf5::transaction::transaction_item_action_is_inbound(action)) {
            if(p.scope.contains(key(pkg)))need(pkg.get_repo_id()==prefix+project(selected),"Solver selected a package outside the selected COPR: "+pkg.get_nevra());
            selected_inbound.insert(pkg.get_nevra());
        }
    }
    for(const auto& [k,target]:p.targets) {
        bool same=false;for(const auto& old:installed)if(key(old)==k&&old.get_nevra()==target.get_nevra()&&(protected_package(old)||old.get_from_repo_id()==target.get_repo_id()))same=true;
        need(same||selected_inbound.contains(target.get_nevra()),"Solver omitted required replacement: "+target.get_nevra());
    }
    return tx;
}
void boot_kernel(const std::string& release) {
    const fs::path helper="/usr/bin/uke-kernel-update";struct stat s{};
    need(lstat(helper.c_str(),&s)==0&&S_ISREG(s.st_mode)&&s.st_uid==0&&!(s.st_mode&0022),"Install the reviewed uke-kernel-update helper");
    const auto bundle="/usr/lib/senemos/kernel-updates/"+release;
    const auto pid=fork();need(pid>=0,"Cannot launch guarded kernel updater");
    if(pid==0){execl(helper.c_str(),helper.c_str(),"apply",bundle.c_str(),static_cast<char*>(nullptr));_exit(127);}
    int status=0;while(waitpid(pid,&status,0)<0)need(errno==EINTR,"Cannot wait for kernel updater");
    need(WIFEXITED(status)&&WEXITSTATUS(status)==0,"Packages are installed but kernel boot selection failed. Pending channel retained; inspect updater output before brunch sync. Do not reboot");
}
void run(Options o) {
    const fs::path state="/var/lib/uke-control", config="/etc/uke-control";
    if(o.status) {
        auto current=saved(config/"channel");auto pending=saved(state/"channel-pending");
        utsname u{};uname(&u);
        std::cout<<"selected="<<(current.empty()?"stable (default; not synchronized)":current)<<"\npending="<<(geteuid()!=0?"unknown (requires root)":pending.empty()?"none":pending)<<"\nrunning_kernel="<<u.release<<"\n";
        libdnf5::Base base;base.load_config();base.setup();base.get_repo_sack()->create_repos_from_system_configuration();
        for(const auto& repo:libdnf5::repo::RepoQuery(base))if(managed_repo(repo->get_id()))std::cout<<"repo="<<repo->get_id()<<" enabled="<<repo->get_config().get_enabled_option().get_value()<<'\n';
        return;
    }
    need(geteuid()==0,"Package operations require root; use sudo uke-control (preview also requires the system RPM lock)");
    const auto dt=read_file("/sys/firmware/devicetree/base/compatible");
    need(dt.find(std::string("xiaomi,uke\0",11))!=std::string::npos&&dt.find(std::string("qcom,sm7675\0",12))!=std::string::npos,"Package changes require Uke / SM7675");
    need(read_file("/etc/os-release").find("Rawhide")!=std::string::npos,"Channel routing is currently qualified only for Fedora Rawhide");
    trusted_directory(state,0700);Fd lock(open((state/"lock").c_str(),O_CREAT|O_RDWR|O_NOFOLLOW|O_CLOEXEC,0600));
    need(lock.n>=0&&flock(lock.n,LOCK_EX|LOCK_NB)==0,"Another uke-control operation is running");
    auto pending=saved(state/"channel-pending");
    if(!pending.empty()){need(o.sync,"An interrupted channel transaction exists; run brunch sync");o.selected=pending;}
    if(o.selected.empty()){o.selected=saved(config/"channel");if(o.selected.empty())o.selected="stable";}
    libdnf5::Base base;base.load_config();
    base.get_config().get_installonly_limit_option().set(priority,0);
    base.get_config().get_protect_running_kernel_option().set(priority,true);
    base.get_config().get_pkg_gpgcheck_option().set(priority,true);
    base.get_config().get_best_option().set(priority,true);
    base.get_config().get_skip_unavailable_option().set(priority,false);
    base.setup();auto repos=configure(base,o.selected);
    base.lock_system_repo(libdnf5::utils::LockAccess::WRITE,libdnf5::utils::LockBlocking::NON_BLOCKING);
    base.get_repo_sack()->load_repos();
    const auto p=plan(base,o.selected,repos.ids);auto tx=resolve(base,p,o.selected,o.update);
    utsname current{};need(uname(&current)==0,"Cannot read running kernel identity");
    const auto gate=read_file("/etc/senemos/kernel-update.conf");std::istringstream gates(gate);std::string line;bool automatic=false;
    while(std::getline(gates,line))if(line=="automatic=1")automatic=true;
    std::cout<<"Channel: "<<o.selected<<"; COPR: "<<project(o.selected)<<"\n";
    if(!p.kernel_nevra.empty())std::cout<<"Selected kernel packages: "<<p.kernel_nevra<<(p.fallback?" (no RC available; test kernel fallback)":"")<<"\n";
    for(const auto& item:tx.get_transaction_packages())std::cout<<libdnf5::transaction::transaction_item_action_to_string(item.get_action())<<' '<<item.get_package().get_nevra()<<" ["<<item.get_package().get_repo_id()<<"]\n";
    if(o.dry){std::cout<<"Preview only: no RPM, channel, boot or repository changes. Metadata cache may refresh.\n";return;}
    need(p.krel.empty()||p.krel==current.release||automatic,"Different default kernel requires the existing reviewed uke-kernel-update automatic=1 gate. No channel or package changes were made; validate the recovery route and updater configuration first");
    if(!o.yes){std::cout<<"Apply these package and repository changes? [y/N] "<<std::flush;std::string answer;std::getline(std::cin,answer);need(answer=="y"||answer=="Y","Cancelled; no channel changes");}
    tx.download();need(tx.check_gpg_signatures(),"Package signature check failed; no channel changes");
    trusted_directory(config,0755);trusted_directory("/etc/yum.repos.d",0755);trusted_directory("/etc/dnf/repos.override.d",0755);
    if(pending.empty()) {
        atomic_write(state/"channel-repositories.before",read_file("/etc/dnf/repos.override.d/zz-uke-control.repo"),0600);
        atomic_write(state/"channel-pending",o.selected+'\n',0600);
    }
    if(!repos.definitions.empty()) {
        const fs::path f="/etc/yum.repos.d/uke-control-channels.repo";
        atomic_write(f,read_file(f)+repos.definitions);
    }
    atomic_write("/etc/dnf/repos.override.d/zz-uke-control.repo",repos.overrides);
    verify_repositories(repos.ids,o.selected);
    atomic_write(state/"channel-transaction.json",tx.serialize(),0600);
    tx.set_description("uke-control channel "+o.selected+(o.update?" update":" sync"));
    const auto result=tx.run();
    need(result==libdnf5::base::Transaction::TransactionRunResult::SUCCESS,"RPM transaction incomplete; keep the pending record and run brunch sync. See DNF history and the private channel transaction record");
    // The RPM hook may already have written this bundle. The updater verifies an idempotent match.
    if(!p.krel.empty()&&automatic)boot_kernel(p.krel);
    atomic_write(config/"channel",o.selected+'\n');
    need(unlink((state/"channel-pending").c_str())==0,"Cannot clear completed channel journal");
    Fd dir(open(state.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));need(dir.n>=0&&fsync(dir.n)==0,"Cannot sync channel journal");
    std::cout<<"Channel synchronized. Retained kernels were not removed or reinstalled.\n"
        "Kernel boot selection follows the existing uke-kernel-update admission and automatic gate. No reboot was requested; physical boot validation remains separate.\n";
}
}
#ifndef UKE_PACKAGES_TEST
int main(int argc,char** argv) {
    try {uke::packages::run(uke::packages::parse(argc,argv));return 0;}
    catch(const std::exception& e){std::cerr<<"uke-control packages: "<<e.what()<<'\n';return 1;}
}
#endif
