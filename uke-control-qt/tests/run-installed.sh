#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail
export UKE_QT_TEST_APP=/usr/bin/uke-control-qt
export UKE_QT_TEST_FIXTURE=/usr/share/uke-control-qt/tests/tui.json
export QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-offscreen}
export QT_QUICK_BACKEND=${QT_QUICK_BACKEND:-software}
test_dir=/usr/libexec/uke-control-qt/tests
"$test_dir/uke-control-qt-tests"
bash "$test_dir/cli-smoke.sh" "$UKE_QT_TEST_APP"
"$test_dir/uke-control-qt-appearance-tests"
"$test_dir/uke-control-kirigami-tests"
"$test_dir/uke-control-kernel-manager-tests"
