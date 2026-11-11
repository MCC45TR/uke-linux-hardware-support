// SPDX-License-Identifier: MIT
// Compare the original SDK's Linux ABI with the exact cumulative kernel base.
// The two unmodified reference headers are source-worker inputs, test only.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <sys/ioctl.h>
#include <linux/types.h>

namespace sdk {
#include "sdk/fastrpc_ioctl.h"
constexpr auto poll_mode = FASTRPC_POLL_MODE;
constexpr auto command_ALLOC_DMA_BUFF = FASTRPC_IOCTL_ALLOC_DMA_BUFF;
constexpr auto command_FREE_DMA_BUFF = FASTRPC_IOCTL_FREE_DMA_BUFF;
constexpr auto command_INVOKE = FASTRPC_IOCTL_INVOKE;
constexpr auto command_INIT_ATTACH = FASTRPC_IOCTL_INIT_ATTACH;
constexpr auto command_INIT_CREATE = FASTRPC_IOCTL_INIT_CREATE;
constexpr auto command_MMAP = FASTRPC_IOCTL_MMAP;
constexpr auto command_MUNMAP = FASTRPC_IOCTL_MUNMAP;
constexpr auto command_INIT_ATTACH_SNS = FASTRPC_IOCTL_INIT_ATTACH_SNS;
constexpr auto command_INIT_CREATE_STATIC = FASTRPC_IOCTL_INIT_CREATE_STATIC;
constexpr auto command_MEM_MAP = FASTRPC_IOCTL_MEM_MAP;
constexpr auto command_MEM_UNMAP = FASTRPC_IOCTL_MEM_UNMAP;
constexpr auto command_SET_OPTION = FASTRPC_IOCTL_SET_OPTION;
constexpr auto command_GET_DSP_INFO = FASTRPC_IOCTL_GET_DSP_INFO;
}
#undef FASTRPC_IOCTL_ALLOC_DMA_BUFF
#undef FASTRPC_IOCTL_FREE_DMA_BUFF
#undef FASTRPC_IOCTL_INVOKE
#undef FASTRPC_IOCTL_INIT_ATTACH
#undef FASTRPC_IOCTL_INIT_CREATE
#undef FASTRPC_IOCTL_MMAP
#undef FASTRPC_IOCTL_MUNMAP
#undef FASTRPC_IOCTL_INIT_ATTACH_SNS
#undef FASTRPC_IOCTL_INIT_CREATE_STATIC
#undef FASTRPC_IOCTL_MEM_MAP
#undef FASTRPC_IOCTL_MEM_UNMAP
#undef FASTRPC_IOCTL_SET_OPTION
#undef FASTRPC_IOCTL_GET_DSP_INFO
#undef FASTRPC_POLL_MODE
namespace kernel {
#include "kernel/fastrpc.h"
constexpr auto poll_mode = FASTRPC_POLL_MODE;
constexpr auto command_ALLOC_DMA_BUFF = FASTRPC_IOCTL_ALLOC_DMA_BUFF;
constexpr auto command_FREE_DMA_BUFF = FASTRPC_IOCTL_FREE_DMA_BUFF;
constexpr auto command_INVOKE = FASTRPC_IOCTL_INVOKE;
constexpr auto command_INIT_ATTACH = FASTRPC_IOCTL_INIT_ATTACH;
constexpr auto command_INIT_CREATE = FASTRPC_IOCTL_INIT_CREATE;
constexpr auto command_MMAP = FASTRPC_IOCTL_MMAP;
constexpr auto command_MUNMAP = FASTRPC_IOCTL_MUNMAP;
constexpr auto command_INIT_ATTACH_SNS = FASTRPC_IOCTL_INIT_ATTACH_SNS;
constexpr auto command_INIT_CREATE_STATIC = FASTRPC_IOCTL_INIT_CREATE_STATIC;
constexpr auto command_MEM_MAP = FASTRPC_IOCTL_MEM_MAP;
constexpr auto command_MEM_UNMAP = FASTRPC_IOCTL_MEM_UNMAP;
constexpr auto command_SET_OPTION = FASTRPC_IOCTL_SET_OPTION;
constexpr auto command_GET_DSP_INFO = FASTRPC_IOCTL_GET_DSP_INFO;
}
static_assert(sizeof(void *) == 8, "AArch64 LP64 qualification required");
static_assert(sdk::poll_mode == kernel::poll_mode);
static_assert(sdk::command_ALLOC_DMA_BUFF == kernel::command_ALLOC_DMA_BUFF);
static_assert(sdk::command_FREE_DMA_BUFF == kernel::command_FREE_DMA_BUFF);
static_assert(sdk::command_INVOKE == kernel::command_INVOKE);
static_assert(sdk::command_INIT_ATTACH == kernel::command_INIT_ATTACH);
static_assert(sdk::command_INIT_CREATE == kernel::command_INIT_CREATE);
static_assert(sdk::command_MMAP == kernel::command_MMAP);
static_assert(sdk::command_MUNMAP == kernel::command_MUNMAP);
static_assert(sdk::command_INIT_ATTACH_SNS == kernel::command_INIT_ATTACH_SNS);
static_assert(sdk::command_INIT_CREATE_STATIC == kernel::command_INIT_CREATE_STATIC);
static_assert(sdk::command_MEM_MAP == kernel::command_MEM_MAP);
static_assert(sdk::command_MEM_UNMAP == kernel::command_MEM_UNMAP);
static_assert(sdk::command_SET_OPTION == kernel::command_SET_OPTION);
static_assert(sdk::command_GET_DSP_INFO == kernel::command_GET_DSP_INFO);
static_assert(sizeof(sdk::fastrpc_invoke_args) == sizeof(kernel::fastrpc_invoke_args));
static_assert(alignof(sdk::fastrpc_invoke_args) == alignof(kernel::fastrpc_invoke_args));
static_assert(offsetof(sdk::fastrpc_invoke_args, ptr) == offsetof(kernel::fastrpc_invoke_args, ptr));
static_assert(offsetof(sdk::fastrpc_invoke_args, length) == offsetof(kernel::fastrpc_invoke_args, length));
static_assert(offsetof(sdk::fastrpc_invoke_args, fd) == offsetof(kernel::fastrpc_invoke_args, fd));
static_assert(offsetof(sdk::fastrpc_invoke_args, attr) == offsetof(kernel::fastrpc_invoke_args, attr));
static_assert(sizeof(sdk::fastrpc_ioctl_invoke) == sizeof(kernel::fastrpc_invoke));
static_assert(alignof(sdk::fastrpc_ioctl_invoke) == alignof(kernel::fastrpc_invoke));
static_assert(offsetof(sdk::fastrpc_ioctl_invoke, handle) == offsetof(kernel::fastrpc_invoke, handle));
static_assert(offsetof(sdk::fastrpc_ioctl_invoke, sc) == offsetof(kernel::fastrpc_invoke, sc));
static_assert(offsetof(sdk::fastrpc_ioctl_invoke, args) == offsetof(kernel::fastrpc_invoke, args));
static_assert(sizeof(sdk::fastrpc_ioctl_alloc_dma_buf) == sizeof(kernel::fastrpc_alloc_dma_buf));
static_assert(alignof(sdk::fastrpc_ioctl_alloc_dma_buf) == alignof(kernel::fastrpc_alloc_dma_buf));
static_assert(offsetof(sdk::fastrpc_ioctl_alloc_dma_buf, fd) == offsetof(kernel::fastrpc_alloc_dma_buf, fd));
static_assert(offsetof(sdk::fastrpc_ioctl_alloc_dma_buf, flags) == offsetof(kernel::fastrpc_alloc_dma_buf, flags));
static_assert(offsetof(sdk::fastrpc_ioctl_alloc_dma_buf, size) == offsetof(kernel::fastrpc_alloc_dma_buf, size));
static_assert(sizeof(sdk::fastrpc_ioctl_init_create) == sizeof(kernel::fastrpc_init_create));
static_assert(alignof(sdk::fastrpc_ioctl_init_create) == alignof(kernel::fastrpc_init_create));
static_assert(offsetof(sdk::fastrpc_ioctl_init_create, filelen) == offsetof(kernel::fastrpc_init_create, filelen));
static_assert(offsetof(sdk::fastrpc_ioctl_init_create, filefd) == offsetof(kernel::fastrpc_init_create, filefd));
static_assert(offsetof(sdk::fastrpc_ioctl_init_create, attrs) == offsetof(kernel::fastrpc_init_create, attrs));
static_assert(offsetof(sdk::fastrpc_ioctl_init_create, siglen) == offsetof(kernel::fastrpc_init_create, siglen));
static_assert(offsetof(sdk::fastrpc_ioctl_init_create, file) == offsetof(kernel::fastrpc_init_create, file));
static_assert(sizeof(sdk::fastrpc_ioctl_init_create_static) == sizeof(kernel::fastrpc_init_create_static));
static_assert(alignof(sdk::fastrpc_ioctl_init_create_static) == alignof(kernel::fastrpc_init_create_static));
static_assert(offsetof(sdk::fastrpc_ioctl_init_create_static, namelen) == offsetof(kernel::fastrpc_init_create_static, namelen));
static_assert(offsetof(sdk::fastrpc_ioctl_init_create_static, memlen) == offsetof(kernel::fastrpc_init_create_static, memlen));
static_assert(offsetof(sdk::fastrpc_ioctl_init_create_static, name) == offsetof(kernel::fastrpc_init_create_static, name));
static_assert(sizeof(sdk::fastrpc_ioctl_req_mmap) == sizeof(kernel::fastrpc_req_mmap));
static_assert(alignof(sdk::fastrpc_ioctl_req_mmap) == alignof(kernel::fastrpc_req_mmap));
static_assert(offsetof(sdk::fastrpc_ioctl_req_mmap, fd) == offsetof(kernel::fastrpc_req_mmap, fd));
static_assert(offsetof(sdk::fastrpc_ioctl_req_mmap, flags) == offsetof(kernel::fastrpc_req_mmap, flags));
static_assert(offsetof(sdk::fastrpc_ioctl_req_mmap, vaddrin) == offsetof(kernel::fastrpc_req_mmap, vaddrin));
static_assert(offsetof(sdk::fastrpc_ioctl_req_mmap, size) == offsetof(kernel::fastrpc_req_mmap, size));
static_assert(offsetof(sdk::fastrpc_ioctl_req_mmap, vaddrout) == offsetof(kernel::fastrpc_req_mmap, vaddrout));
static_assert(sizeof(sdk::fastrpc_ioctl_mem_map) == sizeof(kernel::fastrpc_mem_map));
static_assert(alignof(sdk::fastrpc_ioctl_mem_map) == alignof(kernel::fastrpc_mem_map));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_map, version) == offsetof(kernel::fastrpc_mem_map, version));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_map, fd) == offsetof(kernel::fastrpc_mem_map, fd));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_map, offset) == offsetof(kernel::fastrpc_mem_map, offset));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_map, flags) == offsetof(kernel::fastrpc_mem_map, flags));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_map, vaddrin) == offsetof(kernel::fastrpc_mem_map, vaddrin));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_map, length) == offsetof(kernel::fastrpc_mem_map, length));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_map, vaddrout) == offsetof(kernel::fastrpc_mem_map, vaddrout));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_map, attrs) == offsetof(kernel::fastrpc_mem_map, attrs));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_map, reserved) == offsetof(kernel::fastrpc_mem_map, reserved));
static_assert(sizeof(sdk::fastrpc_ioctl_req_munmap) == sizeof(kernel::fastrpc_req_munmap));
static_assert(alignof(sdk::fastrpc_ioctl_req_munmap) == alignof(kernel::fastrpc_req_munmap));
static_assert(offsetof(sdk::fastrpc_ioctl_req_munmap, vaddrout) == offsetof(kernel::fastrpc_req_munmap, vaddrout));
static_assert(offsetof(sdk::fastrpc_ioctl_req_munmap, size) == offsetof(kernel::fastrpc_req_munmap, size));
static_assert(sizeof(sdk::fastrpc_ioctl_mem_unmap) == sizeof(kernel::fastrpc_mem_unmap));
static_assert(alignof(sdk::fastrpc_ioctl_mem_unmap) == alignof(kernel::fastrpc_mem_unmap));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_unmap, version) == offsetof(kernel::fastrpc_mem_unmap, vesion));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_unmap, fd) == offsetof(kernel::fastrpc_mem_unmap, fd));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_unmap, vaddr) == offsetof(kernel::fastrpc_mem_unmap, vaddr));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_unmap, length) == offsetof(kernel::fastrpc_mem_unmap, length));
static_assert(offsetof(sdk::fastrpc_ioctl_mem_unmap, reserved) == offsetof(kernel::fastrpc_mem_unmap, reserved));
static_assert(sizeof(sdk::fastrpc_ioctl_set_option) == sizeof(kernel::fastrpc_ioctl_set_option));
static_assert(alignof(sdk::fastrpc_ioctl_set_option) == alignof(kernel::fastrpc_ioctl_set_option));
static_assert(offsetof(sdk::fastrpc_ioctl_set_option, req) == offsetof(kernel::fastrpc_ioctl_set_option, request_id));
static_assert(offsetof(sdk::fastrpc_ioctl_set_option, value) == offsetof(kernel::fastrpc_ioctl_set_option, value));
static_assert(offsetof(sdk::fastrpc_ioctl_set_option, reserved) == offsetof(kernel::fastrpc_ioctl_set_option, reserved));
static_assert(sizeof(sdk::fastrpc_ioctl_capability) == sizeof(kernel::fastrpc_ioctl_capability));
static_assert(alignof(sdk::fastrpc_ioctl_capability) == alignof(kernel::fastrpc_ioctl_capability));
static_assert(offsetof(sdk::fastrpc_ioctl_capability, domain) == offsetof(kernel::fastrpc_ioctl_capability, unused));
static_assert(offsetof(sdk::fastrpc_ioctl_capability, attribute_id) == offsetof(kernel::fastrpc_ioctl_capability, attribute_id));
static_assert(offsetof(sdk::fastrpc_ioctl_capability, capability) == offsetof(kernel::fastrpc_ioctl_capability, capability));
static_assert(offsetof(sdk::fastrpc_ioctl_capability, reserved) == offsetof(kernel::fastrpc_ioctl_capability, reserved));
int main() { std::puts("Original SDK/kernel FastRPC ABI: 83 LP64/command/layout assertions passed; no device access."); }
