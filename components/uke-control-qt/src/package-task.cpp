// SPDX-License-Identifier: MIT
#include <cstring>
#include <unistd.h>

int main(int argc,char **argv) {
  if (geteuid()!=0 || argc<2) return 77;
  const auto is=[&](const char *name){return std::strcmp(argv[1],name)==0;};
  const char *control="/usr/bin/uke-control";
  if (argc==2 && is("check"))
    execl(control,control,"--json","update","--check",static_cast<const char *>(nullptr));
  else if (argc==2 && is("update"))
    execl(control,control,"--json","update","--yes",static_cast<const char *>(nullptr));
  else if (argc==3 && is("channel") &&
           (std::strcmp(argv[2],"stable")==0 || std::strcmp(argv[2],"test")==0 || std::strcmp(argv[2],"unstable")==0))
    execl(control,control,"--json","channel","set",argv[2],"--yes",static_cast<const char *>(nullptr));
  else
    return 64;
  return 69;
}
