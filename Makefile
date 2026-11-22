SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate test-native native-guis check-guis
components := uke-core-meta uke-desktop-metas uke-desktop-packaging uke-orangefox-packaging uke-boot uke-camera uke-desktop-integration uke-hardware-provenance uke-platform-runtime uke-sensors plasma-uke-kcm xiaomi-uke-firmware uke-control
validate:
	jq -e '.schema_version==1 and (.imports|length==12) and all(.imports[]; .imported_commit|test("^[a-f0-9]{40}$$"))' manifests/source-imports.json >/dev/null
	for component in $(components); do $(MAKE) -C "components/$$component" validate; done
test-native:
	$(MAKE) -C components/uke-desktop-metas test-applet-load
	bash components/uke-orangefox-packaging/tests/release-contract.sh
	$(MAKE) -C components/uke-sensors test-native
	$(MAKE) -C components/uke-desktop-packaging test-native
	$(MAKE) -C components/uke-boot test-native
	$(MAKE) -C components/uke-boot test-usb
	$(MAKE) -C components/uke-desktop-integration test-native
	$(MAKE) -C components/uke-platform-runtime test-native
	$(MAKE) -C components/uke-control check
	$(MAKE) -C components/xiaomi-uke-firmware test-admission

# Optional desktop applications; their toolkit dependencies stay out of the
# core backend/TUI build and release-image selections.
native-guis:
	$(MAKE) -C components/uke-control build/libuke-control-client.so.1
	cmake -S components/uke-control-gtk -B components/uke-control-gtk/build -DUKE_CONTROL_SOURCE="$(CURDIR)/components/uke-control"
	cmake --build components/uke-control-gtk/build -j4
	cmake -S components/uke-control-qt -B components/uke-control-qt/build -DUKE_CONTROL_SOURCE="$(CURDIR)/components/uke-control"
	cmake --build components/uke-control-qt/build -j4
check-guis: native-guis
	ctest --test-dir components/uke-control-gtk/build --output-on-failure
	ctest --test-dir components/uke-control-qt/build --output-on-failure
