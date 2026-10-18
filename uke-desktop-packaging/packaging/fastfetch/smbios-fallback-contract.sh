#!/usr/bin/env bash
# Actual upstream fallback function; synthetic I/O only, never physical memory.
set -Eeuo pipefail
source=${1:?Pass patched smbios.c}
work=$(mktemp -d)
trap 'rm -r -- "$work"' EXIT
cat > "$work/check.c" <<'C'
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
typedef struct { char *chars; uint32_t length; } FFstrbuf;
#define FF_DEBUG(...) ((void)0)
#define FF_AUTO_FREE
#define FF_AUTO_CLOSE_FD
#define FF_READ_LE(x) (x)
static unsigned opens, reads;
static int fixture_open(const char *path, int flags) {
    assert(!strcmp(path,"/dev/mem") && flags==(O_RDONLY|O_CLOEXEC));
    ++opens; return 7;
}
#define open fixture_open
static bool readPhysicalMemory(int fd,off_t at,size_t length,void *buffer) {
    assert(fd==7 && at==0xf0000 && length==0x10000 && buffer);
    ++reads; return false;
}
static void ffStrbufClear(FFstrbuf *b) { b->length=0; }
static void ffStrbufEnsureFixedLengthFree(FFstrbuf *b,size_t n) { b->chars=realloc(b->chars,n+1); assert(b->chars); }
C
awk '/^typedef struct \[\[gnu::packed\]\] FFSmbios20EntryPoint/ {emit=1}
 emit {print} emit && /^} FFSmbiosEntryPoint;/ {exit}' "$source" >> "$work/check.c"
awk '/^static bool fillTableBufferFallback\(/ {emit=1}
 emit {print} emit && /^}/ {exit}' "$source" >> "$work/check.c"
cat >> "$work/check.c" <<'C'
int main(void) {
    FFstrbuf b={0};
    assert(!fillTableBufferFallback(&b));
#if defined(__i386__) || defined(__x86_64__)
    assert(opens==1 && reads==1);
#else
    assert(opens==0 && reads==0);
#endif
    free(b.chars);
    return 0;
}
C
"${CC:-cc}" -std=gnu23 -O2 -Wall -Wextra -Werror "$work/check.c" -o "$work/check"
"$work/check"
printf 'Actual SMBIOS fallback: architecture boundary and synthetic memory access checks passed.\n'
