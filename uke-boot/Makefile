SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate test-native test-startup test-kernel-update test-usb srpm
version := 1.1.4
outdir ?= $(CURDIR)/build/srpm
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false' manifests/component.json >/dev/null
	jq -e '.device=="uke" and .partitions.esp=="esp" and .partitions.linux=="linux" and .filesystem_labels.esp=="UKE_ESP" and .filesystem_labels.linux=="UKE_LINUX" and .partition_offsets==null and .automatic_activation==false and .android_partition_writes==false and .uefi_binary_included==false and .handoff_accepted==false' src/profiles/image-contract.json >/dev/null
	bash tests/update-packaging.sh
	! grep -Eq '^Requires:.*(python|pypy|nabu)' packaging/uke-boot-integration.spec
	bash -n src/audit-kernel-inputs.sh
	sh -n src/uke-usb-network.sh
	bash -n tests/startup-native.sh src/dracut/93uke-boot-guard/module-setup.sh
test-native: validate
	bash tests/boot-status.sh
	bash tests/kernel-inputs.sh
	c++ -std=c++17 -Wall -Wextra -Werror -O2 tests/wifi-factory.cpp -o build/tests/wifi-factory
	build/tests/wifi-factory
	c++ -std=c++17 -Wall -Wextra -Werror -O2 src/esp32-cdc.cpp -o build/tests/uke-esp32-cdc
	c++ -std=c++17 -Wall -Wextra -Werror -O2 tests/esp32-tty.cpp -o build/tests/esp32-tty -lutil
	build/tests/esp32-tty
test-startup:
	bash tests/startup-contract.sh
	bash tests/startup-native.sh
test-kernel-update:
	mkdir -p build/tests
	c++ -std=c++20 -O2 -Wall -Wextra -Werror tests/kernel-update.cpp -o build/tests/kernel-update-contract $$(pkg-config --cflags --libs openssl libsystemd)
	build/tests/kernel-update-contract
	bash tests/installation-policy.sh
	bash tests/slot-control.sh
	bash tests/userdata-update.sh
test-usb:
	mkdir -p build/tests
	c++ -std=c++17 -O2 -Wall -Wextra -Werror tests/usb-port.cpp -o build/tests/usb-port-contract
	build/tests/usb-port-contract
	bash tests/usb-inventory.sh
	bash tests/usb-auto.sh
	bash tests/usb-ownership.sh
srpm: validate
	top="$$(realpath -m "$(outdir)/rpmbuild")"
	mkdir -p "$$top"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS}
	tar --sort=name --mtime=@1791936000 --owner=0 --group=0 --numeric-owner -cJf "$$top/SOURCES/uke-boot-integration-$(version).tar.xz" --transform 's,^,uke-boot-integration-$(version)/,' LICENSE README.md src tests docs/KERNEL-UPDATES.md docs/USERDATA-AB.md docs/USB-TYPEC.md packaging/uke-boot-integration.spec
	cp packaging/uke-boot-integration.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define "_topdir $$top" "$$top/SPECS/uke-boot-integration.spec"
	mkdir -p "$(outdir)"
	install -m644 "$$top/SRPMS/"*.src.rpm "$(outdir)/"
