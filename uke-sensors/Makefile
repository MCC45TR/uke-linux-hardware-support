SHELL := /bin/bash
.PHONY: validate srpm srpm-ssc srpm-ssc-fileservice srpm-sensor-tools test-native test-hexagonrpc test-touch-normal
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false and (.expected_packages | length > 0)' manifests/component.json >/dev/null
srpm:
	@echo 'No validated Uke package payload yet; complete the manifest acceptance gates.' >&2
	@exit 1
srpm-ssc:
	mkdir -p build/ssc-rpm/{SOURCES,SPECS,SRPMS}
	cp referances/libssc/libssc-v0.4.4.tar.gz referances/iio-sensor-proxy/source/iio-sensor-proxy-3.9.tar.bz2 patches/libssc/*.patch patches/iio-sensor-proxy/*.patch packaging/selinux/uke-ssc.cil build/ssc-rpm/SOURCES/
	cp packaging/libssc.spec packaging/iio-sensor-proxy.spec build/ssc-rpm/SPECS/
	for spec in build/ssc-rpm/SPECS/*.spec; do rpmbuild -bs --define "_topdir $(CURDIR)/build/ssc-rpm" "$$spec" || exit; done
srpm-ssc-fileservice:
	mkdir -p build/ssc-files-rpm/{SOURCES,SPECS,SRPMS}
	cp referances/hexagonrpc-write/source.tar.gz build/ssc-files-rpm/SOURCES/hexagonrpc-510c87e5.tar.gz
	cp patches/hexagonrpc/*.patch packaging/systemd/*.service packaging/systemd/iio-ssc-order.conf packaging/uke-ssc.conf tests/hexagon-registry.cpp tests/hexagon-shell.c build/ssc-files-rpm/SOURCES/
	cp packaging/uke-ssc-fileservice.spec build/ssc-files-rpm/SPECS/
	rpmbuild -bs --define "_topdir $(CURDIR)/build/ssc-files-rpm" build/ssc-files-rpm/SPECS/uke-ssc-fileservice.spec
srpm-sensor-tools:
	mkdir -p build/sensor-tools-rpm/{SOURCES,SPECS,SRPMS}
	tar --sort=name --mtime=@1791936000 --owner=0 --group=0 --numeric-owner --transform='s,^,uke-sensor-tools-0.1.0/,' -cf - src/ssc-discovery.cpp src/ssc-sample.cpp tests/ssc-discovery.cpp tests/ssc-sample.cpp | gzip -n > build/sensor-tools-rpm/SOURCES/uke-sensor-tools-0.1.0.tar.gz
	cp packaging/uke-sensor-tools.spec build/sensor-tools-rpm/SPECS/
	rpmbuild -bs --define "_topdir $(CURDIR)/build/sensor-tools-rpm" build/sensor-tools-rpm/SPECS/uke-sensor-tools.spec
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
	sed -n '/^static int create_shell_pd(/,/^}/p' build/hexagonrpc-source/hexagonrpcd/rpcd.c > build/hexagonrpc/shell-function.h
	$(CC) -std=gnu11 -O2 -Wall -Wextra -Werror -I build/hexagonrpc tests/hexagon-shell.c -o build/hexagonrpc/test-shell
	./build/hexagonrpc/test-shell

# Host qualification needs the privately retained normal CSOT image; never an MP image.
test-touch-normal:
	test -n "$(TOUCH_NORMAL_FIRMWARE)"
	mkdir -p build
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-normal-probe.cpp -lcrypto -o build/touch-normal-contract
	./build/touch-normal-contract "$(TOUCH_NORMAL_FIRMWARE)"
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/touch-normal-probe.cpp -lcrypto -o build/uke-touch-normal-probe
