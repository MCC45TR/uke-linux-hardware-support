Name:           uke-fidelityfx
Version:        1.1.4
Release:        3%{?dist}
Summary:        Experimental native FidelityFX raw Vulkan effects
License:        MIT
URL:            https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK
Source0:        uke-fidelityfx-%{version}.tar.gz
Source1:        fidelityfx-1.1.4.tar.gz
Source2:        fidelityfx-native-compiler.tar.gz
Source3:        source-sha256.txt
ExclusiveArch:  aarch64
BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  ninja-build
BuildRequires:  glslang
BuildRequires:  utf8cpp-devel
BuildRequires:  vulkan-loader-devel
BuildRequires:  vulkan-headers
BuildRequires:  binutils
BuildRequires:  pkgconfig
BuildRequires:  patch

%description
Original AMD SDK 1.1.4 raw FSR 3.1.4 upscaler, optical flow and frame
interpolation APIs with explicit downstream Linux portability patches.
The Windows interpolation swapchain is excluded. Installing this library
does not enable frame generation in games or the desktop.

%package devel
Summary:        Matched Linux context headers for raw FidelityFX effects
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       vulkan-headers

%description devel
Headers and link metadata for the experimental native Vulkan library.
The opaque context ABI differs from the original Windows headers.

%package tests
Summary:        CPU checks and opt-in bounded GPU pipeline diagnostics
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       bash
Requires:       systemd

%description tests
Native C++ API and compatibility checks plus an explicitly invoked GPU
pipeline diagnostic. No test starts automatically. The GPU diagnostic
does not dispatch an effect, generate frames or measure presentation.

%prep
cd %{_sourcedir}
sha256sum -c %{SOURCE3}
cd -
%setup -q -a 1 -a 2
for change in patches/*.patch; do
    patch --directory=fidelityfx-1.1.4 -p1 < "$change"
done
patch --directory=fidelityfx-native-compiler -p1 < src/native-compiler-md5.patch
cp src/posix-compat.h fidelityfx-1.1.4/sdk/src/backends/vk/ffx_posix_compat.h
cp src/compiler.CMakeLists.txt fidelityfx-native-compiler/CMakeLists.txt

%build
cmake -S fidelityfx-native-compiler -B build-compiler -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="%{optflags}"
cmake --build build-compiler --parallel 2
cmake -S fidelityfx-1.1.4/sdk -B build-effects -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
    -DCMAKE_CXX_FLAGS="%{optflags}" \
    -DCMAKE_SHARED_LINKER_FLAGS="%{build_ldflags} -Wl,-z,defs" \
    -DFFX_API_BACKEND=VK_X64 -DFFX_BUILD_AS_DLL=OFF \
    -DFFX_FSR3UPSCALER=ON -DFFX_FI=ON -DFFX_OF=ON -DFFX_SPD=ON \
    -DFFX_NATIVE_COMPILER="$PWD/build-compiler/FidelityFX_SC"
cmake --build build-effects --parallel 2 --target ffx_backend_vk_aarch64
for check in api gpu; do
    g++ %{optflags} -UNDEBUG -std=c++20 -Wall -Wextra -Werror \
        -isystem fidelityfx-1.1.4/sdk/include src/$check-selfcheck.cpp \
        -Lbuild-effects/src/backends/vk -luke_fidelityfx_raw -lvulkan \
        %{build_ldflags} -o uke-fidelityfx-$check-check
done
g++ %{optflags} -UNDEBUG -std=c++20 -Wall -Wextra -Werror \
    src/posix-selfcheck.cpp %{build_ldflags} -o uke-fidelityfx-compat-check

%install
install -Dm755 build-effects/src/backends/vk/libuke_fidelityfx_raw.so.%{version} \
    %{buildroot}%{_libdir}/libuke_fidelityfx_raw.so.%{version}
ln -s libuke_fidelityfx_raw.so.%{version} %{buildroot}%{_libdir}/libuke_fidelityfx_raw.so.0
ln -s libuke_fidelityfx_raw.so.0 %{buildroot}%{_libdir}/libuke_fidelityfx_raw.so
mkdir -p %{buildroot}%{_includedir}/uke-fidelityfx %{buildroot}%{_libdir}/pkgconfig
cp -a fidelityfx-1.1.4/sdk/include/FidelityFX %{buildroot}%{_includedir}/uke-fidelityfx/
cat > %{buildroot}%{_libdir}/pkgconfig/uke-fidelityfx.pc <<'PKGCONFIG'
prefix=%{_prefix}
libdir=%{_libdir}
includedir=%{_includedir}/uke-fidelityfx
Name: uke-fidelityfx
Description: Experimental Linux raw FidelityFX effects, no presentation provider
Version: %{version}
Libs: -L${libdir} -luke_fidelityfx_raw
Cflags: -I${includedir}
Requires: vulkan
PKGCONFIG
for check in api gpu compat; do
    install -Dm755 uke-fidelityfx-$check-check %{buildroot}%{_bindir}/uke-fidelityfx-$check-check
done
install -Dm755 gpu-test.sh %{buildroot}%{_bindir}/uke-fidelityfx-gpu-test
install -Dm644 %{SOURCE3} %{buildroot}%{_datadir}/uke-fidelityfx/source-sha256.txt

%check
export LD_LIBRARY_PATH="$PWD/build-effects/src/backends/vk" LD_BIND_NOW=1
./uke-fidelityfx-compat-check
./uke-fidelityfx-api-check
./uke-fidelityfx-gpu-check --help
readelf -h build-effects/src/backends/vk/libuke_fidelityfx_raw.so.%{version} | grep -q AArch64
readelf -d build-effects/src/backends/vk/libuke_fidelityfx_raw.so.%{version} | grep -q 'SONAME.*libuke_fidelityfx_raw.so.0'
if find %{buildroot} -type f \( -iname '*.py' -o -iname '*.pyc' -o -iname '*.pyo' \) | grep -q .; then
    echo 'Unexpected Python target payload' >&2
    exit 1
fi
if readelf -d %{buildroot}%{_bindir}/uke-fidelityfx-*-check \
    %{buildroot}%{_libdir}/libuke_fidelityfx_raw.so.%{version} | grep -qi 'NEEDED.*libpython'; then
    echo 'Unexpected Python target linkage' >&2
    exit 1
fi

%files
%license fidelityfx-1.1.4/LICENSE.txt
%doc FIDELITYFX.md fsr-research-20261029.json
%{_libdir}/libuke_fidelityfx_raw.so.0
%{_libdir}/libuke_fidelityfx_raw.so.%{version}
%{_datadir}/uke-fidelityfx/

%files devel
%{_includedir}/uke-fidelityfx/
%{_libdir}/libuke_fidelityfx_raw.so
%{_libdir}/pkgconfig/uke-fidelityfx.pc

%files tests
%{_bindir}/uke-fidelityfx-*-check
%{_bindir}/uke-fidelityfx-gpu-test

%changelog
* Thu Oct 29 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.1.4-3
- Bound native shader compiler MD5 formatting to each two-digit output.

* Thu Oct 29 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.1.4-2
- Preserve matching Rawhide SRPM and binary distribution identities.

* Thu Oct 29 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.1.4-1
- Package pinned raw Vulkan effects and opt-in native diagnostics separately.
