# Uke desktop selections

This source family builds `uke-desktop-metas` as a real AArch64 policy metadata
RPM plus its SRPM. It requires the Uke core selection and installs an explicit
readiness profile. KDE applications must be original distribution packages;
cloning, forking and Uke application rebuilds are prohibited. Their current
Python components block complete target admission. This package installs no
graphical session or device-specific display configuration.

`make validate` and `make srpm` support source checks and complete source generation.
COPR's `.copr/Makefile` builds the source RPM from `main`; the GitHub push webhook
requests native Rawhide AArch64 compilation. Actual dependency closure and package
transactions must pass independently; graphics and tablet boot remain untested.
The package itself ships no runtime script or service and does not activate a
session. Fedora owns the dependencies' ordinary installation policies.

The [development COPR](https://copr.fedorainfracloud.org/coprs/mcc45tr/uke-linux-test/)
and [package hub](https://github.com/MCC45TR/uke-linux/blob/main/docs/PACKAGE-HUB.md)
separate packaging readiness from physical platform readiness. Nabu's reference
specification at `98188b595b42ba975f5bc238e596f330d3994ed8` contains panel-specific
profiles, runtime services and session choices; these are not Uke evidence.

Release 3 requires core release 3 with its native GNU C++ library and obsoletes
the former `kde-plasma-uke-meta` selection, whose derivative application
dependencies are withdrawn. The 603-input release-2 transaction and its
lifecycle fixtures passed, but its inherited Fedora base failed the complete
payload gate. A selected-RPM audit cannot establish the installed root's policy.
