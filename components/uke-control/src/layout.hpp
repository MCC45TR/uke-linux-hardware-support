// SPDX-License-Identifier: MIT
#pragma once
#include "../shared/recovery/uke.h"
#include <functional>
#include <set>
#include <optional>
namespace uke::layout {
using ure::Value;
using ure::StorageTarget;
using ure::StorageRange;
inline constexpr const char* recovery_source="8395560393f78bcbe2adeaedc44057c0aa7141de";
inline constexpr const char* engine_version="2.1.1";
struct Evidence {
    unsigned userdata_index=0,root_index=0;
    std::string userdata_node,userdata_mount,filesystem,filesystem_uuid;
    std::uint64_t filesystem_bytes=0,partition_bytes=0;
    bool filesystem_size_verified=false,single_device=false,external_journal=false,require_shutdown=false;
    std::set<unsigned> mounted,swaps,holders;
};
struct Proposal {
    Value document;
    std::vector<StorageRange> before,after;
};
std::string seal(Value value,const char* field="sha256");
Value filesystem_probe(int fd,std::uint64_t offset,std::uint64_t bytes);
void xfs_geometry(Value& probe,const std::string& mount,const std::string& device);
Value native_capabilities();
Value backup(const StorageTarget& target,const std::filesystem::path& destination,const std::string& profile);
Value snapshot(const StorageTarget& target,const std::vector<StorageRange>& ranges,const std::filesystem::path& destination,const std::string& profile,const std::string& kind,const std::string& plan_hash);
void validate_authorization(const Value& plan,const Value& consent,const std::string& expected_consent,const std::string& boot_hash,std::uint64_t now);
void require_ownership(const Value& inspection,const Value& receipt,const Value& identity);
Proposal propose(const ure::Root& system,const StorageTarget& target,const Value& request,
                 const std::filesystem::path& original_backup,const std::string& profile,
                 const Value& receipt,const Evidence& evidence,bool preserve_userdata,bool fastboot);
void validate_protected(const StorageTarget& target,const Value& inspection,
                        const std::vector<StorageRange>& before,const std::vector<StorageRange>& after);
std::string preview(const Value& document);
std::string finish_instructions(const Value& plan);
using Progress=std::function<void(const std::string&)>;
struct Inspection { StorageTarget target;Value graph,summary;Evidence evidence; };
Inspection inspect();
Inspection inspect_storage(StorageTarget target,const Value& graph,const Value& userdata);
Value load_private(const std::filesystem::path& path);
void save_private(const std::filesystem::path& path,const Value& data,bool replace=false);
Value run_worker(const std::filesystem::path& request,bool shutdown,const std::string& expected_consent,const Progress& progress={});
int wizard(bool singleboot,bool dry_run);
int command(int argc,char** argv);
void need(bool condition,const std::string& message);
std::string read(const std::filesystem::path& path,std::size_t limit=1024*1024);
std::string native_tool(const std::string& name);
struct Result { int status;std::string output; };
Result run(const std::vector<std::string>& args,const std::filesystem::path& cwd={},int inherited=-1,bool terminal=false);
void checked(const std::vector<std::string>& args,const std::filesystem::path& cwd={});
void power();
bool fastboot_available();
bool btrfs_mount(int fd,const std::string& device,std::uint64_t* devid=nullptr,std::uint64_t* bytes=nullptr);
Value journal_identity(const std::filesystem::path& directory,const StorageTarget& target);
void prepare_shutdown(const std::filesystem::path& job,const std::string& expected_consent);
void build_shutdown_payload(const std::filesystem::path& stage,const Value& request);
void withdraw_shutdown(const std::string& expected_plan);
std::uint64_t boot_ms();
int offline();
void execute(Inspection& current,const Proposal& proposal,const std::filesystem::path& job,bool shutdown,const Progress& progress);
std::string profile();
}
