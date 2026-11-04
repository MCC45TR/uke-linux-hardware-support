Name:           uke-waydroid-diagnostics
Version:        0.1.0
Release:        3%{?dist}
Summary:        Bounded Android-side Vulkan checks for Waydroid
License:        MIT AND BSD-2-Clause AND BSD-3-Clause AND Apache-2.0
URL:            https://github.com/MCC45TR/uke-linux-hardware-support
Source0:        %{name}-%{version}.tar.xz
ExclusiveArch:  aarch64
BuildRequires:  clang lld binutils glslang
Requires:       waydroid
%global debug_package %{nil}
%global _build_id_links none
# This private ELF runs inside Android, using its original Bionic/Vulkan ABI.
%global __requires_exclude_from ^/var/lib/waydroid/overlay/system/bin/uke-vulkan-check$
%global __provides_exclude_from ^/var/lib/waydroid/overlay/system/bin/uke-vulkan-check$

%description
Enumerate an actual hardware Vulkan device, compare a 4 KiB GPU buffer fill,
and verify every pixel of sixteen alternating offscreen shader-rendered frames
inside the original Android container. This manual diagnostic does not start a
service or replace Android drivers. It does not establish game compatibility,
display presentation, performance or full API conformance.

%prep
%setup -q

%build
sdk="$PWD/sysroot"
libs="$sdk/usr/lib/aarch64-linux-android/30"
glslangValidator --target-env vulkan1.1 --spirv-val --vn uke_triangle_vertex -o triangle-vertex.h triangle.vert
glslangValidator --target-env vulkan1.1 --spirv-val --vn uke_triangle_fragment -o triangle-fragment.h triangle.frag
clang++ --target=aarch64-linux-android30 --sysroot="$sdk" -fuse-ld=lld \
 -std=c++17 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
 -nostdlib -nostdinc++ -fPIE -pie -Wl,-z,relro,-z,now,-z,max-page-size=16384 \
 -Wl,--dynamic-linker=/system/bin/linker64 -Wl,--no-undefined \
 -isystem "$sdk/usr/include/aarch64-linux-android" -isystem "$sdk/usr/include" \
 "$libs/crtbegin_dynamic.o" vulkan-check.cpp -L"$libs" -lvulkan -lc -ldl \
 "$libs/crtend_android.o" -o uke-vulkan-check

%install
install -Dm0755 uke-vulkan-check \
 %{buildroot}/var/lib/waydroid/overlay/system/bin/uke-vulkan-check

%check
readelf -h uke-vulkan-check | grep -F AArch64
readelf -l uke-vulkan-check | grep -F '/system/bin/linker64'
readelf -d uke-vulkan-check | grep -F '[libvulkan.so]'
readelf -d uke-vulkan-check | grep -F '[libc.so]'
# Fedora strips non-runtime ELF metadata after installation. Verify the actual
# executable code, constants, dynamic symbols and relocations instead.
for section in .text .rodata .dynamic .dynsym .dynstr .rela.plt; do
 readelf -x "$section" uke-vulkan-check > expected-section
 readelf -x "$section" %{buildroot}/var/lib/waydroid/overlay/system/bin/uke-vulkan-check > installed-section
 cmp expected-section installed-section
done

%files
%license LICENSE NDK-NOTICE
%doc source-commit ndk-source.properties
/var/lib/waydroid/overlay/system/bin/uke-vulkan-check

%changelog
* Wed Nov 04 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-3
- Render alternating shader-driven frames and compare every output pixel.

* Wed Nov 04 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-2
- Compare runtime ELF sections across Fedora's normal metadata stripping.

* Wed Nov 04 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.1.0-1
- Build a hardware-device and bounded GPU fill/readback check for Android.
