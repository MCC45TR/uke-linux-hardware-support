#!/usr/bin/env bash
# Exercise actual node-selection code with renamed, missing and unrelated devices.
set -Eeuo pipefail
source_file=${1:?Pass the prepared gpu_pre_sandbox_hook_linux.cc}
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cat > "$work/check.cpp" <<'CPP'
#include <cassert>
#include <map>
#include <string>
#include <vector>
namespace sandbox::policy {
struct SandboxSeccompBPF { struct Options {
  bool accelerated_video_decode_enabled=false, accelerated_video_encode_enabled=false;
}; };
}
struct BrokerFilePermission {
  std::string path;
  static BrokerFilePermission ReadWrite(const std::string& p) { return {p}; }
};
namespace base {
struct FilePath {
  std::string path;
  FilePath()=default;
  explicit FilePath(const std::string& p):path(p){}
  bool empty() const { return path.empty(); }
  FilePath Append(const std::string& s) const { return FilePath(path+"/"+s); }
  FilePath Append(const FilePath& p) const { return Append(p.path); }
  FilePath BaseName() const { return FilePath(path.substr(path.find_last_of('/')+1)); }
  const std::string& value() const { return path; }
};
static std::map<std::string,std::string> drivers,names;
static std::vector<std::string> nodes;
struct FileEnumerator {
  static constexpr int DIRECTORIES=1;
  size_t offset=0;
  FileEnumerator(const FilePath& root,bool recursive,int type) {
    assert(root.value()=="/sys/class/video4linux" && !recursive && type==DIRECTORIES);
  }
  FilePath Next() { return offset<nodes.size()?FilePath(nodes[offset++]):FilePath(); }
};
static bool ReadSymbolicLink(const FilePath& path,FilePath* value) {
  auto it=drivers.find(path.value());if(it==drivers.end())return false;
  *value=FilePath(it->second);return true;
}
static bool ReadFileToStringWithMaxSize(const FilePath& path,std::string* value,size_t max) {
  auto it=names.find(path.value());if(it==names.end()||it->second.size()>max)return false;
  *value=it->second;return true;
}
}
CPP
awk '/^void AddIrisGpuPermissions\(/ {emit=1} emit {print} emit && /^}/ {found=1;exit} END {if(!found)exit 1}' "$source_file" >> "$work/check.cpp"
cat >> "$work/check.cpp" <<'CPP'
static void node(const std::string& id,const std::string& driver,const std::string& name) {
  auto path="/sys/class/video4linux/"+id; base::nodes.push_back(path);
  base::drivers[path+"/device/driver"]="../../../drivers/"+driver;
  base::names[path+"/name"]=name;
}
int main() {
  node("video0","nt36532_uke_thp","Uke NT36532 THP\n");
  node("video19","qcom-iris","qcom-iris-decoder\n");
  node("video23","qcom-iris","qcom-iris-encoder\n");
  node("video4","camera","qcom-iris-decoder\n");
  node("video5","qcom-iris","unknown\n");
  node("video6","qcom-iris",std::string(65,'x'));
  base::nodes.push_back("/sys/class/video4linux/video7");
  sandbox::policy::SandboxSeccompBPF::Options options;
  std::vector<BrokerFilePermission> p;
  AddIrisGpuPermissions(&p,options);assert(p.empty());
  options.accelerated_video_decode_enabled=true;
  AddIrisGpuPermissions(&p,options);assert(p.size()==1&&p[0].path=="/dev/video19");
  options.accelerated_video_decode_enabled=false;options.accelerated_video_encode_enabled=true;
  p.clear();AddIrisGpuPermissions(&p,options);assert(p.size()==1&&p[0].path=="/dev/video23");
  options.accelerated_video_decode_enabled=true;
  p.clear();AddIrisGpuPermissions(&p,options);
  assert(p.size()==2&&p[0].path=="/dev/video19"&&p[1].path=="/dev/video23");
  options.accelerated_video_decode_enabled=false;
  base::drivers.erase("/sys/class/video4linux/video23/device/driver");
  p.clear();AddIrisGpuPermissions(&p,options);assert(p.empty());
  base::names.erase("/sys/class/video4linux/video19/name");
  options.accelerated_video_decode_enabled=true;
  p.clear();AddIrisGpuPermissions(&p,options);assert(p.empty());
}
CPP
"${CXX:-g++}" -std=c++20 -O2 -Wall -Wextra -Werror "$work/check.cpp" -o "$work/check"
"$work/check"
printf 'Iris node renumbering, direction gates, touch/camera exclusion and missing identity fail-closed passed (host functions only)\n'
