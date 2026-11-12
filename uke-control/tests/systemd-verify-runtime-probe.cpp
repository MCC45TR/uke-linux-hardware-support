// SPDX-License-Identifier: MIT
// Public path lookup only; no manager, device or namespace activation.
#include <systemd/sd-path.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc,char **argv) {
    if (argc!=2)return 64;
    const bool runtime=std::strcmp(argv[1],"runtime")==0;
    if (!runtime && std::strcmp(argv[1],"state")!=0)return 64;
    char *path=nullptr;
    const int result=sd_path_lookup(runtime ? SD_PATH_SYSTEM_RUNTIME : SD_PATH_SYSTEM_STATE_PRIVATE,nullptr,&path);
    if (result<0)return 1;
    std::puts(path);
    std::free(path);
    return 0;
}
