Name: uke-ssc-fileservice
Version: 2026.09.21
Release: 2%{?dist}
Summary: Native Uke SSC file service with a disposable registry
License: GPL-3.0-or-later
URL: https://github.com/linux-msm/hexagonrpc
Source0: hexagonrpc-510c87e5.tar.gz
Source1: uke-ssc-root.service
Source2: uke-ssc-sensors.service
Source3: uke-ssc.conf
Source4: iio-ssc-order.conf
Source5: hexagon-registry.cpp
Source6: hexagon-shell.c
Patch0: 0001-select-sequence-type.patch
Patch1: 0002-verbose-native-u64.patch
Patch2: 0003-map-oem-odm-sensor-config.patch
Patch3: 0004-reject-mapped-symlinks.patch
Patch4: 0005-reject-incomplete-shell-images.patch
BuildRequires: gcc gcc-c++ systemd-rpm-macros
Requires: iio-sensor-proxy >= 3.9-4.uke3
Requires: /usr/share/uke-sensors/odm/json.lst
Requires: /usr/share/uke-sensors/sns_reg.conf
Requires: /usr/lib/firmware/qcom/uke/sensors/sns_tppe.so
Requires: /usr/bin/findmnt
%{?systemd_requires}

%description
Serve the pinned upstream FastRPC implementation from a private RAM tree.
Copy calibration from a read-only persist mount; firmware writes remain in
RAM. Device-local firmware/configuration is required separately and is not
redistributed here. Startup, restart and suspend remain device test gates.

%prep
echo 'ed7b0e704a5e8c20b77b013b4b34f3cddbe41d4cf546edc73f029501f7ebdec5  %{SOURCE0}' | sha256sum -c -
%setup -q -c -T
tar -xf %{SOURCE0} --strip-components=1
%autopatch -p1

%build
%set_build_flags
gcc $CFLAGS $LDFLAGS -std=gnu11 -Wall -Wextra -Werror -Wno-unused-parameter \
    -I include libhexagonrpc/*.c libhexagonrpc/interface/*.c hexagonrpcd/*.c \
    hexagonrpcd/interface/*.c -o uke-hexagonrpcd

%check
mkdir contract
for name in hexagonfs hexagonfs_mapped hexagonfs_virt_dir rpcd_builder; do
    gcc -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -I include \
        -c hexagonrpcd/$name.c -o contract/$name.o || exit
done
g++ -std=c++20 -O2 -Wall -Wextra -Werror -I hexagonrpcd %{SOURCE5} contract/*.o -o contract/registry
./contract/registry
sed -n '/^static int create_shell_pd(/,/^}/p' hexagonrpcd/rpcd.c > contract/shell-function.h
gcc -std=gnu11 -O2 -Wall -Wextra -Werror -I contract %{SOURCE6} -o contract/shell
./contract/shell

%install
install -Dm755 uke-hexagonrpcd %{buildroot}/usr/libexec/uke-hexagonrpcd
install -Dm644 %{SOURCE1} %{buildroot}%{_unitdir}/uke-ssc-root.service
install -Dm644 %{SOURCE2} %{buildroot}%{_unitdir}/uke-ssc-sensors.service
install -Dm644 %{SOURCE3} %{buildroot}%{_tmpfilesdir}/uke-ssc.conf
install -Dm644 %{SOURCE4} %{buildroot}%{_unitdir}/iio-sensor-proxy.service.d/uke-ssc.conf

%post
%systemd_post uke-ssc-root.service uke-ssc-sensors.service

%preun
%systemd_preun uke-ssc-root.service uke-ssc-sensors.service

%postun
%systemd_postun_with_restart uke-ssc-root.service uke-ssc-sensors.service

%files
%license COPYING
/usr/libexec/uke-hexagonrpcd
%{_unitdir}/uke-ssc-root.service
%{_unitdir}/uke-ssc-sensors.service
%{_unitdir}/iio-sensor-proxy.service.d/uke-ssc.conf
%{_tmpfilesdir}/uke-ssc.conf

%changelog
* Wed Oct 14 2026 Uke Linux project <75160848+MCC45TR@users.noreply.github.com> - 2026.09.21-1
- Package native SSC file transport and root-stage ordering for device tests.
- Reject symlinks and keep registry writes in a disposable RAM copy.
