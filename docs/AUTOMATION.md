# Release delivery and automatic rebuilds

COPR uses the `main` branch, `.copr/Makefile`, `make_srpm` and the normal RPM spec.
Source generation downloads the locked GitHub assets, checks every SHA-256 and
verifies the public Uke validation manifest. Binary packaging runs offline.
Source snapshots are offered alongside the original release and identified in
each package's lock file; the SRPM rebuilds image delivery rather than compiling
OrangeFox from scratch.

A GitHub push webhook rebuilds accepted packaging changes. The daily workflow
calls `make track-stable`, selecting the newest published non-draft,
non-prerelease release. It admits only the explicit stable tag format
`rMAJOR.MINOR-uke.YYYYMMDD`, all ten delivery assets and both corresponding
source snapshots with GitHub SHA-256 digests. The artifact manifest must retain
the reviewed Uke firmware profile and pass compile, header, ZIP, installer,
source, privacy and Python gates. Unknown profiles, missing assets and disguised
alphas fail before changing the source pin.

When eligible stable inputs change, the workflow audits both image ramdisks,
the nonempty extracted target, ELF dependencies and the ZIP on its host runner,
then commits the new lock/spec and explicitly requests a COPR build. This
does not execute any image or installer. `COPR_RECOVERY_BUILD_HOOK` is a GitHub
secret and is excluded from public source and reports. `releases_file=PATH` is
an optional local metadata fixture input; scheduled runs always use the project
GitHub API. The alpha pin remains intentional until a stable release qualifies.

DNF makes accepted images available through ordinary package upgrades. It
does not flash them, change Android firmware or schedule device writes. The
versioned image directory and manifest allow inspection before any separately
authorized device test. Physical acceptance never follows automatically from
a release label or COPR job.

Same-version payload changes need an increasing RPM release. A new stable
recovery version resets that release counter. Unimplemented Nabu counterparts
remain initial repositories until a real Uke payload and its package tests exist.

Reference: [COPR source methods and webhook protocol](https://docs.pagure.org/copr.copr/user_documentation.html).
