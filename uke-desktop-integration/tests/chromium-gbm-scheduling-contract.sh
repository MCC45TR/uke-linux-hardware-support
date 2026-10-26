#!/usr/bin/env bash
# Compile the actual permission/policy functions; no seccomp or device acceptance.
set -Eeuo pipefail
hook=${1:?Pass gpu_pre_sandbox_hook_linux.cc}
policy=${2:?Pass bpf_gpu_policy_linux.cc}
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cat > "$work/check.cpp" <<'CPP'
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <sched.h>
#include <string>
#include <sys/syscall.h>
#include <vector>
#define BUILDFLAG(flag) flag
#ifndef IS_LINUX
#define IS_LINUX 1
#endif
#ifndef IS_CHROMEOS
#define IS_CHROMEOS 0
#endif
#ifdef UKE_TEST_ARM64
#define ARCH_CPU_ARM64 1
#endif
struct BrokerFilePermission {
  std::string path;
  static BrokerFilePermission ReadOnly(const char* path) { return {path}; }
};
static void AddDrmGpuPermissions(std::vector<BrokerFilePermission>* paths) {
  paths->push_back({"existing-drm-policy"});
}
enum Kind { kAllow,kErrno,kBaseline,kAffinity,kPrlimit,kMemfd,kMremap,kBroker };
struct ResultExpr { Kind kind; int error=0; };
static ResultExpr Allow() { return {kAllow}; }
static ResultExpr Error(int error) { return {kErrno,error}; }
using BoolExpr=bool;
static unsigned long arguments[6];
template<class T> struct Arg {
  unsigned index;
  explicit Arg(unsigned index):index(index){}
  operator T() const { return static_cast<T>(arguments[index]); }
};
static bool AllOf(bool a,bool b) { return a&&b; }
struct Conditional {
  bool condition; ResultExpr yes;
  ResultExpr Else(ResultExpr no) const { return condition?yes:no; }
};
static Conditional If(bool condition,ResultExpr yes) { return {condition,yes}; }
struct SyscallSets {
  static bool IsDlopen(int) { return false; }
  static bool IsSystemVSharedMemory(int) { return false; }
};
struct SandboxLinux {
  static SandboxLinux* GetInstance() { static SandboxLinux value; return &value; }
  bool ShouldBrokerHandleSyscall(int) { return false; }
  ResultExpr HandleViaBroker(int) { return {kBroker}; }
};
static ResultExpr RestrictSchedTarget(int,int) { return {kAffinity}; }
static ResultExpr RestrictPrlimit64(int) { return {kPrlimit}; }
static ResultExpr RestrictMemfdCreateWithExecMappings() { return {kMemfd}; }
static ResultExpr RestrictMremapFlagsForODML() { return {kMremap}; }
struct BPFBasePolicy {
  ResultExpr EvaluateSyscall(int) const { return {kBaseline}; }
};
enum class MremapPolicy { kAllow,kDeny };
struct GpuProcessPolicy:BPFBasePolicy {
  MremapPolicy mremap_policy_=MremapPolicy::kDeny;
  int GetPolicyPid() const { return 42; }
  ResultExpr EvaluateSyscall(int) const;
};
CPP
awk '/^void AddArmMesaGpuPermissions\(/ {emit=1} emit {print}
     emit && /^}/ {found=1;exit} END {if(!found)exit 1}' "$hook" >> "$work/check.cpp"
awk '/^ResultExpr GpuProcessPolicy::EvaluateSyscall\(/ {emit=1} emit {print}
     emit && /^}/ {found=1;exit} END {if(!found)exit 1}' "$policy" >> "$work/check.cpp"
cat >> "$work/check.cpp" <<'CPP'
int main() {
  unsigned failures=0;
  const auto check=[&](bool ok,const char* text) {
    if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);++failures;}
  };
  std::vector<BrokerFilePermission> paths;
  AddArmMesaGpuPermissions(&paths);
  check(std::any_of(paths.begin(),paths.end(),[](const auto& p){
    return p.path=="/usr/lib64/gbm/dri_gbm.so";
  }),"exact GBM backend read permission");
  check(std::none_of(paths.begin(),paths.end(),[](const auto& p){
    return p.path=="/" || p.path=="/usr/lib64/gbm/";
  }),"no recursive GBM or filesystem permission");
  GpuProcessPolicy policy;
  for(unsigned long pid:{0ul,42ul,99ul}) {
    arguments[0]=pid;arguments[1]=SCHED_BATCH;
    const auto result=policy.EvaluateSyscall(__NR_sched_setscheduler);
#if defined(UKE_TEST_ARM64) && IS_LINUX && !IS_CHROMEOS
    check(result.kind==kErrno && result.error==EPERM,"batch hint refused without granting scheduling");
#else
    check(result.kind==kBaseline,"other architecture/platform policy unchanged");
#endif
  }
  for(int scheduler:{SCHED_OTHER,SCHED_FIFO,SCHED_RR,SCHED_IDLE}) {
    arguments[1]=scheduler;
    check(policy.EvaluateSyscall(__NR_sched_setscheduler).kind==kBaseline,
          "other scheduler requests retain baseline policy");
  }
  check(policy.EvaluateSyscall(__NR_sched_setaffinity).kind==kAffinity,
        "existing affinity target restriction retained");
  check(policy.EvaluateSyscall(__NR_sched_setparam).kind==kBaseline,
        "scheduler parameter policy unchanged");
  return failures?1:0;
}
CPP
for platform in arm64-linux x86-linux arm64-chromeos; do
  flags=()
  case $platform in
    arm64-linux) flags=(-DUKE_TEST_ARM64);;
    arm64-chromeos) flags=(-DUKE_TEST_ARM64 -DIS_LINUX=1 -DIS_CHROMEOS=1);;
  esac
  "${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Werror "${flags[@]}" \
    "$work/check.cpp" -o "$work/check"
  "$work/check"
done
printf 'Exact GBM read path and optional ARM64/Linux batch refusal passed (host functions only)\n'
