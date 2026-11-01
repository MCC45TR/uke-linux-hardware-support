// SPDX-License-Identifier: MIT
#include "fedora-target.hpp"
#include "cli.hpp"
#include "maintenance.hpp"
#include "package-operation.hpp"
#include "sysfs.hpp"
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
#include <csignal>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <sys/random.h>
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
std::string project(const std::string& s){need(channel(s),"Unknown channel: "+s);return s=="stable"?"uke-linux":s=="test"?"uke-linux-test":"uke-linux-unstable";}
bool managed_repo(const std::string& id) {
    for(const auto& p:{std::string("uke-linux"),std::string("uke-linux-test"),std::string("uke-linux-unstable")})
        if(id==p||id==prefix+p||id.starts_with(prefix+p+":custom:"))return true;
    return false;
}
bool managed_source(const std::string& id,const std::vector<std::string>& urls) {
    if(managed_repo(id))return true;
    for(const auto& url:urls)for(const auto& p:{"uke-linux/","uke-linux-test/","uke-linux-unstable/"})
        if(url.starts_with(std::string("https://download.copr.fedorainfracloud.org/results/mcc45tr/")+p))return true;
    return false;
}
std::set<std::string> enabled_managed_repos(libdnf5::Base& base) {
    std::set<std::string> result;
    for(const auto& repo:libdnf5::repo::RepoQuery(base))
        if(repo->get_config().get_enabled_option().get_value() &&
           managed_source(repo->get_id(),repo->get_config().get_baseurl_option().get_value()))result.insert(repo->get_id());
    return result;
}
std::string infer_channel(const std::set<std::string>& enabled) {
    if(enabled.size()!=1)return {};
    for(const auto& selection:{"stable","test","unstable"})
        if(*enabled.begin()==project(selection)||*enabled.begin()==prefix+project(selection))return selection;
    return {};
}
bool kernel_package(const std::string& name){return name.starts_with("senemos-uke-linux-kernel-");}
bool private_package(const std::string& name){return name=="uke-local-touch-data"||name=="uke-video-firmware-local";}
bool kernel_userspace(const std::string& name) {
    return name=="kernel-headers"||name=="kernel-tools"||name=="kernel-tools-libs"||name=="kernel-tools-libs-devel";
}
void retain_kernels(libdnf5::Base& base) {
    auto& c=base.get_config();const auto& names=c.get_installonlypkgs_option().get_value();
    // The real selection RPM lacks the installonly provide carried by its payloads.
    if(std::find(names.begin(),names.end(),kernel)==names.end())c.get_installonlypkgs_option().add_item(priority,kernel);
    c.get_installonly_limit_option().set(priority,0);
    c.get_protect_running_kernel_option().set(priority,true);
}
bool protected_package(const Package& p) {
    for(const auto& dep:p.get_provides())if(std::string_view(dep.get_name())=="installonlypkg(kernel)")return true;
    if(private_package(p.get_name()))return true;
    if(p.get_name().starts_with("uke-senemos-kernel-"))return true;
    if(kernel_package(p.get_name())||p.get_name()=="kernel"||(p.get_name().starts_with("kernel-")&&!kernel_userspace(p.get_name())))return true;
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
bool channel_api(const Package& pkg) {
    for(const auto& dep:pkg.get_provides())if(std::string_view(dep.get_name())=="uke-control(channel-api)"&&std::string_view(dep.get_version())=="1")return true;
    return false;
}
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
    need(fchmod(out.n,mode)==0,"Cannot set configuration permissions");
    size_t offset=0;
    while(offset<value.size()){auto n=write(out.n,value.data()+offset,value.size()-offset);if(n<0&&errno==EINTR)continue;need(n>0,"Configuration write failed");offset+=size_t(n);}
    need(fsync(out.n)==0&&rename(tmp.c_str(),path.c_str())==0,"Cannot commit configuration");
    Fd parent(open(path.parent_path().c_str(),O_DIRECTORY|O_RDONLY|O_CLOEXEC));need(parent.n>=0&&fsync(parent.n)==0,"Cannot sync configuration directory");
}
std::string saved(const fs::path& path) {
    Fd input(open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC));
    if(input.n<0){need(errno==ENOENT,"Cannot read channel record: "+path.string());return {};}
    struct stat st{},parent{};
    need(fstat(input.n,&st)==0&&S_ISREG(st.st_mode)&&st.st_uid==0&&st.st_nlink==1&&!(st.st_mode&0022)&&
         lstat(path.parent_path().c_str(),&parent)==0&&S_ISDIR(parent.st_mode)&&parent.st_uid==0&&!(parent.st_mode&0022),
         "Untrusted channel record: "+path.string());
    std::string s;std::array<char,64> bytes{};
    for(;;) {
        const auto n=read(input.n,bytes.data(),bytes.size());if(n<0&&errno==EINTR)continue;
        need(n>=0&&s.size()+size_t(n)<=bytes.size(),"Unreadable or oversized channel record: "+path.string());
        if(!n)break;
        s.append(bytes.data(),size_t(n));
    }
    if(s.ends_with('\n'))s.pop_back();
    need(channel(s),"Invalid channel record: "+path.string());return s;
}
std::optional<operation::Record> saved_operation(const fs::path& path) {
    Fd fd(open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC));
    if(fd.n<0){need(errno==ENOENT,"Cannot read package operation");return std::nullopt;}
    struct stat st{};need(!fstat(fd.n,&st)&&S_ISREG(st.st_mode)&&st.st_uid==0&&st.st_nlink==1&&(st.st_mode&0777)==0600&&st.st_size>0&&st.st_size<=512,"Untrusted package operation record");
    std::string value;std::array<char,513> data{};
    while(true){const auto n=read(fd.n,data.data(),data.size());if(n<0&&errno==EINTR)continue;need(n>=0&&value.size()+size_t(n)<=512,"Invalid package operation size");if(!n)break;value.append(data.data(),size_t(n));}
    return operation::decode(value);
}
std::string operation_id() {
    std::array<unsigned char,16> random{};size_t offset=0;
    while(offset<random.size()){const auto n=getrandom(random.data()+offset,random.size()-offset,0);if(n<0&&errno==EINTR)continue;need(n>0,"Cannot allocate package operation identity");offset+=size_t(n);}
    std::string id;constexpr char hex[]="0123456789abcdef";for(auto byte:random){id+=hex[byte>>4];id+=hex[byte&15];}return id;
}
std::string operation_json(const operation::Record& record) {
    return "{\"schema\":\"org.uke.control.operation.v1\",\"operation_id\":"+sysfs::quote(record.id)+",\"kind\":\"packages\",\"phase\":"+sysfs::quote(record.phase)+
        ",\"needs_recovery\":"+(operation::resume(record.phase)==operation::Resume::review?"true":"false")+",\"resume_action\":"+sysfs::quote(operation::next(record.phase))+
        ",\"physical_boot_verified\":null}";
}
struct CriticalSignals {
    sigset_t previous{};
    CriticalSignals(){sigset_t blocked;sigemptyset(&blocked);for(int signal:{SIGTERM,SIGHUP,SIGINT,SIGPIPE})sigaddset(&blocked,signal);need(!sigprocmask(SIG_BLOCK,&blocked,&previous),"Cannot protect package mutation from terminal loss");}
    ~CriticalSignals(){sigprocmask(SIG_SETMASK,&previous,nullptr);}
};
struct Options {std::string selected;bool sync=false,update=false,dry=false,yes=false,status=false,json=false;};
Options parse(int argc,char** argv) {
    const auto usage=[](bool ok,const std::string& text){if(!ok)throw cli::UsageError(text);};
    usage(argc>=1,"Missing package command");
    Options o;int i=1;const std::string command=argv[0];
    if(command=="update"){o.sync=true;o.update=true;}
    else {
        usage(command=="channel"||command=="brunch"||command=="branch","Unknown package command");
        usage(argc>=2,"Usage: uke-control channel status|set CHANNEL|sync [--dry-run] [--yes]");
        const std::string action=argv[i++];
        if(action=="sync")o.sync=true;else if(action=="status")o.status=true;
        else if(action=="set"){if(i>=argc || !channel(argv[i]))throw cli::UsageError("channel set requires stable, test or unstable");o.selected=argv[i++];}
        else {usage(channel(action),"Unknown channel: "+action);o.selected=action;}
    }
    for(;i<argc;i++) {
        std::string flag=argv[i];
        if(flag=="--dry-run"||(o.update&&flag=="--check")){usage(!o.dry,"Duplicate preview option");o.dry=true;}
        else if(flag=="--yes"){usage(!o.yes,"Duplicate --yes");o.yes=true;}
        else if(flag=="--json"){if(o.json)throw cli::UsageError("duplicate --json");o.json=true;}
        else throw cli::UsageError("Unknown package option: "+flag);
    }
    usage(!(o.status&&(o.dry||o.yes)),"status accepts no transaction options");
    if(o.json && !o.status && !o.dry && !o.yes)throw cli::UsageError("--json package changes require --yes; use --dry-run or --check for a preview");
    return o;
}
std::string signing_key(const std::string& p) {
    if(p=="uke-linux")return "30DFB81A724CE0762F9DF77F844A293E5068FAFE";
    if(p=="uke-linux-test")return "DAFC3C5A881FB49C7167EE2D6F772E3D487BD13E";
    need(p=="uke-linux-unstable","Unknown COPR signing identity: "+p);
    return "2204A661EA630A5303500085538797E72A3E4ECA";
}
class Keys:public libdnf5::repo::RepoCallbacks {
    std::string fingerprint;
public:
    explicit Keys(const std::string& p):fingerprint(signing_key(p)){}
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
    const auto release=fedora_target(read_file("/etc/os-release"));
    need(selected!="unstable"||release=="rawhide","The unstable channel is available only on Fedora Rawhide");
    const auto chroot="fedora-"+release+"-$basearch/";
    auto sack=base.get_repo_sack();
    Repositories result;
    for(const auto& repo:libdnf5::repo::RepoQuery(base)) {
        const bool ours=managed_source(repo->get_id(),repo->get_config().get_baseurl_option().get_value());
        if(ours){result.ids.insert(repo->get_id());repo->get_config().get_enabled_option().set(priority,false);}
        repo->get_config().get_pkg_gpgcheck_option().set(priority,true);
    }
    for(const auto& p:{std::string("uke-linux"),std::string("uke-linux-test"),std::string("uke-linux-unstable")}) {
        const auto id=prefix+p;
        libdnf5::repo::RepoQuery found(base);found.filter_id(id);
        const bool missing=found.empty();auto repo=missing?sack->create_repo(id):*found.begin();
        auto& c=repo->get_config();const bool enabled=p==project(selected);
        const auto url="https://download.copr.fedorainfracloud.org/results/mcc45tr/"+p+"/";
        // A channel change stays within the installed Fedora release.
        c.get_baseurl_option().set(priority,{url+chroot});
        c.get_metalink_option().set(priority,"");c.get_mirrorlist_option().set(priority,"");
        c.get_gpgkey_option().set(priority,{url+"pubkey.gpg"});
        c.get_enabled_option().set(priority,enabled);c.get_skip_if_unavailable_option().set(priority,false);
        c.get_pkg_gpgcheck_option().set(priority,true);c.get_sslverify_option().set(priority,true);
        c.get_priority_option().set(priority,1);c.get_metadata_expire_option().set(priority,0);
        auto excludes=c.get_excludepkgs_option().get_value();for(const auto& expression:prerelease_excludes)std::erase(excludes,expression);
        // RC policy is applied below after all candidates have been examined.
        c.get_excludepkgs_option().set(priority,std::vector<std::string>{});
        c.get_excludepkgs_option().add(priority,excludes);
        repo->set_callbacks(std::make_unique<Keys>(p));
        result.ids.insert(id);
        std::ostringstream stanza;
        stanza<<'['<<id<<"]\nenabled="<<enabled<<"\nbaseurl="<<url<<chroot<<"\nmetalink=\nmirrorlist=\ngpgkey="<<url<<"pubkey.gpg\ngpgcheck=1\npkg_gpgcheck=1\nsslverify=1\npriority=1\nskip_if_unavailable=0\n";
        if(selected!="unstable")excludes.insert(excludes.end(),prerelease_excludes.begin(),prerelease_excludes.end());
        stanza<<"excludepkgs=,";for(const auto& e:excludes)stanza<<e<<' ';stanza<<"\n\n";
        result.overrides+=stanza.str();
        if(missing)result.definitions+='['+id+"]\nname=Uke "+p+"\nbaseurl="+url+chroot+"\nenabled=0\ngpgcheck=1\n\n";
    }
    for(const auto& id:result.ids)if(id!=prefix+"uke-linux"&&id!=prefix+"uke-linux-test"&&id!=prefix+"uke-linux-unstable")result.overrides+='['+id+"]\nenabled=0\n\n";
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
struct Plan {std::map<std::string,Package> targets;std::map<std::string,std::string> renames;std::set<std::string> scope;bool fallback=false;std::string kernel_nevra,krel;};
Plan plan(libdnf5::Base& base,const std::string& selected,const std::set<std::string>& ids,const std::string& dist={}) {
    Plan p;const auto repo=prefix+project(selected);
    libdnf5::rpm::PackageQuery installed(base);installed.filter_installed();
    libdnf5::rpm::PackageQuery available(base);available.filter_available();available.filter_repo_id(repo);
    std::map<std::string,std::vector<Package>> candidates;
    for(const auto& pkg:available) {
        if(!dist.empty())need(fedora_package_release(pkg.get_release(),dist),"Foreign-release candidate: "+pkg.get_nevra());
        candidates[key(pkg)].push_back(pkg);
    }
    std::map<std::string,std::vector<Package>> replacements;
    for(const auto& pkg:available)if(!protected_package(pkg)) {
        std::set<std::string> provides;
        for(const auto& dep:pkg.get_provides())provides.insert(dep.get_name());
        for(const auto& dep:pkg.get_obsoletes()) {
            const std::string name=dep.get_name();
            if(name!=pkg.get_name()&&provides.contains(name))replacements[name].push_back(pkg);
        }
    }
    std::map<std::string,unsigned> counts;
    bool has_kernel=false;std::set<std::string> kernel_arches;
    for(const auto& pkg:installed)if(pkg.get_name()==kernel+"-core")kernel_arches.insert(pkg.get_arch());
    for(const auto& pkg:installed) {
        if(kernel_package(pkg.get_name())){if(pkg.get_name()==kernel||pkg.get_name()==kernel+"-core")has_kernel=true;continue;}
        if(protected_package(pkg))continue;
        const auto k=key(pkg);
        if(pkg.get_name()=="fastfetch") {
            // The project fork is withdrawn; return only this package to Fedora.
            libdnf5::rpm::PackageQuery fedora(base);fedora.filter_available();fedora.filter_repo_id(std::vector<std::string>{"fedora","updates","rawhide"});fedora.filter_name("fastfetch");fedora.filter_arch(pkg.get_arch());
            need(!fedora.empty(),"Original Fedora Fastfetch is unavailable; keep the installed package until its signed replacement is available");
            const auto target=*std::max_element(fedora.begin(),fedora.end(),older);
            p.scope.insert(k);p.targets.insert_or_assign(k,target);continue;
        }
        std::vector<Package> renamed;
        for(const auto& replacement:replacements[pkg.get_name()])
            if(replacement.get_arch()==pkg.get_arch()||replacement.get_arch()=="noarch")renamed.push_back(replacement);
        const bool ours=ids.contains(pkg.get_from_repo_id())||managed_repo(pkg.get_from_repo_id())||pkg.get_name().starts_with("uke-")||pkg.get_name().starts_with("senemos-uke-");
        if(!ours&&!candidates.contains(k)&&renamed.empty())continue;
        p.scope.insert(k);need(++counts[k]==1,"Duplicate installed package requires repair: "+k);
        need(candidates.contains(k)||!renamed.empty(),"Target channel has no replacement for "+k+"; nothing was removed");
        const auto& list=renamed.empty()?candidates.at(k):renamed;
        const auto target=*std::max_element(list.begin(),list.end(),older);
        for(const auto& candidate:renamed)need(candidate.get_name()==target.get_name(),"Ambiguous renamed replacement for "+k);
        if(!renamed.empty())p.renames.emplace(pkg.get_nevra(),target.get_nevra());
        p.scope.insert(key(target));p.targets.insert_or_assign(key(target),target);
        if(pkg.get_name()=="uke-control")need(channel_api(target),"Target uke-control lacks channel API 1; promote a compatible manager first so the return path remains available");
    }
    if(has_kernel) {
        std::vector<Package> stable,rc;
        for(const auto& pkg:available)if(kernel_arches.contains(pkg.get_arch())&&pkg.get_name()==kernel)
            (prerelease(pkg)?rc:stable).push_back(pkg);
        // COPR retains old builds: a superseded RC is not the current unstable kernel.
        if(!stable.empty()) {
            const auto latest=*std::max_element(stable.begin(),stable.end(),older);
            std::erase_if(rc,[&](const Package& candidate){return !older(latest,candidate);});
        }
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
        // Factory calibration belongs to the device, including when a public repo advertises its name.
        if(private_package(pkg.get_name())||
           (kernel_package(pkg.get_name())&&(chosen==p.targets.end()||chosen->second.get_id()!=pkg.get_id()))||
           (p.scope.contains(k)&&pkg.get_repo_id()!=(chosen==p.targets.end()?prefix+project(selected):chosen->second.get_repo_id())))excluded.add(pkg);
    }
    base.get_rpm_package_sack()->add_user_excludes(excluded);
    for(const auto& [k,target]:p.targets) {
        bool same=false,foreign=false;
        for(const auto& old:installed)if(key(old)==k&&old.get_nevra()==target.get_nevra()){same=true;foreign=old.get_from_repo_id()!=target.get_repo_id();}
        // Protected installed packages are retained byte-for-byte.
        if(same&&protected_package(target))continue;
        // libdnf5 applies vendor exceptions to spec jobs, not explicit package-ID jobs.
        libdnf5::GoalJobSettings settings;
        settings.set_to_repo_ids({target.get_repo_id()});
        settings.set_to_vendors({target.get_vendor()});
        if(same&&foreign)goal.add_rpm_reinstall(target.get_nevra(),settings);
        else if(!same)goal.add_rpm_install(target.get_nevra(),settings);
    }
    if(update)goal.add_rpm_upgrade();
    auto tx=goal.resolve();
    if(tx.get_problems()!=libdnf5::GoalProblem::NO_PROBLEM) {
        std::string errors;for(const auto& e:tx.get_resolve_logs_as_strings())errors+=e+'\n';
        throw std::runtime_error("Channel dependency resolution failed:\n"+errors);
    }
    std::set<std::string> selected_inbound;
    std::set<std::string> renamed_outbound;
    for(const auto& item:tx.get_transaction_packages()) {
        const auto pkg=item.get_package();const auto action=item.get_action();
        need(action!=Action::REMOVE,"Transaction would remove "+pkg.get_nevra()+"; refusing");
        if(action==Action::REPLACED&&p.renames.contains(pkg.get_nevra())) {
            renamed_outbound.insert(pkg.get_nevra());
        }
        if(protected_package(pkg))need(!libdnf5::transaction::transaction_item_action_is_outbound(action)&&action!=Action::REINSTALL,"Transaction would replace a protected package: "+pkg.get_nevra());
        if(libdnf5::transaction::transaction_item_action_is_inbound(action)) {
            if(p.scope.contains(key(pkg))) {
                const auto chosen=p.targets.find(key(pkg));
                const auto expected=chosen==p.targets.end()?prefix+project(selected):chosen->second.get_repo_id();
                need(pkg.get_repo_id()==expected,"Solver selected a package outside its admitted repository: "+pkg.get_nevra());
            }
            selected_inbound.insert(pkg.get_nevra());
        }
    }
    for(const auto& [old,replacement]:p.renames)
        need(renamed_outbound.contains(old)&&selected_inbound.contains(replacement),"Solver omitted the complete declared rename: "+old);
    for(const auto& [k,target]:p.targets) {
        bool same=false;for(const auto& old:installed)if(key(old)==k&&old.get_nevra()==target.get_nevra()&&(protected_package(old)||old.get_from_repo_id()==target.get_repo_id()))same=true;
        need(same||selected_inbound.contains(target.get_nevra()),"Solver omitted required replacement: "+target.get_nevra());
    }
    return tx;
}
bool boot_helper_ready(const fs::path& helper="/usr/bin/uke-kernel-update") {
    struct stat s{};
#ifdef UKE_PACKAGES_TEST
    const uid_t helper_owner=geteuid(); // Disposable COPR fixture helper; production requires root ownership.
#else
    const uid_t helper_owner=0;
#endif
    return lstat(helper.c_str(),&s)==0&&S_ISREG(s.st_mode)&&s.st_uid==helper_owner&&s.st_nlink==1&&!(s.st_mode&0022)&&access(helper.c_str(),X_OK)==0;
}
void boot_kernel(const std::string& release,bool json=false,const fs::path& helper="/usr/bin/uke-kernel-update") {
    need(boot_helper_ready(helper),"Install the reviewed uke-kernel-update helper");
    const auto bundle="/usr/lib/senemos/kernel-updates/"+release;
    const auto pid=fork();need(pid>=0,"Cannot launch guarded kernel updater");
    if(pid==0){if(json && dup2(STDERR_FILENO,STDOUT_FILENO)<0)_exit(127);execl(helper.c_str(),helper.c_str(),"transaction",bundle.c_str(),static_cast<char*>(nullptr));_exit(127);}
    int status=0;while(waitpid(pid,&status,0)<0)need(errno==EINTR,"Cannot wait for kernel updater");
    need(WIFEXITED(status)&&WEXITSTATUS(status)==0,"Packages are installed but kernel boot selection failed. Pending channel retained; inspect updater output before channel sync. Do not reboot");
}
std::string space_observation(fs::path path) {
    const auto requested=path.string();std::error_code error;
    while(!path.empty()&&!fs::exists(path,error)&&path!=path.root_path()){error.clear();path=path.parent_path();}
    struct stat st{};struct statvfs space{};
    if(error||path.empty()||stat(path.c_str(),&st)||statvfs(path.c_str(),&space))return "{\"path\":"+sysfs::quote(requested)+",\"available_bytes\":null}";
    need(space.f_frsize&&space.f_bavail<=std::numeric_limits<uint64_t>::max()/space.f_frsize,"Invalid filesystem space report");
    return "{\"path\":"+sysfs::quote(requested)+",\"filesystem_device\":"+sysfs::quote(std::to_string(major(st.st_dev))+":"+std::to_string(minor(st.st_dev)))+
        ",\"available_bytes\":"+std::to_string(uint64_t(space.f_bavail)*space.f_frsize)+",\"available_inodes\":"+std::to_string(space.f_favail)+"}";
}
void run(Options o) {
    const fs::path state="/var/lib/uke-control", config="/etc/uke-control";
    if(o.status) {
        auto current=saved(config/"channel");const auto pending=geteuid()==0?saved(state/"channel-pending"):std::string{};
        libdnf5::Base base;base.load_config();base.setup();unique_repository_ids(base);base.get_repo_sack()->create_repos_from_system_configuration();
        const bool inferred=current.empty();if(inferred)current=infer_channel(enabled_managed_repos(base));
        utsname u{};uname(&u);
        std::string result="{\"schema\":\"org.uke.control.channel.v1\",\"selected\":"+(current.empty()?std::string("null"):sysfs::quote(current))+
            ",\"inferred\":"+(inferred?"true":"false")+",\"pending\":"+(geteuid()!=0?std::string("null"):sysfs::quote(pending))+
            ",\"pending_readable\":"+(geteuid()==0?"true":"false")+",\"running_kernel\":"+sysfs::quote(u.release);
        const auto operation=geteuid()==0?saved_operation(state/"channel-operation"):std::nullopt;
        result+=",\"operation\":"+(operation?operation_json(*operation):"null")+",\"repositories\":[";
        for(const auto& repo:libdnf5::repo::RepoQuery(base))if(managed_source(repo->get_id(),repo->get_config().get_baseurl_option().get_value())) {
            if(result.back()!='[')result+=',';
            result+="{\"id\":"+sysfs::quote(repo->get_id())+",\"enabled\":"+(repo->get_config().get_enabled_option().get_value()?"true":"false")+"}";
        }
        cli::print(result+"]}",o.json,"Package channel");
        return;
    }
    need(geteuid()==0,"Package operations require root; use sudo uke-control (preview also requires the system RPM lock)");
    const auto dt=read_file("/sys/firmware/devicetree/base/compatible");
    need(dt.find(std::string("xiaomi,uke\0",11))!=std::string::npos&&dt.find(std::string("qcom,sm7675\0",12))!=std::string::npos,"Package changes require Uke / SM7675");
    const auto release=fedora_release(read_file("/etc/os-release"));
    trusted_directory(state,0700);Fd lock(open((state/"lock").c_str(),O_CREAT|O_RDWR|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC,0600));
    struct stat lock_stat{};
    need(lock.n>=0&&!fstat(lock.n,&lock_stat)&&S_ISREG(lock_stat.st_mode)&&lock_stat.st_uid==0&&lock_stat.st_nlink==1&&(lock_stat.st_mode&0777)==0600&&flock(lock.n,LOCK_EX|LOCK_NB)==0,"Another uke-control operation is running or its lock is untrusted");
    auto pending=saved(state/"channel-pending");
    auto previous_operation=saved_operation(state/"channel-operation");
    if(!pending.empty()){need(o.sync,"An interrupted channel transaction exists; run channel sync");o.selected=pending;}
    need(!fs::exists(state/"channel-operation.new"),"An interrupted operation-record write exists; inspect it before another mutation");
    if(pending.empty()&&previous_operation)need(previous_operation->phase=="complete"||operation::resume(previous_operation->phase)==operation::Resume::replan,"A package operation lost its pending marker; inspect RPM history before proceeding");
    if(!pending.empty()) {
        need(previous_operation.has_value(),"The pending transaction predates phase receipts; inspect RPM history and boot state before any retry");
        auto& record=*previous_operation;need(record.channel==pending,"Pending channel and operation identity differ");
        if(o.dry){cli::print(operation_json(record),o.json,"Pending package operation");return;}
        need(operation::resume(record.phase)!=operation::Resume::review,"RPM outcome is incomplete or unknown; no automatic replay. Inspect DNF history and channel status before recovery");
        if(operation::resume(record.phase)!=operation::Resume::replan) {
            Maintenance maintenance;
            need(!fs::exists("/run/initramfs/etc/uke-layout/request.json")&&!fs::exists("/run/initramfs/etc/uke-resize-system.request"),"Resolve prepared offline maintenance before finalizing packages");
            if(!o.yes){std::cout<<"Resume verified package finalization without replaying RPM? [y/N] "<<std::flush;std::string answer;std::getline(std::cin,answer);need(answer=="y"||answer=="Y","Cancelled");}
            trusted_directory(config,0755);CriticalSignals critical;
            operation::Actions actions;
            actions.checkpoint=[&](const auto& current){atomic_write(state/"channel-operation",operation::encode(current),0600);};
            actions.boot=[&]{boot_kernel(record.kernel,o.json);};
            actions.select=[&]{atomic_write(config/"channel",record.channel+'\n');};
            actions.clear=[&]{need(unlink((state/"channel-pending").c_str())==0||errno==ENOENT,"Cannot clear finalized channel journal");Fd directory(open(state.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));need(directory.n>=0&&!fsync(directory.n),"Cannot sync finalized journal");};
            operation::finish(record,actions);cli::print(operation_json(record),o.json,"Package finalization completed");return;
        }
    }
    if(o.selected.empty())o.selected=saved(config/"channel");
    libdnf5::Base base;base.load_config();
    retain_kernels(base);
    base.get_config().get_pkg_gpgcheck_option().set(priority,true);
    base.get_config().get_best_option().set(priority,true);
    base.get_config().get_skip_unavailable_option().set(priority,false);
    base.setup();unique_repository_ids(base);base.get_repo_sack()->create_repos_from_system_configuration();
    if(o.selected.empty())o.selected=infer_channel(enabled_managed_repos(base));
    need(channel(o.selected),"No unambiguous saved or enabled main COPR channel; choose brunch stable|test|unstable explicitly");
    const auto dist=fedora_dist(release,base.get_vars()->get_value("releasever"));
    auto repos=configure(base,o.selected);
    need(base.lock_system_repo(libdnf5::utils::LockAccess::WRITE,libdnf5::utils::LockBlocking::NON_BLOCKING),"Another package manager holds the system repository lock");
    base.get_repo_sack()->load_repos();
    const auto p=plan(base,o.selected,repos.ids,dist);auto tx=resolve(base,p,o.selected,o.update);
    utsname current{};need(uname(&current)==0,"Cannot read running kernel identity");
    std::string plan="{\"schema\":\"org.uke.control.packages.v1\",\"channel\":"+sysfs::quote(o.selected)+",\"copr\":"+sysfs::quote(project(o.selected))+
        ",\"selected_kernels\":"+sysfs::quote(p.kernel_nevra)+",\"test_kernel_fallback\":"+(p.fallback?"true":"false")+
        ",\"kernel_activation_pending\":"+(!p.krel.empty()&&p.krel!=current.release?"true":"false")+",\"packages\":[";
    uint64_t download_bytes=0,inbound_bytes=0,outbound_bytes=0;
    const auto add=[](uint64_t& total,uint64_t bytes){need(bytes<=std::numeric_limits<uint64_t>::max()-total,"Package size estimate overflow");total+=bytes;};
    for(const auto& item:tx.get_transaction_packages()) {
        if(libdnf5::transaction::transaction_item_action_is_inbound(item.get_action())){add(download_bytes,item.get_package().get_download_size());add(inbound_bytes,item.get_package().get_install_size());}
        if(libdnf5::transaction::transaction_item_action_is_outbound(item.get_action()))add(outbound_bytes,item.get_package().get_install_size());
        if(plan.back()!='[')plan+=',';
        plan+="{\"action\":"+sysfs::quote(libdnf5::transaction::transaction_item_action_to_string(item.get_action()))+
            ",\"package\":"+sysfs::quote(item.get_package().get_nevra())+",\"repository\":"+sysfs::quote(item.get_package().get_repo_id())+"}";
    }
    bool helper_in_transaction=false;
    for(const auto& item:tx.get_transaction_packages())if(libdnf5::transaction::transaction_item_action_is_inbound(item.get_action())&&item.get_package().get_name()=="uke-kernel-update") {
        const auto files=item.get_package().get_files();
        helper_in_transaction=std::find(files.begin(),files.end(),"/usr/bin/uke-kernel-update")!=files.end();
    }
    const bool helper_required=!p.krel.empty(),helper_ready=boot_helper_ready();
    plan+="],\"boot_helper\":{\"required\":"+std::string(helper_required?"true":"false")+
        ",\"ready\":"+(helper_ready?"true":"false")+",\"provided_by_transaction\":"+(helper_in_transaction?"true":"false")+"}";
    plan+=",\"space\":{\"download_upper_bound_bytes\":"+std::to_string(download_bytes)+",\"rpm_payload_inbound_bytes\":"+std::to_string(inbound_bytes)+
        ",\"rpm_payload_outbound_bytes\":"+std::to_string(outbound_bytes)+",\"staging_peak_bytes\":null,\"estimate_complete\":false,\"native_rpm_space_checks_required\":true,\"kernel_removal_planned\":false,\"filesystems\":["+
        space_observation("/")+","+space_observation(base.get_config().get_cachedir_option().get_value())+","+space_observation("/usr/lib/senemos/kernel-updates")+","+space_observation("/run")+"]}";
    if(o.dry){cli::print(plan+",\"preview_only\":true,\"operation_completed\":false,\"reboot_requested\":false}",o.json,"Package plan");return;}
    Maintenance maintenance;
    need(!fs::exists("/run/initramfs/etc/uke-layout/request.json")&&!fs::exists("/run/initramfs/etc/uke-resize-system.request"),"Offline storage maintenance is prepared; resolve it before package changes");
    need(!helper_required||helper_ready||helper_in_transaction,"The boot helper is missing or untrusted and this plan does not provide it. No packages changed");
    if(!o.json)cli::print(plan+"}",false,"Package plan");
    if(!o.yes){std::cout<<"Apply these package and repository changes? [y/N] "<<std::flush;std::string answer;std::getline(std::cin,answer);need(answer=="y"||answer=="Y","Cancelled; no channel changes");}
    tx.download();need(tx.check_gpg_signatures(),"Package signature check failed; no channel changes");
    trusted_directory(config,0755);trusted_directory("/etc/yum.repos.d",0755);trusted_directory("/etc/dnf/repos.override.d",0755);
    CriticalSignals critical;
    operation::Record record{pending.empty()?operation_id():previous_operation->id,o.selected,"prepared",p.krel,helper_required};
    operation::Actions actions;
    actions.checkpoint=[&](const auto& current){atomic_write(state/"channel-operation",operation::encode(current),0600);};
    actions.checkpoint(record);
    if(pending.empty()) {
        atomic_write(state/"channel-repositories.before",read_file("/etc/dnf/repos.override.d/zz-uke-control.repo"),0600);
        atomic_write(state/"channel-pending",o.selected+'\n',0600);
    }
    actions.repositories=[&]{
    if(!repos.definitions.empty()) {
        const fs::path f="/etc/yum.repos.d/uke-control-channels.repo";
        atomic_write(f,read_file(f)+repos.definitions);
    }
    atomic_write("/etc/dnf/repos.override.d/zz-uke-control.repo",repos.overrides);
    verify_repositories(repos.ids,o.selected);
    atomic_write(state/"channel-transaction.json",tx.serialize(),0600);
    };
    tx.set_description("uke-control channel "+o.selected+(o.update?" update":" sync"));
    actions.rpm=[&]{
    const auto result=tx.run();
    if(result!=libdnf5::base::Transaction::TransactionRunResult::SUCCESS) {
        std::string errors=libdnf5::base::Transaction::transaction_result_to_string(result);
        for(const auto& error:tx.get_transaction_problems())errors+='\n'+error;
        for(const auto& error:tx.get_gpg_signature_problems())errors+='\n'+error;
        for(const auto& error:tx.get_rpm_messages())errors+='\n'+error;
        throw std::runtime_error("RPM transaction incomplete: "+errors+"\nAutomatic replay is blocked. Inspect DNF history and the private operation record");
    }
    };
    // The updater owns automatic policy and the selected userdata profile.
    actions.boot=[&]{boot_kernel(record.kernel,o.json);};
    actions.select=[&]{atomic_write(config/"channel",o.selected+'\n');};
    actions.clear=[&]{need(unlink((state/"channel-pending").c_str())==0,"Cannot clear completed channel journal");Fd dir(open(state.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC));need(dir.n>=0&&fsync(dir.n)==0,"Cannot sync channel journal");};
    operation::apply(record,actions);
    cli::print(plan+",\"preview_only\":false,\"operation_id\":"+sysfs::quote(record.id)+",\"operation_completed\":true,\"retained_kernels_preserved\":true,\"reboot_requested\":false,\"physical_boot_verified\":null}",o.json,"Channel synchronized");
}
}
#ifndef UKE_PACKAGES_TEST
int main(int argc,char** argv) {
    try {uke::packages::run(uke::packages::parse(argc,argv));return 0;}
    catch(const uke::cli::UsageError& e){return uke::cli::error(e.what(),uke::cli::json_requested(argc,argv),64);}
    catch(const std::exception& e){return uke::cli::error(e.what(),uke::cli::json_requested(argc,argv),1);}
}
#endif
