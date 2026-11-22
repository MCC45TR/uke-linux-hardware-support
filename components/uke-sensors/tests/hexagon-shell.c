// SPDX-License-Identifier: GPL-3.0-or-later
// Compile the actual upstream shell-loader function against bounded I/O failures.
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <misc/fastrpc.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
static ssize_t supplied;
static int initialized, closed, unmapped;
static char buffer[4096];
static int fake_open(const char *path, int flags) { (void)path; (void)flags; return 11; }
static int fake_fstat(int fd, struct stat *s) { (void)fd; s->st_size=32; return 0; }
static long fake_sysconf(int key) { (void)key; return sizeof(buffer); }
static int fake_ioctl(int fd, unsigned long operation, void *value) {
    (void)fd;
    if(operation==FASTRPC_IOCTL_ALLOC_DMA_BUFF) {
        ((struct fastrpc_alloc_dma_buf *)value)->fd=12;
    } else {
        assert(operation==FASTRPC_IOCTL_INIT_CREATE);
        initialized++;
    }
    return 0;
}
static void *fake_mmap(void *p, size_t n, int prot, int flags, int fd, off_t offset) {
    (void)p; (void)n; (void)prot; (void)flags; (void)fd; (void)offset; return buffer;
}
static ssize_t fake_read(int fd, void *p, size_t n) { (void)fd; (void)p; (void)n; return supplied; }
static int fake_munmap(void *p, size_t n) { (void)p; (void)n; unmapped++; return 0; }
static int fake_close(int fd) { (void)fd; closed++; return 0; }
#define open fake_open
#define fstat fake_fstat
#define sysconf fake_sysconf
#define ioctl fake_ioctl
#define mmap fake_mmap
#define read fake_read
#define munmap fake_munmap
#define close fake_close
#include "shell-function.h"
int main(void) {
    const ssize_t cases[]={-1, 0, 31, 32};
    for(unsigned i=0; i<sizeof(cases)/sizeof(cases[0]); i++) {
        const ssize_t count=cases[i];
        supplied=count; initialized=closed=unmapped=0;
        const int result=create_shell_pd(10,"fixture");
        assert((result==0)==(count==32));
        assert(initialized==(count==32));
        assert(closed==2 && unmapped==1);
    }
}
