#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Host-only regression of the pinned upstream C function; no device access.
set -Eeuo pipefail
ulimit -c 0
source=${1:?Pass upstream src/util/file_util.c}
work=$(mktemp -d)
trap 'rm -r --one-file-system -- "$work"' EXIT
cat > "$work/check.cpp" <<'CPP'
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
// Isolated GPL-2.0-or-later upstream ddcutil function, commit 5085ae9.
// Copyright (C) Sanford Rockowitz; the reference file remains unmodified.
CPP
awk '/^file_get_first_line\(/ {print "char *";emit=1} emit{print} emit&&/^}/{exit}' "$source" >> "$work/check.cpp"
cat >> "$work/check.cpp" <<'CPP'
int main(int argc,char **argv) {
 assert(argc==2);
 const std::string root=argv[1];
 auto write=[&](const char *name,const char *value) {
  const auto path=root+"/"+name;FILE *f=fopen(path.c_str(),"w");assert(f);
  assert(fputs(value,f)>=0);assert(!fclose(f));return path;
 };
 auto expect=[&](const std::string &path,const char *value) {
  char *actual=file_get_first_line(path.c_str(),false);
  if(value)assert(actual&&!strcmp(actual,value));else assert(actual==nullptr);
  free(actual);
 };
 expect(write("normal","alpha\nsecond\n"),"alpha");
 expect(write("empty",""),nullptr);
 expect(write("tail","omega"),"omega");
 expect(write("single","x"),"x");
 expect(write("crlf","alpha\r\n"),"alpha\r");
 const std::string long_line(16384,'z');
 expect(write("long",(long_line+"\n").c_str()),long_line.c_str());
 expect(write("long-tail",long_line.c_str()),long_line.c_str());
 expect(write("blank","\n"),"");
 expect(root,nullptr);
 expect(root+"/missing",nullptr);
 for(unsigned i=0;i<1000;i++)expect(root+"/empty",nullptr);
 puts("Pinned first-line reader: normal/empty/unterminated/single/CRLF/long/blank/directory/missing and repeated failure: PASS; build only.");
}
CPP
"${CXX:-c++}" -std=c++20 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer "$work/check.cpp" -o "$work/check"
"$work/check" "$work"
