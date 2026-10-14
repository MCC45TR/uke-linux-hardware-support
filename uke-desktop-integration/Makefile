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
