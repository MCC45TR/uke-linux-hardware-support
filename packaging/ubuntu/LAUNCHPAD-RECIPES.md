# Launchpad recipe handoff

Recorded 1 November 2026. The first admitted recipe source is
`uke-speaker-firmware`, the attributed generic 4,828-byte FS19xx preset.
No device calibration or unrelated firmware is included.

- Public Git import URL: `https://github.com/MCC45TR/uke-linux-hardware-support.git`
- Launchpad repository: `~mcs45tr/uke-linux-hardware-support/+git/uke-linux-hardware-support`
- Imported branch: `launchpad/uke-speaker-firmware`
- Branch commit: `482f7d1170eeb8d8ffce38e53ec5173bb6890e0f`
- Required upstream tag: `upstream/3.0.304.0`
- Recipe name: `uke-speaker-firmware-tenacious-unstable`
- Recipe text: [uke-speaker-firmware.recipe](uke-speaker-firmware.recipe)
- Destination: `mcs45tr/uke-linux-unstable`
- Series: Tenacious only; PPA processors: ARM64 only
- Build daily: enabled; request the first build after the import completes
- Stable: manual qualified promotion; no daily recipe

The parent confirmed creation of the public project and a reviewed, queued
Git import on 1 November. Import completion and the first recipe build remain
pending. The import form requires a project; a personal `+git` path is not the
configured target. Preserve the `pristine-tar` branch and source tags when
importing: exact original archives are available without native-format fallback.
The exported branch is a generated
package view, with unchanged shared payload and a Debian overlay, not a second
application implementation.

[Native ARM64 validation](https://github.com/MCC45TR/uke-linux-hardware-support/actions/runs/41308699685)
at packaging commit 99bf892 passed real DEB construction, direct payload
inspection, installation, reinstallation with a local conffile change,
remove-with-conffile-preservation, purge and recipe source construction using
official Tenacious git-build-recipe 0.3.7build1. Recipe validation substituted
the local exported repository for the pending Launchpad import URL; the
recipe grammar and version expression were identical. This is not evidence of
a successful Launchpad build, publication or speaker playback/protection.

The official recipe tool and its upstream Python dependencies ran on the
server only. It is required to exercise Launchpad's recipe syntax and source
construction; no practical native replacement reproduces that service tool.
No Python is included in this package. User private signing material was not
used. Launchpad recipe creation remains with the parent session;
no new OAuth token or SSH credential was created.

Future source changes require a refreshed, validated export using
`export-recipe-source.sh` on a pristine assembled source directory. Preserve
the immutable upstream tag for the same upstream version. Advance the
package branch without rewriting published history. Daily Launchpad builds
only see changes that have reached this reviewed package branch. The other
source routes are not automatically enrolled by this first-package setup.
Their validated branches, recipes and dependency gates are recorded in
[RECIPE-SOURCE-EXPORTS.md](RECIPE-SOURCE-EXPORTS.md).

Reference: [Launchpad recipe manual](https://ubuntu.com/docs/launchpad/user/how-to/source-package-recipe/).
