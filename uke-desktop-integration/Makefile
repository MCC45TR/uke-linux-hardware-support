SHELL := /bin/bash
.PHONY: validate srpm
.PHONY: displayport-audio-srpm
.PHONY: pipewire-monitor-srpm
.PHONY: speaker-orientation-srpm
BUILD_DIR ?= build
speaker-orientation-srpm:
	mkdir -p $(BUILD_DIR)/speaker-orientation/rpmbuild/{SOURCES,SRPMS,SPECS}
	bash tests/speaker-orientation.sh
	tar --sort=name --mtime=@1793059200 --owner=0 --group=0 --numeric-owner --transform='s,^,uke-speaker-orientation-0.1.0/,' -czf $(BUILD_DIR)/speaker-orientation/rpmbuild/SOURCES/uke-speaker-orientation-0.1.0.tar.gz LICENSE SPEAKER-ORIENTATION.md src/uke-speaker-orientation.cpp src/uke-speaker-orientation.desktop tests/speaker-orientation.cpp tests/speaker-orientation.sh
	cp packaging/uke-speaker-orientation.spec $(BUILD_DIR)/speaker-orientation/rpmbuild/SPECS/
	rpmbuild -bs --target aarch64 --define '_topdir $(abspath $(BUILD_DIR))/speaker-orientation/rpmbuild' --define '_buildhost copr-builder' $(BUILD_DIR)/speaker-orientation/rpmbuild/SPECS/uke-speaker-orientation.spec
pipewire-monitor-srpm:
	bash scripts/build-pipewire-monitor-srpm.sh "$(FEDORA_PIPEWIRE_SRPM)" "$(abspath $(BUILD_DIR))/pipewire-monitor/rpmbuild"
displayport-audio-srpm:
	mkdir -p $(BUILD_DIR)/displayport-audio/rpmbuild/{SOURCES,SRPMS,SPECS}
	bash tests/audio-policy.sh
	tar --sort=name --mtime=@1792800000 --owner=0 --group=0 --numeric-owner --transform='s,^,uke-displayport-audio-0.1.0/,' -czf $(BUILD_DIR)/displayport-audio/rpmbuild/SOURCES/uke-displayport-audio-0.1.0.tar.gz LICENSE src/ucm2 src/wireplumber tests/audio-policy.sh tests/audio-policy.cpp
	cp packaging/uke-displayport-audio.spec $(BUILD_DIR)/displayport-audio/rpmbuild/SPECS/
	rpmbuild -bs --define '_topdir $(abspath $(BUILD_DIR))/displayport-audio/rpmbuild' --define '_buildhost copr-builder' $(BUILD_DIR)/displayport-audio/rpmbuild/SPECS/uke-displayport-audio.spec
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false and (.expected_packages | length > 0)' manifests/component.json >/dev/null
srpm:
	@echo 'No validated Uke package payload yet; complete the manifest acceptance gates.' >&2
	@exit 1
