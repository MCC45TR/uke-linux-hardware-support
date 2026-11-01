/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exercise the actual listener function with a rejected and successful handshake. */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define REMOTECTL 0
struct fastrpc_interface { int unused; };
struct hexagonfs_dirent { int unused; };
static struct fastrpc_interface iface;
static struct hexagonfs_dirent root;
static int stage, registration_error;
static struct hexagonfs_dirent *construct_root_dir(const char *path, const char *dsp)
{ (void)path; (void)dsp; return &root; }
static struct fastrpc_interface *fastrpc_localctl_init(size_t count, struct fastrpc_interface **ifaces)
{ assert(count == 3 && ifaces); return &iface; }
static struct fastrpc_interface *fastrpc_apps_std_init(struct hexagonfs_dirent *dir)
{ assert(dir == &root); return &iface; }
static struct fastrpc_interface *fastrpc_apps_mem_init(int fd)
{ assert(fd == 7); return &iface; }
static int register_fastrpc_listener(int fd)
{ assert(fd == 7 && stage == 0); stage = 1; return registration_error; }
static int sd_notify(int unset, const char *message)
{ assert(!unset && stage == 1 && !registration_error && !strcmp(message, "READY=1")); stage = 2; return 1; }
static void run_fastrpc_listener(int fd, size_t count, struct fastrpc_interface **ifaces)
{ assert(fd == 7 && count == 3 && ifaces && stage == 2); stage = 3; }
static void fastrpc_localctl_deinit(struct fastrpc_interface *entry)
{ assert(entry == &iface && stage == 3); }
#include "listener-function.h"
int main(void)
{
    registration_error = 1;
    start_reverse_tunnel(7, "/run/fixture", "adsp");
    assert(stage == 1);
    stage = registration_error = 0;
    start_reverse_tunnel(7, "/run/fixture", "adsp");
    assert(stage == 3);
    return 0;
}
