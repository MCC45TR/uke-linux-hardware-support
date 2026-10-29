# FidelityFX source integration

29 October 2026. The selected implementation is the original AMD
[FidelityFX SDK v1.1.4](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/tree/c6efa6bf7f2027b3ec94f28578bb5965eabb9e55),
commit `c6efa6bf7f2027b3ec94f28578bb5965eabb9e55`, containing FSR 3.1.4.
The component manifest records source licenses, archive digests and the exact
Linux portability comparisons. Third-party prebuilt effect binaries and Python
shader tooling are excluded.

The development build includes the original FSR 3 upscaler, optical flow and
frame-interpolation effect APIs. It excludes the Win32 interpolation swapchain.
This is an application library, not a desktop-wide frame-generation service.
It does not add FSR 3 to 0 A.D. or qualify a game, presentation or device result.
Patched context headers and the library must be used together: their opaque
storage accommodates Linux's 32-bit `wchar_t` and differs from the Windows ABI.

## Host build

The standalone `uke-fidelityfx` source package builds in COPR for
`fedora-rawhide-aarch64`. Its normal `-devel` and `-tests` subpackages come
from this one SDK producer, independently of the hardware package families.
The source builder acquires the two exact Git revisions and records the
generated source archive digests in the SRPM and runtime documentation.
The older manifest archive digests describe the earlier curated prototype
archives; they are not asserted as digests of newly generated COPR archives.

All further compilation runs in COPR. The commands below document the earlier
host prototype and are not instructions to compile on the tablet.

Use a Linux build environment with GCC supporting C++23, CMake 3.23 or newer,
Ninja, utf8cpp headers, glslangValidator, Vulkan headers and the Vulkan loader.
Compilation does not require a GPU. Check the two source archive SHA-256 values
against `manifests/fsr-research-20261029.json` before extraction. Commands run
from this component directory; use a fresh build directory.

```sh
make test-fsr-compat
mkdir -p build/fidelityfx-source
tar -xzf referances/fsr-20261029/fidelityfx-1.1.4.tar.gz -C build/fidelityfx-source
tar -xzf referances/fsr-20261029/fidelityfx-native-compiler.tar.gz -C build/fidelityfx-source
patch -d build/fidelityfx-source/fidelityfx-1.1.4 -p1 < patches/fsr/0001-linux-sdk-build.patch
patch -d build/fidelityfx-source/fidelityfx-1.1.4 -p1 < patches/fsr/0002-native-compiler-and-raw-effects.patch
cp src/fsr/posix-compat.h build/fidelityfx-source/fidelityfx-1.1.4/sdk/src/backends/vk/ffx_posix_compat.h
cp src/fsr/compiler.CMakeLists.txt build/fidelityfx-source/fidelityfx-native-compiler/CMakeLists.txt
cmake -S build/fidelityfx-source/fidelityfx-native-compiler -B build/fidelityfx-compiler -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/fidelityfx-compiler --parallel 2
cmake -S build/fidelityfx-source/fidelityfx-1.1.4/sdk -B build/fidelityfx-effects -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_PROCESSOR="$(uname -m)" \
  -DFFX_API_BACKEND=VK_X64 -DFFX_BUILD_AS_DLL=OFF \
  -DFFX_FSR3UPSCALER=ON -DFFX_FI=ON -DFFX_OF=ON -DFFX_SPD=ON \
  -DFFX_NATIVE_COMPILER="$(realpath build/fidelityfx-compiler/FidelityFX_SC)"
cmake --build build/fidelityfx-effects --parallel 2
c++ -std=c++20 -Wall -Wextra -Werror \
  -isystem build/fidelityfx-source/fidelityfx-1.1.4/sdk/include \
  src/fsr/api-selfcheck.cpp \
  "build/fidelityfx-effects/src/backends/vk/libffx_backend_vk_$(uname -m).so" \
  -o build/fidelityfx-api-selfcheck
LD_LIBRARY_PATH="$(realpath build/fidelityfx-effects/src/backends/vk)" \
  LD_BIND_NOW=1 ./build/fidelityfx-api-selfcheck
```

`VK_X64` retains the upstream API selector; the actual native compiler and
`CMAKE_SYSTEM_PROCESSOR` determine the CPU target. Check the resulting ELF
architecture rather than inferring it from that selector or an output name.
The C++ shader compiler input is the pinned DethRaid source; the selected
Linux build/context changes retain attribution to the pinned ychwch comparison.
Neither donor's build scripts are executed.

The API self-check loads the library and compares its three CPU-side effect
versions with the matched headers. It does not create a Vulkan device or run
a GPU shader. Vendor headers use `-isystem`; warnings remain errors in the
project-owned check.

## Integration boundary

Current upstream SDK 2.4.0 was also reviewed at commit
`7539ae8b6568ad29ed78a753404351daf3b5cca7`. Its FSR 3 Vulkan swapchain still
uses Win32 threading and requires four distinct queue handles. It does not
provide a ready native Linux/ARM64 replacement for this device's single
graphics/compute queue. The selected older source is not an AMD-supported
Linux port; the small downstream changes must remain explicit.

A caller still has to supply valid frame color, depth, motion vectors,
resource synchronization and generated-frame presentation. Raw effect
dispatch may be evaluated separately from the stock swapchain. Do not remove
its queue validation or claim that an SDK build enables frame generation.
Tablet package installation remains through DNF update after a useful caller
and its separate GPU, pixel, pacing and thermal checks exist.

The tests subpackage includes CPU version/CRT checks and an opt-in
`uke-fidelityfx-gpu-test --execute` pipeline diagnostic. It records failed
Vulkan calls without altering their result. The wrapper limits execution to
45 seconds and 1536 MiB, requires 2 GiB available memory and GPU temperature
below 65 C, and stops at 75 C or below 512 MiB available memory.
It initializes optical-flow/interpolation pipelines only; dispatch, pixel
correctness, presentation and game integration remain separate work.
