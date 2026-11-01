# SPDX-License-Identifier: MIT
%global debug_package %{nil}
%global source_commit 87f66007242ff10e9e84bf32be84f9057f6abc75
%global compatibility_evr 22.1.8-4.fc46

Name:           uke-mesa-libclc22-spirv
Version:        0.2.0
Release:        1%{?dist}
Summary:        Mesa-maintained OpenCL built-ins for the Uke libclc22 runtime
License:        Apache-2.0 WITH LLVM-exception
URL:            https://gitlab.freedesktop.org/karolherbst/mesa-libclc
Source0:        https://gitlab.freedesktop.org/api/v4/projects/27606/repository/archive.tar.gz?sha=%{source_commit}#/mesa-libclc-%{source_commit}.tar.gz
Packager:       Uke Linux Maintainers <75160848+MCC45TR@users.noreply.github.com>
ExclusiveArch:  aarch64

BuildRequires:  gcc
BuildRequires:  clang >= 22.1
BuildRequires:  llvm-devel >= 22.1
BuildRequires:  spirv-llvm-translator-tools
BuildRequires:  spirv-tools
BuildRequires:  cmake >= 3.20
BuildRequires:  ninja-build

# shortcut: compatibility covers Fedora libclc22 22.1.8-4 only, review when that base changes.
Provides:       libclc22-spirv = %{compatibility_evr}
Provides:       libclc22-spirv%{?_isa} = %{compatibility_evr}
Obsoletes:      libclc22-spirv <= %{compatibility_evr}
Conflicts:      libclc22-spirv

%description
Pinned Mesa-maintained 32-bit and 64-bit SPIR-V built-in libraries for
Rusticl. This package replaces only the matching Fedora SPIR-V subset.
The installed libclc22 package and its other target libraries remain intact.
Compatibility with another Fedora libclc22 revision requires separate review.

%prep
%setup -q -n mesa-libclc-%{source_commit}-%{source_commit}

%build
# The payload is portable SPIR-V; CPU flags do not apply to these device libraries.
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS= \
  -DLLVM_DIR="$(llvm-config --cmakedir)" \
  '-DLIBCLC_TARGETS_TO_BUILD=spirv-mesa3d-;spirv64-mesa3d-'
cmake --build build --parallel %{_smp_build_ncpus}

%install
install -d -m 0755 %{buildroot}%{_libdir}/clc22
install -m 0644 build/spirv-mesa3d-.spv build/spirv64-mesa3d-.spv \
  %{buildroot}%{_libdir}/clc22/

%check
for library in build/spirv-mesa3d-.spv build/spirv64-mesa3d-.spv; do
  spirv-val --target-env opencl2.2 "$library"
  spirv-dis "$library" -o "$library.spvasm"
  grep -q '__clc_mesa_libclc_version' "$library.spvasm"
done

%files
%license LICENSE.TXT
%doc README.md CREDITS.TXT
%dir %{_libdir}/clc22
%{_libdir}/clc22/spirv-mesa3d-.spv
%{_libdir}/clc22/spirv64-mesa3d-.spv

%changelog
* Sat Oct 24 2026 Uke Linux Maintainers <75160848+MCC45TR@users.noreply.github.com> - 0.2.0-1
- Build the pinned Mesa libclc22 SPIR-V libraries and preserve other Fedora target data.
