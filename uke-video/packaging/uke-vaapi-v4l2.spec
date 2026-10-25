%global upstream_commit 6d03c3f8fb19b86f84b2b6c60ff20d1402f0ac41
%global source_digest e8756bf642eb3fc458209b8dd2380571dcb52ec4b2201e9d09db208b7e995d52

Name:           uke-vaapi-v4l2
Version:        1.0.0
Release:        0.4.20261025git6d03c3f%{?dist}
Summary:        Experimental stateful Iris VA-API backend
License:        MIT
URL:            https://github.com/radxa-pkg/libva-v4l2
Source0:        libva-v4l2-%{upstream_commit}.tar.gz
Source1:        runtime-caps.cpp
Source2:        README.md
Source3:        main10-contract.sh
Source4:        rate-negotiation.sh
Patch0:         0001-runtime-capability-intersection.patch
Patch1:         0002-main10-p010-encode.patch
Patch2:         0003-runtime-encode-rate-limits.patch
BuildRequires:  gcc-c++
BuildRequires:  meson >= 0.61
BuildRequires:  ninja-build
BuildRequires:  pkgconfig(libva) >= 1.10
BuildRequires:  pkgconfig(libdrm)
BuildRequires:  pkgconfig(egl)
BuildRequires:  pkgconfig(glesv2) >= 3.2
BuildRequires:  pkgconfig(gbm)

%description
Experimental VA-API backend for the Qualcomm Iris stateful V4L2 API. Profiles and
entrypoints are intersected with kernel format and profile-menu queries.
Requires a separately qualified Iris platform and firmware. Installation
does not establish working Uke codecs. FastCV VPP is disabled. The MSM alias
selects the same backend for default libva discovery on MSM DRM devices;
native codec and application acceptance remain separate requirements.

%prep
echo '%{source_digest}  %{SOURCE0}' | sha256sum --check --status
%setup -q -n radxa-pkg-libva-v4l2-6d03c3f
%patch -P 0 -p1
%patch -P 1 -p1
%patch -P 2 -p1

%build
%meson -Dgpu_copy=enabled -Dfastcv=disabled
%meson_build

%check
%{__cxx} -std=c++17 -O2 -Wall -Wextra -Werror $(pkg-config --cflags libva) \
    -Isrc src/runtime-caps.cpp %{SOURCE1} \
    -Wl,--wrap=open,--wrap=ioctl,--wrap=close -o runtime-caps-test
./runtime-caps-test
bash %{SOURCE3} "$PWD"
bash %{SOURCE4} "$PWD"

%install
%meson_install
rm -r %{buildroot}%{_datadir}/doc/libva-v4l2
rm -r %{buildroot}%{_datadir}/licenses/libva-v4l2
install -m 0644 %{SOURCE2} UKE-README.md

%files
%license LICENSE
%doc UKE-README.md
%{_libdir}/dri/v4l2_drv_video.so
%{_libdir}/dri/msm_drv_video.so

%changelog
* Sun Oct 25 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-0.4.20261025git6d03c3f
- Retain the upstream MSM alias for default libva backend discovery.
- Keep native codec output, browser integration and sandbox acceptance separate.

* Sun Oct 25 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-0.3.20261025git6d03c3f
- Validate requested bitrate against the current Iris control range and step.
- Verify returned Q16 frame rates and let Iris enforce codec and workload limits.
- Cover high and fractional frame rates, substituted rates and ioctl failures.

* Sun Oct 25 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-0.2.20261025git6d03c3f
- Select P010 input and matching HEVC Main10 parameters without 8-bit conversion.
- Require runtime P010 and Main10 profile-menu support before advertising encode.
- Test profile/depth refusal and padded P010 CPU-copy precision on the host.
- Keep native codec and GPU/direct-import validation separate from source checks.

* Sun Oct 25 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.0.0-0.1.20261025git6d03c3f
- Pin the upstream backend and query runtime codec/profile availability.
- Keep driver selection opt-in and FastCV VPP disabled for platform bring-up.
