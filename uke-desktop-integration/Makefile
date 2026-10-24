SHELL := /bin/bash
.PHONY: validate srpm
.PHONY: displayport-audio-srpm
BUILD_DIR ?= build
displayport-audio-srpm:
	mkdir -p $(BUILD_DIR)/displayport-audio/rpmbuild/{SOURCES,SRPMS,SPECS}
	tar --sort=name --mtime=@1792800000 --owner=0 --group=0 --numeric-owner --transform='s,^,uke-displayport-audio-0.1.0/,' -czf $(BUILD_DIR)/displayport-audio/rpmbuild/SOURCES/uke-displayport-audio-0.1.0.tar.gz LICENSE src/ucm2 src/wireplumber
	cp packaging/uke-displayport-audio.spec $(BUILD_DIR)/displayport-audio/rpmbuild/SPECS/
	rpmbuild -bs --define '_topdir $(abspath $(BUILD_DIR))/displayport-audio/rpmbuild' --define '_buildhost copr-builder' $(BUILD_DIR)/displayport-audio/rpmbuild/SPECS/uke-displayport-audio.spec
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false and (.expected_packages | length > 0)' manifests/component.json >/dev/null
srpm:
	@echo 'No validated Uke package payload yet; complete the manifest acceptance gates.' >&2
	@exit 1
