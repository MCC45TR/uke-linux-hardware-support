%global upstream_commit 6d03c3f8fb19b86f84b2b6c60ff20d1402f0ac41
%global source_digest e8756bf642eb3fc458209b8dd2380571dcb52ec4b2201e9d09db208b7e995d52

Name:           uke-vaapi-v4l2
Version:        1.0.0
Release:        0.1.20261025git6d03c3f%{?dist}
Summary:        Experimental stateful Iris VA-API backend
License:        MIT
URL:            https://github.com/radxa-pkg/libva-v4l2
Source0:        libva-v4l2-%{upstream_commit}.tar.gz
Source1:        runtime-caps.cpp
Source2:        README.md
Patch0:         0001-runtime-capability-intersection.patch
BuildRequires:  gcc-c++
BuildRequires:  meson >= 0.61
BuildRequires:  ninja-build
BuildRequires:  pkgconfig(libva) >= 1.10
BuildRequires:  pkgconfig(libdrm)
BuildRequires:  pkgconfig(egl)
BuildRequires:  pkgconfig(glesv2) >= 3.2
BuildRequires:  pkgconfig(gbm)

%description
Opt-in VA-API backend for the Qualcomm Iris stateful V4L2 API. Profiles and
entrypoints are intersected with kernel format and profile-menu queries.
Requires a separately qualified Iris platform and firmware. Installation
does not establish working Uke codecs. FastCV VPP is disabled. The MSM alias
is omitted until native V4L2 and application acceptance are complete.

%prep
echo '%{source_digest}  %{SOURCE0}' | sha256sum --check --status
%setup -q -n radxa-pkg-libva-v4l2-6d03c3f
%patch -P 0 -p1

%build
%meson -Dgpu_copy=enabled -Dfastcv=disabled
%meson_build

%check
%{__cxx} -std=c++17 -O2 -Wall -Wextra -Werror $(pkg-config --cflags libva) \
    -Isrc src/runtime-caps.cpp %{SOURCE1} \
    -Wl,--wrap=open,--wrap=ioctl,--wrap=close -o runtime-caps-test
./runtime-caps-test

%install
%meson_install
rm %{buildroot}%{_libdir}/dri/msm_drv_video.so
rm -r %{buildroot}%{_datadir}/doc/libva-v4l2
rm -r %{buildroot}%{_datadir}/licenses/libva-v4l2
install -m 0644 %{SOURCE2} UKE-README.md

%files
%license LICENSE
%doc UKE-README.md
%{_libdir}/dri/v4l2_drv_video.so

%changelog
* Sun Oct 25 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-0.1.20261025git6d03c3f
- Pin the upstream backend and query runtime codec/profile availability.
- Keep driver selection opt-in and FastCV VPP disabled for platform bring-up.
