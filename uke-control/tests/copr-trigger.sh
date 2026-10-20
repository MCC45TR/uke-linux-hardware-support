#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Local Git and stub HTTP only; never contacts COPR or another Git service.
set -Eeuo pipefail
source_root=$(pwd)
test_root=$(mktemp -d)
trap 'rm -rf -- "$test_root"' EXIT
mkdir -p "$test_root/work/uke-control/packaging" "$test_root/work/uke-control/src" "$test_root/work/.copr" "$test_root/bin"
cd "$test_root/work"
git init -q -b main
git config user.name 'Fixture'
git config user.email fixture@example.invalid
cat >uke-control/packaging/uke-control.spec <<'SPEC'
Name: uke-control
Version: 0.1.0
Release: 1%{?dist}
Summary: Fixture
License: MIT
%description
Fixture only.
%files
SPEC
printf '%s\n' base >uke-control/src/value
printf '%s\n' rule >.copr/Makefile
git add .
git commit -qm base
base=$(git rev-parse HEAD)
git init -q --bare "$test_root/origin.git"
git remote add origin "$test_root/origin.git"
git push -q origin main
cat >"$test_root/bin/curl" <<'STUB'
#!/usr/bin/env bash
set -Eeuo pipefail
if [[ $* == *api_3/build/list* ]]; then
    case $FIXTURE_MODE in
        active) printf '%s\n' '{"items":[{"state":"running","source_package":null}]}' ;;
        published) printf '%s\n' '{"items":[{"state":"succeeded","source_package":{"version":"0.1.0-2.fc46"}}]}' ;;
        *) printf '%s\n' '{"items":[]}' ;;
    esac
else
    printf '%s\n' POST >>"$FIXTURE_REQUESTS"
    [[ $FIXTURE_MODE != uncertain ]] || exit 28
    printf '%s\n' 4242
fi
STUB
chmod +x "$test_root/bin/curl"
export PATH="$test_root/bin:$PATH"
export FIXTURE_MODE=none FIXTURE_REQUESTS="$test_root/requests"
export COPR_SUPPORT_BUILD_HOOK_BASE=https://copr.fedorainfracloud.org/webhooks/custom/1/fixture
export UKE_CONTROL_BEFORE=$base UKE_CONTROL_COMMIT=$base
script="$source_root/host/request-copr.sh"
bash "$script"
test ! -e "$FIXTURE_REQUESTS"
printf '%s\n' changed >uke-control/src/value
git add .;git commit -qm unversioned
export UKE_CONTROL_COMMIT=$(git rev-parse HEAD)
if bash "$script"; then exit 1; fi
test ! -e "$FIXTURE_REQUESTS"
sed -i 's/Release: 1/Release: 2/' uke-control/packaging/uke-control.spec
git add .;git commit -qm release2
git push -q origin main
export UKE_CONTROL_COMMIT=$(git rev-parse HEAD)
for mode in active published; do FIXTURE_MODE=$mode bash "$script"; done
test ! -e "$FIXTURE_REQUESTS"
bash "$script"
test "$(wc -l <"$FIXTURE_REQUESTS")" = 1
if FIXTURE_MODE=uncertain bash "$script"; then exit 1; fi
test "$(wc -l <"$FIXTURE_REQUESTS")" = 2
# A newer package tree must not be built by an older validation.
printf '%s\n' advanced >uke-control/src/value
git add .;git commit -qm advanced
git push -q origin main
git checkout -q "$UKE_CONTROL_COMMIT"
bash "$script"
test "$(wc -l <"$FIXTURE_REQUESTS")" = 2
# Source reverts need a new package revision, rather than republishing old RPM identities.
git checkout -q main
previous=$(git rev-parse HEAD)
sed -i 's/Release: 2/Release: 1/' uke-control/packaging/uke-control.spec
git add .;git commit -qm backwards
export UKE_CONTROL_BEFORE=$previous UKE_CONTROL_COMMIT=$(git rev-parse HEAD)
if bash "$script"; then exit 1; fi
test "$(wc -l <"$FIXTURE_REQUESTS")" = 2
mkdir -p packaging/families
cat >packaging/families/families.json <<'JSON'
{"families":[{"name":"uke-core","version":"0.1.0","release":1,"watch_paths":["uke-control/src","uke-control/packaging"]}]}
JSON
printf '%s\n' '[]' >packaging/families/source-inputs.json
git add .; git commit -qm family-base
export UKE_CONTROL_BEFORE=$(git rev-parse HEAD)
printf '%s\n' shared-change >>.copr/Makefile
git add .; git commit -qm shared-only
export UKE_CONTROL_COMMIT=$(git rev-parse HEAD)
bash "$script"
test "$(wc -l <"$FIXTURE_REQUESTS")" = 2
printf '%s\n' own-change >>uke-control/src/value
git add .; git commit -qm family-unversioned
export UKE_CONTROL_COMMIT=$(git rev-parse HEAD)
if bash "$script"; then exit 1; fi
test "$(wc -l <"$FIXTURE_REQUESTS")" = 2
printf '%s\n' 'PASS: unchanged source, NVR admission, deduplication, uncertain POST, source advance and family-scoped shared tooling'
