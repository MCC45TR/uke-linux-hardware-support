// SPDX-License-Identifier: MIT
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(cl_int result, const char *operation)
{
    if (result != CL_SUCCESS) {
        std::fprintf(stderr, "%s failed: OpenCL=%d\n", operation, result);
        std::exit(1);
    }
}

int main()
{
    cl_uint count = 0;
    check(clGetPlatformIDs(0, nullptr, &count), "count platforms");
    std::vector<cl_platform_id> platforms(count);
    if (count)
        check(clGetPlatformIDs(count, platforms.data(), nullptr), "list platforms");
    unsigned tested = 0;
    for (cl_platform_id platform : platforms) {
        cl_uint device_count = 0;
        cl_int status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &device_count);
        if (status == CL_DEVICE_NOT_FOUND)
            continue;
        check(status, "count GPU devices");
        std::vector<cl_device_id> devices(device_count);
        check(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, device_count, devices.data(), nullptr), "list GPU devices");
        for (cl_device_id device : devices) {
            char name[256]{}, version[256]{};
            cl_device_type type = 0;
            check(clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(name), name, nullptr), "device name");
            check(clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(version), version, nullptr), "device version");
            check(clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(type), &type, nullptr), "device type");
            std::printf("device=%s type=0x%llx version=%s\n", name,
                        static_cast<unsigned long long>(type), version);
            if (!(type & CL_DEVICE_TYPE_GPU) || (type & CL_DEVICE_TYPE_CPU))
                continue;
            cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
            check(status, "create context");
            cl_command_queue queue = clCreateCommandQueue(context, device, 0, &status);
            check(status, "create queue");
            const char *source =
                "__kernel void calculate(__global uint *words) {"
                " uint i=(uint)get_global_id(0); words[i]=i*3u+7u; }"
                "__kernel void arithmetic(__global float4 *values) {"
                " uint i=(uint)get_global_id(0); float x=((int)(i%257)-128)*0.0625f;"
                " values[i]=(float4)(sqrt(fabs(x)+0.25f),sin(x),"
                " exp2(x*0.25f),log2(fabs(x)+1.0f)); }";
            cl_program program = clCreateProgramWithSource(context, 1, &source, nullptr, &status);
            check(status, "create source program");
            status = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", nullptr, nullptr);
            if (status != CL_SUCCESS) {
                size_t length = 0;
                check(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &length), "build log length");
                std::vector<char> log(length + 1);
                check(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, length, log.data(), nullptr), "build log");
                std::fputs(log.data(), stderr);
            }
            check(status, "compile kernel");
            cl_kernel kernel = clCreateKernel(program, "calculate", &status);
            check(status, "create kernel");
            uint32_t words[1024]{};
            cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                           sizeof(words), words, &status);
            check(status, "create initialized buffer");
            check(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer), "set buffer argument");
            constexpr size_t global = 1024;
            check(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global, nullptr, 0, nullptr, nullptr), "dispatch kernel");
            check(clEnqueueReadBuffer(queue, buffer, CL_TRUE, 0, sizeof(words), words, 0, nullptr, nullptr), "read result");
            for (uint32_t i = 0; i < 1024; ++i) {
                if (words[i] != i * 3 + 7) {
                    std::fprintf(stderr, "word %u: expected %u, received %u\n", i, i * 3 + 7, words[i]);
                    return 1;
                }
            }
            check(clReleaseMemObject(buffer), "release buffer");
            check(clReleaseKernel(kernel), "release kernel");
            kernel = clCreateKernel(program, "arithmetic", &status);
            check(status, "create arithmetic kernel");
            float values[1024 * 4]{};
            buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                    sizeof(values), values, &status);
            check(status, "create arithmetic buffer");
            check(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer), "set arithmetic buffer");
            check(clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &global, nullptr, 0, nullptr, nullptr), "dispatch arithmetic");
            check(clEnqueueReadBuffer(queue, buffer, CL_TRUE, 0, sizeof(values), values, 0, nullptr, nullptr), "read arithmetic");
            float max_error = 0;
            for (unsigned i = 0; i < 1024; ++i) {
                const float x = (int(i % 257) - 128) * 0.0625F;
                const float expected[] = {std::sqrt(std::fabs(x) + 0.25F), std::sin(x),
                    std::exp2(x * 0.25F), std::log2(std::fabs(x) + 1.0F)};
                for (unsigned component = 0; component < 4; ++component) {
                    const float value = values[i * 4 + component];
                    const float error = std::fabs(value - expected[component]);
                    if (!std::isfinite(value) || error > 1e-5F + 1e-5F * std::fabs(expected[component])) {
                        std::fprintf(stderr, "arithmetic %u/%u: expected %.9g, received %.9g\n",
                                     i, component, expected[component], value);
                        return 1;
                    }
                    max_error = std::max(max_error, error);
                }
            }
            check(clFinish(queue), "finish queue");
            std::printf("arithmetic_values_checked=4096 max_absolute_error=%.9g\n", max_error);
            check(clReleaseMemObject(buffer), "release arithmetic buffer");
            check(clReleaseKernel(kernel), "release arithmetic kernel");
            check(clReleaseProgram(program), "release program");
            check(clReleaseCommandQueue(queue), "release queue");
            check(clReleaseContext(context), "release context");
            ++tested;
            std::puts("PASS: 1024 integer and 4096 bounded arithmetic GPU results; full API conformance untested");
        }
    }
    if (!tested) {
        std::fputs("no physical GPU tested\n", stderr);
        return 2;
    }
}
