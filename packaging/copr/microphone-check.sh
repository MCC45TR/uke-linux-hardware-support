#!/usr/bin/env bash
# COPR-only executable microphone checks; installs no policy or audio service.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/../.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
mkdir -p "$output"
make -C "$owner/uke-control" check-microphone
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
git -C "$owner" rev-parse HEAD > "$stage/SOURCE-COMMIT"
printf '%s\n' 'COPR source checks: microphone policy and lifecycle, actual graph reader, actual WebRTC synthetic processing, and isolated PipeWire capture-only/duplex frame flow. No hardware or acoustic acceptance.' > "$stage/VALIDATION"
tar -czf "$output/uke-microphone-check-0.1.tar.gz" -C "$stage" .
cat > "$output/uke-microphone-check.spec" <<'SPEC'
Name: uke-microphone-check
Version: 0.1
Release: 1%{?dist}
Summary: Source-validation receipt for Uke microphone processing
License: MIT
BuildArch: noarch
Source0: %{name}-%{version}.tar.gz
%description
COPR check receipt only. No application, policy, firmware or calibration payload.
%prep
%setup -q -c
%install
mkdir -p %{buildroot}%{_datadir}/doc/%{name}
install -m644 VALIDATION SOURCE-COMMIT %{buildroot}%{_datadir}/doc/%{name}/
%files
%doc %{_datadir}/doc/%{name}/VALIDATION
%doc %{_datadir}/doc/%{name}/SOURCE-COMMIT
SPEC
rpmbuild -bs --define "_sourcedir $output" --define "_srcrpmdir $output" "$output/uke-microphone-check.spec"
