# SPDX-License-Identifier: MIT
# Embedded RPM postun body; source template, never installed as a helper.
# Incoming Control6 captures the five service states in its pre scriptlet.
# Keep a retained provider's warm RAM mount across upgrades and reinstalls.
# If the incoming package no longer owns the mount, stop its readers before
# unmounting; incoming posttrans/refresh restores the captured service states.
if [ -d /run/systemd/system ] &&
   [ ! -f '/usr/lib/systemd/system/run-uke\x2dssc-fedora.mount' ]; then
    /usr/bin/systemctl stop uke-ssc-sensors.service uke-ssc-root.service uke-ssc-cache.service 'run-uke\x2dssc-fedora.mount' || exit $?
fi
