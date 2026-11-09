#!/usr/bin/bash
# Execute the actual reporter with fake command results. No private/unit data.
set -Eeuo pipefail
source_file=$(realpath "${1:?Patched preflight script}")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$work/bin"
sed "s|/usr/bin/uke-speaker-runtime|$work/absent-runtime|g" "$source_file" > "$work/preflight"
cat > "$work/bin/rpm" <<'RPM'
#!/usr/bin/bash
[[ $# == 3 && $1 == -q && $2 == --whatprovides && $3 == 'uke-speaker-runtime(gain-readiness-v1)' ]] || exit 98
case $UKE_PROVIDER_TEST in
 legacy) printf 'uke-speaker-runtime-0.4-3.fc46.aarch64\n';;
 folded) printf 'uke-control-2.14.0-1.fc46.aarch64\n';;
 missing) exit 1;;
 *) exit 99;;
esac
RPM
cat > "$work/bin/amixer" <<'ALSA'
#!/usr/bin/bash
printf "numid=1,iface=MIXER,name='FS19xx Protection State'\nnumid=2,iface=MIXER,name='FS19xx Runtime Provisioning'\n"
ALSA
cat > "$work/bin/findmnt" <<'MOUNT'
#!/usr/bin/bash
printf 'ro,nosuid,nodev,noexec\n'
MOUNT
chmod 700 "$work/bin/"*
for provider in legacy folded missing;do
 result=0
 PATH=$work/bin:$PATH UKE_PROVIDER_TEST=$provider bash "$work/preflight" --json > "$work/result.json" || result=$?
 [[ $result == 2 ]]
 jq -e --arg provider "$provider" '.runtime_installed==($provider!="missing") and .gain_243_admissible==false and
  .decision=="blocked" and (.native_readiness==null) and
  (((.blockers|index("runtime_not_installed"))!=null)==($provider=="missing"))' "$work/result.json" >/dev/null
done
printf 'Actual speaker preflight accepts legacy/folded capability providers and refuses missing; gain243 remains blocked.\n'
