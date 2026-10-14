SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: validate srpm test-native test-cache
PACKAGE ?= libssc
outdir ?= $(CURDIR)/build/srpm/$(PACKAGE)
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false and (.expected_packages | length > 0)' manifests/component.json >/dev/null
	for spec in packaging/{libssc,iio-sensor-proxy,uke-ssc-fileservice}/*.spec; do
		test -f "$$spec"
		if grep -E '^(Version|Release):.*[Uu][Kk][Ee]|^Requires:[[:space:]]*(python|pypy|libpython)' "$$spec"; then exit 1; fi
	done
srpm: validate
	case '$(PACKAGE)' in libssc|iio-sensor-proxy|uke-ssc-fileservice) ;; *) echo 'Unreviewed sensor package' >&2; exit 1;; esac
	case '$(PACKAGE)' in
	libssc) archive=referances/libssc/libssc-v0.4.4.tar.gz; url=https://codeberg.org/DylanVanAssche/libssc/archive/v0.4.4.tar.gz; hash=716d6bd6b34d2d753060c6b54c9a87e34fae75b724c763bf9ef487efa3621587; name=libssc-v0.4.4.tar.gz;;
	iio-sensor-proxy) archive=referances/iio-sensor-proxy/source/iio-sensor-proxy-3.9.tar.bz2; url=https://gitlab.freedesktop.org/hadess/iio-sensor-proxy/-/archive/3.9/iio-sensor-proxy-3.9.tar.bz2; hash=19a4ae99b1b8af603fc789841025f9747bf445418d69ec7389e75adb6c8186bf; name=iio-sensor-proxy-3.9.tar.bz2;;
	uke-ssc-fileservice) archive=referances/hexagonrpc-write/source.tar.gz; url=https://api.github.com/repos/linux-msm/hexagonrpc/tarball/510c87e5c89026ee2bd2034776fc4d15994f353d; hash=ed7b0e704a5e8c20b77b013b4b34f3cddbe41d4cf546edc73f029501f7ebdec5; name=hexagonrpc-510c87e5.tar.gz;;
	esac
	mkdir -p "$$(dirname "$$archive")"
	if test ! -s "$$archive"; then curl -fL --retry 3 --connect-timeout 20 "$$url" -o "$$archive.part"; mv "$$archive.part" "$$archive"; fi
	test "$$(sha256sum "$$archive" | cut -d ' ' -f1)" = "$$hash"
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	cp "$$archive" "$$top/SOURCES/$$name"
	case '$(PACKAGE)' in
	libssc) cp patches/libssc/*.patch "$$top/SOURCES/";;
	iio-sensor-proxy) cp patches/iio-sensor-proxy/*.patch packaging/selinux/uke-ssc.cil "$$top/SOURCES/";;
	uke-ssc-fileservice) cp patches/hexagonrpc/*.patch packaging/systemd/*.service packaging/systemd/iio-ssc-order.conf packaging/uke-ssc.conf tests/hexagon-registry.cpp tests/hexagon-shell.c "$$top/SOURCES/";;
	esac
	cp packaging/$(PACKAGE)/$(PACKAGE).spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define "_topdir $$top" "$$top/SPECS/$(PACKAGE).spec"
	install -m644 "$$top/SRPMS/"*.src.rpm '$(outdir)/'
test-native:
	bash tests/source-contract.sh
	mkdir -p build
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/ssc-discovery.cpp -o build/ssc-contract
	./build/ssc-contract
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/ssc-discovery.cpp -o build/uke-sensor-discovery
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror tests/touch-readonly.cpp -o build/touch-contract
	./build/touch-contract
	$(CXX) -std=c++20 -O2 -Wall -Wextra -Werror src/touch-readonly.cpp -o build/uke-touch-readonly
test-cache:
	test "$$(id -u)" = 0
	bash tests/ssc-cache.sh
