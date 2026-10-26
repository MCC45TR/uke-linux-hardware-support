#!/usr/bin/env bash
# Source the actual Fedora-style configuration with controlled host identities.
set -Eeuo pipefail
config=$(realpath "${1:?Pass chromium.conf}")
for test_arch in aarch64 x86_64; do
  for desktop in wayland x11; do
    (
      arch() { printf '%s\n' "$test_arch"; }
      systemd-detect-virt() { printf 'none\n'; }
      if [[ $desktop == wayland ]]; then export WAYLAND_DISPLAY=wayland-test;
      else unset WAYLAND_DISPLAY; fi
      unset CHROMIUM_FLAGS ENABLE_FEATURES DISABLE_FEATURES
      # shellcheck disable=SC1090
      source "$config"
      case "$CHROMIUM_FLAGS" in
        *--ignore-gpu-blocklist*|*--disable-gpu-driver-bug-workaround*|*--no-sandbox*|*--disable-gpu-sandbox*)
          printf 'Unsafe graphics/sandbox default\n' >&2; exit 1;;
      esac
      case "$ENABLE_FEATURES" in
        *AcceleratedVideo*|*VaapiIgnoreDriverChecks*|*Graphite*|*DrDC*|*DirectRenderingDisplayCompositor*)
          printf 'Unqualified video/rendering feature force\n' >&2; exit 1;;
      esac
      [[ $CHROMIUM_FLAGS != *--enable-zero-copy* ]]
      if [[ $test_arch == aarch64 ]]; then
        [[ $CHROMIUM_FLAGS == *--gpu-sandbox-start-early* &&
           $CHROMIUM_FLAGS == *--gpu-sandbox-failures-fatal=yes* ]]
      else [[ $CHROMIUM_FLAGS != *--gpu-sandbox-start-early* ]]; fi
      if [[ $desktop == wayland ]]; then
        [[ $CHROMIUM_FLAGS == *--ozone-platform=wayland* ]]
      else [[ $CHROMIUM_FLAGS != *--ozone-platform=wayland* ]]; fi
      [[ $DISABLE_FEATURES == *OptimizationGuideOnDeviceModel* && $ENABLE_FEATURES == *AllowQt* ]]
    )
  done
done
printf 'ARM64 early/fatal sandbox and neutral Wayland/X11 feature defaults passed (host shell only)\n'
