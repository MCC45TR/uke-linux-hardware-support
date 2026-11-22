#!/bin/bash
# Describe additional pinned references present in a shared sensor source tree.
# The references retain their original license files; no runtime payload changes.
set -euo pipefail
package=${1:?package required}
output=${2:?copyright file required}
case $package in libssc|iio-sensor-proxy|uke-ssc-fileservice|uke-touchscreen|uke-front-color|uke-proximity-capture) ;; *) exit 0;; esac
cat >> "$output" <<'COPYRIGHT'

Files: source/references/libssc/* source/references/iio-sensor-proxy/* source/references/hexagonrpc-write/*
Copyright: Respective upstream contributors; see retained archive notices
License: GPL-3+
 The full GNU General Public License version 3 is available in
 /usr/share/common-licenses/GPL-3 on Debian systems.

Files: source/references/nt36532-thp/*
Copyright: See preserved pinned upstream source notices
License: Apache-2.0
 The original license is retained in the reference input directory. The full
 Apache License version 2.0 is available in /usr/share/common-licenses/Apache-2.0.
COPYRIGHT
