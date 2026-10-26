#!/usr/bin/env bash
# Exercise the patched Chromium functions; no GPU, browser or device acceptance.
set -Eeuo pipefail
source_file=${1:?Pass the prepared gpu_pre_sandbox_hook_linux.cc}
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cat > "$work/check.cpp" <<'CPP'
#include <algorithm>
#include <array>
#include <cassert>
#include <string>
#include <vector>
#define BUILDFLAG(flag) 0
struct BrokerFilePermission {
  std::string path;
  static BrokerFilePermission ReadOnly(const std::string& p) { return {p}; }
};
namespace sandbox::policy {
struct SandboxSeccompBPF {
  struct Options { bool use_amd_specific_policies=false, use_intel_specific_policies=false,
    use_nvidia_specific_policies=false, use_virtio_specific_policies=false; };
};
}
static bool arm, chromeos, cast;
static bool IsArchitectureArm() { return arm; }
static bool IsChromeOS() { return chromeos; }
static bool UseChromecastSandboxAllowlist() { return cast; }
static bool UseV4L2Codec(const sandbox::policy::SandboxSeccompBPF::Options&) { return false; }
using Permissions = std::vector<BrokerFilePermission>;
static void AddDrmGpuPermissions(Permissions* p) { p->push_back({"mock-existing-drm-policy"}); }
#define STUB(name) static void name(Permissions*) {}
STUB(AddVulkanICDPermissions) STUB(AddStandardChromeOsPermissions)
STUB(AddImgPvrGpuPermissions) STUB(AddArmGpuPermissions) STUB(AddAmdGpuPermissions)
STUB(AddIntelGpuPermissions) STUB(AddStandardGpuPermissions) STUB(AddNvidiaGpuPermissions)
STUB(AddVirtIOGpuPermissions) STUB(AddChromecastArmGpuPermissions)
static void AddV4L2GpuPermissions(Permissions*, const sandbox::policy::SandboxSeccompBPF::Options&) {}
static unsigned iris_policy_calls;
static void AddIrisGpuPermissions(Permissions*, const sandbox::policy::SandboxSeccompBPF::Options&) { ++iris_policy_calls; }
static constexpr int dlopen_flag=1;
static const char* kLibMaliPath="libmali-test", *kLibTegraPath="libtegra-test";
static const char* kAllowedChromecastPaths[]={"/cast/"};
static std::string successful_library;
static std::vector<std::string> loads;
static void* dlopen(const char* name, int) {
  loads.emplace_back(name); return name==successful_library ? &loads : nullptr;
}
CPP
for name in AddArmMesaGpuPermissions FilePermissionsForGpu LoadArmGpuLibraries; do
    awk -v name="$name" '
      !emit && $0 ~ ("[ *]" name "\\(") { emit=1 }
      emit { print }
      emit && /^}/ { found=1; exit }
      END { if (!found) exit 1 }
    ' "$source_file" >> "$work/check.cpp"
done
cat >> "$work/check.cpp" <<'CPP'
static bool has(const Permissions& p, const std::string& path) {
  return std::any_of(p.begin(),p.end(),[&](const auto& v){return v.path==path;});
}
static bool loaded(const char* name) {
  return std::find(loads.begin(),loads.end(),name)!=loads.end();
}
int main() {
  sandbox::policy::SandboxSeccompBPF::Options options;
  arm=true;
  auto p=FilePermissionsForGpu(options);
  assert(has(p,"mock-existing-drm-policy") && iris_policy_calls==1);
  assert(has(p,"/usr/share/glvnd/egl_vendor.d/50_mesa.json"));
  assert(has(p,"/etc/glvnd/egl_vendor.d"));
  assert(!has(p,"/dev/video0") && !has(p,"/"));
  arm=false; p=FilePermissionsForGpu(options);
  assert(!has(p,"mock-existing-drm-policy") && iris_policy_calls==1);
  chromeos=true; p=FilePermissionsForGpu(options);
  assert(!has(p,"/usr/share/glvnd/egl_vendor.d/50_mesa.json"));
  chromeos=false; arm=true; LoadArmGpuLibraries();
  assert(loaded("libglapi.so.0") && loaded("libEGL_mesa.so.0"));
  assert(loaded("/usr/lib64/dri/msm_dri.so")); // libglapi deliberately fails.
  for(const auto& vendor:{kLibMaliPath,kLibTegraPath}) {
    successful_library=vendor; loads.clear(); LoadArmGpuLibraries();
    assert(!loaded("libEGL_mesa.so.0") && !loaded("/usr/lib64/dri/msm_dri.so"));
  }
}
CPP
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Werror "$work/check.cpp" -o "$work/check"
"$work/check"
printf 'Linux ARM broker selection, Mesa discovery, missing-libglapi and vendor isolation passed (host functions only)\n'
