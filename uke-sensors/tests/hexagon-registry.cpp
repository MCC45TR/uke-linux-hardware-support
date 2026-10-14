// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise real file operations required by SEE, using disposable host data.
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <fcntl.h>
#include <unistd.h>
extern "C" {
#include "hexagonfs.h"
#include "rpcd_builder.h"
}
int main() {
    char directory[] = "/tmp/uke-registry-contract-XXXXXX";
    assert(mkdtemp(directory));
    hexagonfs_dirent root{};
    root.name = "/";
    root.ops = &hexagonfs_mapped_ops;
    root.u.phys = directory;
    hexagonfs_fd *fds[HEXAGONFS_MAX_FD]{};
    const int rootfd = hexagonfs_open_root(fds, &root);
    assert(rootfd >= 0);
    char outside[] = "/tmp/uke-registry-outside-XXXXXX";
    assert(mkdtemp(outside));
    char secret[256], link[256];
    assert(std::snprintf(secret, sizeof(secret), "%s/secret", outside) > 0);
    int secretfd = open(secret, O_CREAT | O_EXCL | O_RDWR, 0600);
    assert(secretfd >= 0 && write(secretfd, "outside", 7) == 7);
    assert(std::snprintf(link, sizeof(link), "%s/link", directory) > 0);
    assert(symlink(secret, link) == 0);
    assert(hexagonfs_openat(fds, rootfd, rootfd, "/link") < 0);
    assert(hexagonfs_create(fds, rootfd, rootfd, "/link", O_TRUNC) < 0);
    assert(lseek(secretfd, 0, SEEK_SET) == 0);
    char untouched[8]{};
    assert(read(secretfd, untouched, 7) == 7 && !std::memcmp(untouched, "outside", 7));
    assert(close(secretfd) == 0 && unlink(link) == 0);
    assert(symlink(outside, link) == 0);
    assert(hexagonfs_openat(fds, rootfd, rootfd, "/link/secret") < 0);
    assert(hexagonfs_create(fds, rootfd, rootfd, "/link/new", O_TRUNC) < 0);
    hexagonfs_dirent linked_root = root;
    linked_root.u.phys = link;
    assert(hexagonfs_open_root(fds, &linked_root) < 0);
    assert(unlink(link) == 0 && unlink(secret) == 0 && rmdir(outside) == 0);
    int fd = hexagonfs_create(fds, rootfd, rootfd, "/fstempfile", O_TRUNC);
    assert(fd >= 0);
    assert(hexagonfs_write(fds, fd, 3, "14\n") == 3);
    assert(hexagonfs_close(fds, fd) == 0);
    assert(hexagonfs_rename(fds, rootfd, rootfd, "/fstempfile", "/sns_reg_version") == 0);
    fd = hexagonfs_openat(fds, rootfd, rootfd, "/sns_reg_version");
    assert(fd >= 0);
    char value[4]{};
    assert(hexagonfs_read(fds, fd, 3, value) == 3);
    assert(!std::memcmp(value, "14\n", 3));
    assert(hexagonfs_close(fds, fd) == 0);
    assert(hexagonfs_unlink(fds, rootfd, rootfd, "/sns_reg_version") == 0);
    assert(hexagonfs_close(fds, rootfd) == 0);
    const auto make_dir = [&](const char *suffix) {
        char path[256];
        assert(std::snprintf(path, sizeof(path), "%s%s", directory, suffix) > 0);
        assert(mkdir(path, 0700) == 0);
    };
    make_dir("/sensors");
    make_dir("/sensors/config");
    char filename[256];
    assert(std::snprintf(filename, sizeof(filename), "%s/sensors/config/json.lst", directory) > 0);
    int config = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0600);
    assert(config >= 0 && write(config, "fixture\n", 8) == 8 && close(config) == 0);
    auto *tree = construct_root_dir(directory, "adsp");
    assert(tree);
    const int treefd = hexagonfs_open_root(fds, tree);
    assert(treefd >= 0);
    for (const char *path : {"/vendor/etc/sensors/config/json.lst", "/odm/etc/sensors/config/json.lst"}) {
        fd = hexagonfs_openat(fds, treefd, treefd, path);
        assert(fd >= 0);
        char data[8];
        assert(hexagonfs_read(fds, fd, sizeof(data), data) == 8);
        assert(!std::memcmp(data, "fixture\n", 8));
        assert(hexagonfs_close(fds, fd) == 0);
    }
    assert(hexagonfs_close(fds, treefd) == 0);
    assert(unlink(filename) == 0);
    assert(std::snprintf(filename, sizeof(filename), "%s/sensors/config", directory) > 0);
    assert(rmdir(filename) == 0);
    assert(std::snprintf(filename, sizeof(filename), "%s/sensors", directory) > 0);
    assert(rmdir(filename) == 0);
    assert(rmdir(directory) == 0);
}
