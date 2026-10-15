SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate srpm test-native test-hexagonrpc test-touch-normal test-touch-thp touchscreen-rpm
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false and (.expected_packages | length > 0)' manifests/component.json >/dev/null
srpm:
	@echo 'No validated Uke package payload yet; complete the manifest acceptance gates.' >&2
	@exit 1
touchscreen-rpm:
	test "$$(uname -m)" = aarch64
	test -s src/upstream/nt36532-thp/nvt_touch_core.cpp
	top="$$(realpath -m build/rpmbuild)"; mkdir -p "$$top"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS}
	tar --sort=name --mtime=@1791024250 --owner=0 --group=0 --numeric-owner \
	 --transform='s#^#uke-touchscreen-0.1.0/#' -cJf "$$top/SOURCES/uke-touchscreen-0.1.0.tar.xz" \
	 src/touch-* src/upstream/nt36532-thp src/units src/modules-load.d src/modprobe.d src/udev tests/touch-input.cpp tests/touch-normal-probe.cpp LICENSE.GPL-3.0
	cp packaging/uke-touchscreen.spec "$$top/SPECS/"
	rpmbuild -ba --define "_topdir $$(realpath -m build/rpmbuild)" "$$(realpath -m build/rpmbuild)/SPECS/uke-touchscreen.spec"
test-native:
	mkdir -p build
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/ssc-discovery.cpp -o build/ssc-contract
	./build/ssc-contract
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/ssc-discovery.cpp -o build/uke-sensor-discovery
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/ssc-sample.cpp -o build/ssc-sample-contract
	./build/ssc-sample-contract
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/ssc-sample.cpp -o build/uke-sensor-sample
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-readonly.cpp -o build/touch-contract
	./build/touch-contract
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/touch-readonly.cpp -o build/uke-touch-readonly

# Host qualification only; this target neither installs nor starts a tablet daemon.
test-hexagonrpc:
	printf '%s\n' 'ed7b0e704a5e8c20b77b013b4b34f3cddbe41d4cf546edc73f029501f7ebdec5  referances/hexagonrpc-write/source.tar.gz' | sha256sum -c -
	mkdir -p build/hexagonrpc-source build/hexagonrpc
	tar -xf referances/hexagonrpc-write/source.tar.gz --strip-components=1 -C build/hexagonrpc-source
	for patch in patches/hexagonrpc/*.patch; do patch --batch -p1 -d build/hexagonrpc-source < "$$patch" || exit; done
	cd build/hexagonrpc-source && $(CC) -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -DHEXAGONRPC_VERBOSE -I include libhexagonrpc/*.c libhexagonrpc/interface/*.c hexagonrpcd/*.c hexagonrpcd/interface/*.c -o ../hexagonrpc/hexagonrpcd
	cd build/hexagonrpc-source && $(CC) -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -I include tests/test_hexagonrpc.c libhexagonrpc/hexagonrpc.c -o ../hexagonrpc/test-rpc
	./build/hexagonrpc/test-rpc
	cd build/hexagonrpc-source && $(CC) -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -I include tests/test_iobuffer.c hexagonrpcd/iobuffer.c -o ../hexagonrpc/test-buffer
	./build/hexagonrpc/test-buffer
	cd build/hexagonrpc-source && $(CC) -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -I include tests/test_hexagonfs.c hexagonrpcd/hexagonfs.c hexagonrpcd/hexagonfs_mapped.c -o ../hexagonrpc/test-files
	./build/hexagonrpc/test-files build/hexagonrpc-source/tests/sample_file.txt
	cd build/hexagonrpc-source && for name in hexagonfs hexagonfs_mapped hexagonfs_virt_dir rpcd_builder; do $(CC) -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -I include -c "hexagonrpcd/$$name.c" -o "../hexagonrpc/$$name.o" || exit; done
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror -I build/hexagonrpc-source/hexagonrpcd tests/hexagon-registry.cpp build/hexagonrpc/{hexagonfs,hexagonfs_mapped,hexagonfs_virt_dir,rpcd_builder}.o -o build/hexagonrpc/test-registry
	./build/hexagonrpc/test-registry

# Host qualification needs the privately retained normal CSOT image; never an MP image.
test-touch-normal:
	test -n "$(TOUCH_NORMAL_FIRMWARE)"
	mkdir -p build
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-normal-probe.cpp -lcrypto -o build/touch-normal-contract
	./build/touch-normal-contract "$(TOUCH_NORMAL_FIRMWARE)"
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/touch-normal-probe.cpp -lcrypto -o build/uke-touch-normal-probe

# Laboratory only: private OEM calibration/reference packets are not RPM payloads.
test-touch-thp:
	test -n "$(TOUCH_CORE_REFERENCE)" -a -n "$(TOUCH_FRAME_REFERENCE)" -a -n "$(TOUCH_CONFIG)"
	bash src/prepare-touch-core.sh "$(TOUCH_CORE_REFERENCE)"
	mkdir -p build
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-thp.cpp src/upstream/nt36532-thp/nvt_touch_core.cpp -lcrypto -o build/touch-thp-contract
	./build/touch-thp-contract "$(TOUCH_FRAME_REFERENCE)" "$(TOUCH_CONFIG)"
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-input.cpp src/upstream/nt36532-thp/nvt_touch_core.cpp -lcrypto -o build/touch-input-contract
	./build/touch-input-contract
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-replay.cpp src/upstream/nt36532-thp/nvt_touch_core.cpp -lcrypto -o build/touch-replay
	bash tests/touch-replay.sh ./build/touch-replay "$(TOUCH_CONFIG)" "$(TOUCH_FRAME_REFERENCE)"
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/touch-thp.cpp src/upstream/nt36532-thp/nvt_touch_core.cpp -lcrypto -o build/uke-touch-thp
