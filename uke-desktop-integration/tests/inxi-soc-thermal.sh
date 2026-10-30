#!/usr/bin/env bash
# Exercise the real classifier without probing hardware or changing DT names.
set -Eeuo pipefail
source_file=${1:?Pass the inxi source file}
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
sed -n '/^sub soc_type {/,/^sub pci_class {/p' "$source_file" | sed '$d' > "$work/check.pl"
cat >> "$work/check.pl" <<'PERL'
my @cases = (
    ['display-thermal-sensor', 'generic-adc-thermal', 'generic_adc_thermal', 'thermal'],
    ['wifi-thermal-sensor', 'generic-adc-thermal', 'generic_adc_thermal', 'thermal'],
    ['gpu-thermal', 'vendor', '', 'thermal'],
    ['wifi_thermal_sensor', 'vendor', '', 'thermal'],
    ['display', 'vendor', 'generic_adc_thermal', 'thermal'],
    ['display-subsystem', 'qcom', 'msm_mdss', 'display'],
    ['display-controller', 'qcom', 'msm_dpu', 'display'],
    ['gpu', 'qcom', 'msm', 'display'],
    ['wifi', 'vendor', 'wlan', 'wifi'],
    ['ethernet', 'vendor', 'macb', 'ethernet'],
    ['hdmi', 'vendor', '', 'hdmi'],
    ['sound-card', 'vendor', 'snd_soc', 'audio'],
    ['wlan', 'vendor', 'reg_fixed_voltage', 'regulator'],
);
for my $case (@cases) {
    my ($name, $info, $driver, $expected) = @$case;
    my $actual = soc_type($name, $info, $driver);
    die "$name: expected $expected, got $actual\n" if $actual ne $expected;
}
print scalar(@cases), " SoC classifier cases passed\n";
PERL
perl "$work/check.pl"
