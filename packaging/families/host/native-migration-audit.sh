#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Read-only inventory. This tool never installs packages or retires producers.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
policy=$root/packaging/families/consolidation/native-migration.json
case ${1:-} in
    snapshot)
        [[ $# == 2 && ! -e $2 && ! -L $2 ]]
        # Include original names: a fork need not have an uke- prefix.
        rpm -qa --qf '%{NAME}\t%{VERSION}-%{RELEASE}.%{ARCH}\t%{SOURCERPM}\t%{VENDOR}\n' > "$2"
        ;;
    audit)
        [[ $# == 4 && -s $2 && -s $3 && ! -e $4 && ! -L $4 ]]
        stage=$(mktemp -d)
        trap 'rm -rf -- "$stage"' EXIT
        jq -e -n --slurpfile policy "$policy" --slurpfile copr "$2" --rawfile rpms "$3" \
            -f "$root/packaging/families/host/native-migration-audit.jq" > "$stage/report"
        install -m600 "$stage/report" "$4"
        jq -e '.coverage_complete' "$4" >/dev/null || {
            echo 'Unmapped packages/producers require review; no retirement is admitted.' >&2
            exit 2
        }
        ;;
    *) echo "Usage: $0 snapshot NEW_RPM_TSV | audit COMPLETE_COPR_JSON RPM_TSV NEW_REPORT_JSON" >&2; exit 1;;
esac
