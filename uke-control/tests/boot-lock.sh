#!/usr/bin/bash
# COPR-native: actual production Lock with real flock/fork, private files and UID injection.
set -Eeuo pipefail
[[ $# == 1 && $(uname -m) == aarch64 ]]
root=$(realpath "$1")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
ulimit -c 0
cat > "$work/test.cpp" <<'CPP'
#include <cassert>
#include <cerrno>
#include <csignal>
#include <filesystem>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
namespace fs=std::filesystem;
static fs::path state;
static bool foreign=false;
static void root(){}
static void directory(const fs::path& path){fs::create_directories(path);}
static void need(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
struct Fd {int n=-1;~Fd(){if(n>=0)close(n);}};
static int owner_stat(int fd,struct stat* st){const int result=::fstat(fd,st);if(!result)st->st_uid=foreign?1001:0;return result;}
#define fstat owner_stat
CPP
awk '/^struct Lock \{/{p=1} p{print} p&&/^};$/{exit}' "$root/src/backend.cpp" >> "$work/test.cpp"
cat >> "$work/test.cpp" <<'CPP'
#undef fstat
static void refused(auto operation){bool failed=false;try{operation();}catch(const std::exception&){failed=true;}assert(failed);}
static void byte(int fd){char c=0;assert(::read(fd,&c,1)==1&&c=='x');}
static void send(int fd){assert(write(fd,"x",1)==1);}
static bool ready(int fd,int ms){pollfd event{fd,POLLIN,0};const int result=poll(&event,1,ms);assert(result>=0);return result>0&&(event.revents&POLLIN);}
static void joined(pid_t pid){int status=0;assert(waitpid(pid,&status,0)==pid&&WIFEXITED(status)&&WEXITSTATUS(status)==0);}
int main(int argc,char** argv){
 assert(argc==2);state=argv[1];fs::create_directories(state);
 {Lock lock;}
 // Manual requests still refuse contention; boot requests block in the kernel.
 int started[2],finished[2];assert(!pipe(started)&&!pipe(finished));pid_t waiter=0;
 {
  Lock owner;
  const pid_t manual=fork();assert(manual>=0);
  if(!manual){close(owner.fd.n);refused([]{Lock busy;});_exit(0);}joined(manual);
  waiter=fork();assert(waiter>=0);
  if(!waiter){close(owner.fd.n);send(started[1]);{Lock boot(true);send(finished[1]);}_exit(0);}
  byte(started[0]);assert(!ready(finished[0],150));
 }
 assert(ready(finished[0],2000));byte(finished[0]);joined(waiter);
 // Cancellation of a waiting service must not leave an acquired or orphan lock.
 {
  Lock owner;waiter=fork();assert(waiter>=0);
  if(!waiter){close(owner.fd.n);send(started[1]);Lock boot(true);_exit(20);}
  byte(started[0]);assert(!ready(finished[0],100));assert(!kill(waiter,SIGTERM));
  int status=0;assert(waitpid(waiter,&status,0)==waiter&&WIFSIGNALED(status)&&WTERMSIG(status)==SIGTERM);
 }
 {Lock next(true);}
 // Waiting never weakens ownership, link, type or mode admission.
 foreign=true;refused([]{Lock rejected(true);});foreign=false;
 assert(!chmod((state/"lock").c_str(),0644));refused([]{Lock rejected(true);});assert(!chmod((state/"lock").c_str(),0600));
 fs::create_hard_link(state/"lock",state/"extra");refused([]{Lock rejected(true);});fs::remove(state/"extra");
 fs::rename(state/"lock",state/"original");fs::create_symlink(state/"original",state/"lock");refused([]{Lock rejected(true);});fs::remove(state/"lock");
 fs::create_directory(state/"lock");refused([]{Lock rejected(true);});fs::remove(state/"lock");fs::rename(state/"original",state/"lock");
 {Lock next(true);}
 for(int fd:{started[0],started[1],finished[0],finished[1]})close(fd);
}
CPP
for compiler in g++ clang++;do
 for mode in plain sanitized;do
  flags=(-std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror)
  [[ $mode != sanitized ]] || flags+=("-fsanitize=address,undefined" -fno-omit-frame-pointer)
  "$compiler" "${flags[@]}" "$work/test.cpp" -o "$work/check"
  readelf -h "$work/check" | grep -F AArch64
  timeout --kill-after=2s 20s "$work/check" "$work/state"
  echo 'PASS: actual boot lock waiting, release, manual contention, waiter cancellation and unchanged ownership/mode/link/type refusals; private fixture only'
 done
done
