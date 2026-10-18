SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
CXX ?= g++
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++20 -Wall -Wextra -Werror -fPIC
CPPFLAGS += -Isrc -Ibuild $(shell pkg-config --cflags libdrm libsystemd wayland-client libselinux)
LDLIBS = $(shell pkg-config --libs libdrm libsystemd wayland-client)
outdir ?= $(CURDIR)/build/srpm
protocols := $(shell pkg-config --variable=pkgdatadir wayland-protocols)
.PHONY: all check clean srpm validate
.PHONY: test-audio-inventory
all: build/uke-control build/uke-control-tests build/ssc-discovery build/ssc-sample
test-audio-inventory:
	bash tests/audio-inventory.sh
build:
	mkdir -p build
build/libuke-control.so.0: src/backend.cpp src/backend.hpp src/charge-policy.cpp src/charge-policy.hpp src/usb.cpp src/usb.hpp src/usb-request.cpp src/usb-request.hpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -shared -Wl,-soname,libuke-control.so.0 -o $@ src/backend.cpp src/charge-policy.cpp src/usb.cpp src/usb-request.cpp $(shell pkg-config --libs libselinux)
build/uke-control: src/main.cpp build/libuke-control.so.0
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ src/main.cpp -Lbuild -l:libuke-control.so.0
build/xdg-shell-client.h: | build
	wayland-scanner client-header $(protocols)/stable/xdg-shell/xdg-shell.xml $@
build/presentation-time-client.h: | build
	wayland-scanner client-header $(protocols)/stable/presentation-time/presentation-time.xml $@
build/%-protocol.c: | build
	case '$*' in xdg-shell) xml=$(protocols)/stable/xdg-shell/xdg-shell.xml;; presentation-time) xml=$(protocols)/stable/presentation-time/presentation-time.xml;; esac
	wayland-scanner private-code "$$xml" $@
build/%-protocol.o: build/%-protocol.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@ $(shell pkg-config --cflags wayland-client)
build/uke-control-tests: src/diagnostics.cpp src/cadence.cpp src/contact.cpp build/xdg-shell-client.h build/presentation-time-client.h build/xdg-shell-protocol.o build/presentation-time-protocol.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ src/diagnostics.cpp src/cadence.cpp src/contact.cpp build/*-protocol.o $(LDLIBS)
build/ssc-discovery: shared/ssc-discovery.cpp | build
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<
build/ssc-sample: shared/ssc-sample.cpp shared/ssc-discovery.cpp | build
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<
check: all
	bash tests/audio-inventory.sh
	LD_LIBRARY_PATH=build build/uke-control --self-test
	build/uke-control-tests --self-test
	bash tests/charge-policy.sh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/usb.cpp -Lbuild -l:libuke-control.so.0 -o build/test-usb
	LD_LIBRARY_PATH=build build/test-usb
	for name in discovery sample; do $(CXX) $(CXXFLAGS) $(LDFLAGS) tests/ssc-$$name.cpp -o build/test-ssc-$$name; build/test-ssc-$$name; done
	LD_LIBRARY_PATH=build build/uke-control --help
validate:
	sh -n src/audio-inventory.sh
	test -s src/backend.cpp -a -s src/diagnostics.cpp -a -s packaging/uke-control.spec
	cmp shared/ssc-discovery.cpp ../uke-sensors/src/ssc-discovery.cpp
	cmp shared/ssc-sample.cpp ../uke-sensors/src/ssc-sample.cpp
srpm: validate
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	tar --sort=name --mtime=@1792270800 --owner=0 --group=0 --numeric-owner --exclude=build --transform='s#^#uke-control-0.1.0/#' -czf "$$top/SOURCES/uke-control-0.1.0.tar.gz" src shared tests packaging Makefile LICENSE README.md CHARGING.md
	cp packaging/uke-control.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define "_topdir $$top" "$$top/SPECS/uke-control.spec"
	install -m644 "$$top/SRPMS/"*.src.rpm '$(outdir)/'
clean:
	rm -rf build
