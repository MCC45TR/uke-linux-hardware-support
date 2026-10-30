SHELL := /bin/bash
.PHONY: all check validate srpm
version := 0.2.0
outdir ?= $(CURDIR)/build/srpm
all:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr
	cmake --build build --parallel
check: all
	ctest --test-dir build --output-on-failure
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false and (.expected_packages | length > 0)' manifests/component.json >/dev/null
srpm:
	mkdir -p build/rpmbuild/{SOURCES,SPECS,SRPMS}
	tar --sort=name --mtime=@1793318400 --owner=0 --group=0 --numeric-owner --transform='s#^#plasma-uke-kcm-$(version)/#' -czf build/rpmbuild/SOURCES/plasma-uke-kcm-$(version).tar.gz CMakeLists.txt kcm_uke.json src ui tests po LICENSE README.md
	cp packaging/plasma-uke-kcm.spec build/rpmbuild/SPECS/
	rpmbuild -bs --nodeps --define '_buildhost uke-builder' --define '_topdir $(CURDIR)/build/rpmbuild' build/rpmbuild/SPECS/plasma-uke-kcm.spec
	mkdir -p '$(outdir)'
	cp build/rpmbuild/SRPMS/*.src.rpm '$(outdir)/'
