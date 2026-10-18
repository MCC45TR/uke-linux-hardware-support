Name:           fastfetch
%global upstream_commit 1c1136ebd1e943d6d2ba7c204a11deee3e948dd8
Version:        2.68.1
Release:        2%{?dist}
Summary:        Fast neofetch-like system information tool

License:        MIT
URL:            https://github.com/fastfetch-cli/fastfetch
Source0:        https://codeload.github.com/fastfetch-cli/fastfetch/tar.gz/%{upstream_commit}#/fastfetch-%{upstream_commit}.tar.gz
Patch0:         0001-limit-legacy-smbios-scan-to-x86.patch
Source1:        smbios-fallback-contract.sh
Source2:        source-lock.json

BuildRequires:  cmake
BuildRequires:  gcc
BuildRequires:  hwdata-devel
BuildRequires:  wayland-devel
BuildRequires:  libXrandr-devel
BuildRequires:  dconf-devel
BuildRequires:  dbus-devel
BuildRequires:  sqlite-devel
BuildRequires:  ImageMagick-devel
BuildRequires:  zlib-devel
BuildRequires:  libglvnd-devel
BuildRequires:  mesa-libGL-devel
BuildRequires:  glib2-devel
BuildRequires:  ocl-icd-devel
BuildRequires:  rpm-devel
BuildRequires:  libdrm-devel
BuildRequires:  pulseaudio-libs-devel
BuildRequires:  elfutils-libelf-devel
# not available on s390x
%if "%{_arch}" != "s390x"
BuildRequires:  libddcutil-devel
%endif
# vulkan-loader not available in el8 on some arches
%if 0%{?rhel} == 8
  %if "%{_arch}" != "s390x" && "%{_arch}" != "ppc64le"
BuildRequires:  vulkan-loader-devel
  %endif
%else
BuildRequires:  vulkan-loader-devel
%endif
BuildRequires:  chafa-devel
BuildRequires:  yyjson-devel
%if 0%{?fedora} || 0%{?rhel} < 10
BuildRequires:  efl-devel
%endif
BuildRequires:  libva-devel
BuildRequires:  libvdpau-devel
BuildRequires:  lua-devel

Recommends:     hwdata
Suggests:       libXrandr
Suggests:       dconf
Suggests:       sqlite-libs
Suggests:       zlib
Suggests:       libglvnd-glx
Suggests:       ImageMagick-libs
Suggests:       glib2
Suggests:       ocl-icd
Suggests:       chafa-libs
Suggests:       libddcutil
Suggests:       libdrm
Suggests:       pulseaudio-libs
Suggests:       elfutils-libelf

# The shell completion files were previously provided as separate subpackages
# which depended on their respective shell.  That was necessary to avoid the
# parent directories of the completion files from being unowned.  However, the
# filesystem package now owns those directories, so the separate subpackages
# are no longer necessary.
Provides:       fastfetch-bash-completion = %{version}%{release}
Provides:       fastfetch-zsh-completion = %{version}%{release}
Provides:       fastfetch-fish-completion = %{version}%{release}
Obsoletes:      fastfetch-bash-completion < 2.31.0-2
Obsoletes:      fastfetch-zsh-completion < 2.31.0-2
Obsoletes:      fastfetch-fish-completion < 2.31.0-2

ExclusiveArch:  aarch64

%description
fastfetch is a neofetch-like tool for fetching system information and
displaying them in a pretty way. It is written in c to achieve much better
performance, in return only Linux and Android are supported. It also uses
mechanisms like multithreading and caching to finish as fast as possible.


%prep
echo 'aaf759881615c9df0aae1c2872562001b58c9c81489579f787ceef65c2743c6c  %{SOURCE0}' | sha256sum -c -
%autosetup -n fastfetch-%{upstream_commit} -p1


%build
%cmake -DBUILD_TESTS=ON -DENABLE_SYSTEM_YYJSON=ON -DBUILD_FLASHFETCH=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Python=ON
%cmake_build


%check
bash %{SOURCE1} src/common/impl/smbios.c
%ctest


%install
%cmake_install
if find %{buildroot} \( -iname '*python*' -o -name '*.py' -o -name '*.pyc' \) | grep .; then exit 1; fi


%files
%license LICENSE
%doc README.md
%{_bindir}/%{name}
%{_datadir}/%{name}/
%{_mandir}/man1/fastfetch.1*
%{_datadir}/bash-completion/completions/%{name}
%{_datadir}/fish/vendor_completions.d/%{name}.fish
%{_datadir}/zsh/site-functions/_%{name}


%changelog
* Sun Oct 18 2026 Senemos Maintainers <75160848+MCC45TR@users.noreply.github.com> - 2.68.1-2
- Reject the legacy PC BIOS memory scan on non-x86 architectures.
- Preserve normal firmware-table detection and use upstream Python-free build fallbacks.
