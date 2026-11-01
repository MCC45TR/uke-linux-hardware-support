SHELL := /bin/bash
.PHONY: validate srpm srpm-gpu
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false and (.expected_packages | length > 0)' manifests/component.json >/dev/null
srpm:
	@echo 'No validated Uke package payload yet; complete the manifest acceptance gates.' >&2
	@exit 1
srpm-gpu:
	mkdir -p build/gpu-rpm/{SOURCES,SPECS,SRPMS}
	cp src/modules-load.d/uke-gpu.conf build/gpu-rpm/SOURCES/uke-gpu-modules.conf
	cp src/modprobe.d/uke-gpu.conf build/gpu-rpm/SOURCES/uke-gpu-options.conf
	cp LICENSE build/gpu-rpm/SOURCES/
	cp packaging/uke-gpu-defaults.spec build/gpu-rpm/SPECS/
	rpmbuild -bs --define "_topdir $(CURDIR)/build/gpu-rpm" build/gpu-rpm/SPECS/uke-gpu-defaults.spec
