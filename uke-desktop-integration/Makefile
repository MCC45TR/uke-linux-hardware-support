SHELL := /bin/bash
BUILD_DIR ?= build
.PHONY: validate srpm vulkan-probe gles-probe opencl-probe mesa-libclc-srpm
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false and (.expected_packages | length > 0)' manifests/component.json >/dev/null
	jq -e '.schema_version == 1 and .date == "2026-10-18" and .donor_code_executed == false and .kernel_build_lock_changed == false and all(.sources[]; .commit | test("^[a-f0-9]{40}$$"))' manifests/graphics-research-20261018.json >/dev/null
	jq -e '.schema_version == 1 and .date == "2026-10-24" and .donor_scripts_executed == false and .kernel_build_lock_changed == false and .full_conformance_claimed == false and all(.sources[]; .commit | test("^[a-f0-9]{40}$$"))' manifests/graphics-research-20261024.json >/dev/null
srpm:
	@echo 'No validated Uke package payload yet; complete the manifest acceptance gates.' >&2
	@exit 1
vulkan-probe:
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++17 -Wall -Wextra -Werror src/vulkan-probe.cpp $(LDFLAGS) -lvulkan -o $(BUILD_DIR)/uke-vulkan-probe
gles-probe:
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++17 -Wall -Wextra -Werror src/gles-probe.cpp $(LDFLAGS) -lEGL -lGLESv2 -o $(BUILD_DIR)/uke-gles-probe
opencl-probe:
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++17 -Wall -Wextra -Werror src/opencl-probe.cpp $(LDFLAGS) -lOpenCL -o $(BUILD_DIR)/uke-opencl-probe
mesa-libclc-srpm:
	@test "$$(sha256sum referances/mesa-libclc-20261024/mesa-libclc-87f66007242ff10e9e84bf32be84f9057f6abc75.tar.gz | cut -d ' ' -f1)" = 85ed6be5a3be9ae8928bb6c19b67cd9646779fe7e6dc25e66794d43d9635280a
	mkdir -p "$(BUILD_DIR)/mesa-libclc-rpm/SOURCES" "$(BUILD_DIR)/mesa-libclc-rpm/SPECS"
	cp referances/mesa-libclc-20261024/mesa-libclc-87f66007242ff10e9e84bf32be84f9057f6abc75.tar.gz "$(BUILD_DIR)/mesa-libclc-rpm/SOURCES/"
	cp packaging/uke-mesa-libclc22-spirv.spec "$(BUILD_DIR)/mesa-libclc-rpm/SPECS/"
	rpmbuild -bs --target aarch64 --define '_buildhost uke-builder' --define "_topdir $$(realpath "$(BUILD_DIR)/mesa-libclc-rpm")" "$(BUILD_DIR)/mesa-libclc-rpm/SPECS/uke-mesa-libclc22-spirv.spec"
