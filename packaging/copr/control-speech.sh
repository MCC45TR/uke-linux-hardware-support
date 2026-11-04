#!/usr/bin/env bash
# COPR source producer. Preserve the installed control cohort's other features.
set -Eeuo pipefail
[[ $(rpm --eval '%{_host_cpu}') == aarch64 ]]
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/../.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$output" "$stage/retained" "$stage/overlay"
curl -fLsS --retry 2 --max-time 120 'https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11254600/uke-control-2.8.1-11.fc46.src.rpm' -o "$stage/retained.src.rpm"
printf '%s  %s\n' ae799861750aca1ed7576e33d5319197499b889ef8a5eedf5287badfa2d2fe2e "$stage/retained.src.rpm" | sha256sum -c -
(cd "$stage/retained"; rpm2cpio "$stage/retained.src.rpm" | cpio -idm --quiet)
cp "$stage/retained/"* "$output/"
for relative in src/microphone.cpp src/microphone-engine.hpp src/microphone-graph.hpp \
 src/uke-microphone-policy.lua src/uke-microphone.lua packaging/96-uke-microphone-policy.conf \
 packaging/microphone-speech.mk tests/microphone-policy.lua tests/microphone-runtime.lua \
 tests/microphone-graph.cpp tests/microphone-engine.cpp tests/microphone-pipewire.sh \
 packaging/merge-speech-catalog.sh MICROPHONE.md; do
    install -Dm644 "$owner/uke-control/$relative" "$stage/overlay/$relative"
done
install -Dm644 "$owner/uke-control/src/client-catalog.cpp" "$stage/overlay/packaging/microphone-catalog-template.cpp"
install -m644 "$owner/uke-control/packaging/microphone-retained-parents.sha256" "$output/uke-control-speech-parents.sha256"
git -C "$owner" rev-parse HEAD > "$stage/overlay/SPEECH-SOURCE-COMMIT"
tar -czf "$output/uke-control-speech-overlay.tar.gz" -C "$stage/overlay" .
spec=$output/uke-control.spec
sed -i -e 's/Release: 11.fc46/Release: 12.speech1.fc46/g' -e 's/2.8.1-11.fc46/2.8.1-12.speech1.fc46/g' "$spec"
sed -i '/^Source5:/a Source6: uke-control-speech-overlay.tar.gz\nSource7: uke-control-speech-parents.sha256\nBuildRequires: pkgconfig(libspa-0.2) pipewire pipewire-libs pipewire-utils wireplumber dbus-daemon' "$spec"
sed -i '/^Recommends: uke-battery-observer/a Recommends: /usr/lib64/spa-0.2/aec/libspa-aec-webrtc.so' "$spec"
# Insert into %prep only after all original nested source extractions.
awk '
 /^%build$/ {
  print "("
  print "cd \"%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1\""
  print "sha256sum --check --strict %{SOURCE7}"
  print "tar -xzf %{SOURCE6}"
  print "bash packaging/merge-speech-catalog.sh packaging/microphone-catalog-template.cpp src/client-catalog.cpp"
  print "printf '\''\\ninclude packaging/microphone-speech.mk\\n'\'' >> Makefile"
  print ")"
 }
 { print }
' "$spec" > "$stage/spec"
mv "$stage/spec" "$spec"
rm -f "$output/"*.src.rpm
rpmbuild -bs --define "_sourcedir $output" --define "_srcrpmdir $output" "$spec"
