SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate srpm test-native
outdir ?= $(CURDIR)/build/srpm
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false and (.expected_packages | index("uke-adaptive-display") != null)' manifests/component.json >/dev/null
	bash tests/adaptive-display.sh
srpm: validate
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	tar --sort=name --mtime=@1791936000 --owner=0 --group=0 --numeric-owner --transform='s,^,uke-adaptive-display-0.1.0/,' -cf - LICENSE docs/ADAPTIVE-DISPLAY.md packaging/uke-adaptive-display.target packaging/80-uke-adaptive-display.preset | gzip -n > "$$top/SOURCES/uke-adaptive-display-0.1.0.tar.gz"
	cp packaging/uke-adaptive-display.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --define "_topdir $$top" "$$top/SPECS/uke-adaptive-display.spec"
	install -m644 "$$top/SRPMS/"*.src.rpm '$(outdir)/'

.PHONY: power-srpm
power-srpm:
	archive=referances/power-profiles-daemon/power-profiles-daemon-0.30.tar.gz
	echo "90f2c95024861d9f4062abedd514084939224e7a49edd468f001af7260ec8e6c  $$archive" | sha256sum -c -
	top="$$(realpath -m build/power-srpm/rpmbuild)"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	cp "$$archive" "$$top/SOURCES/"
	stage="$$(mktemp -d build/power-source.XXXXXX)"
	mkdir -p "$$stage/patches" "$$stage/tests" "$$stage/src" "$$stage/packaging" "$$stage/docs"
	cp src/ppd-driver-uke-cpufreq.* "$$stage/src/"
	cp patches/0001-ppd-native-uke-cpu-driver.patch "$$stage/patches/"
	cp tests/ppd-cpufreq.cpp "$$stage/tests/"
	cp packaging/60-uke-cpufreq.conf "$$stage/packaging/"
	cp packaging/uke-power-profile.conf packaging/uke-power-profile.cil "$$stage/packaging/"
	cp docs/POWER-PROFILES.md "$$stage/docs/"
	tar --sort=name --mtime=@1792195200 --owner=0 --group=0 --numeric-owner -C "$$stage" -cf - . | gzip -n > "$$top/SOURCES/uke-power-profile-backend-0.1.0.tar.gz"
	cp packaging/uke-power-profile-backend.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define "_topdir $$top" "$$top/SPECS/uke-power-profile-backend.spec"
