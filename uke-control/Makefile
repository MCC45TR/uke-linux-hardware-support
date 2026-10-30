SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
CXX ?= g++
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++20 -Wall -Wextra -Werror -fPIC
CPPFLAGS += -Isrc -Ibuild $(shell pkg-config --cflags libdrm libsystemd wayland-client libselinux)
LDLIBS = $(shell pkg-config --libs libdrm libsystemd wayland-client)
outdir ?= $(CURDIR)/build/srpm
version := $(shell awk -F '"' '/application_version=/ {print $$2}' src/version.hpp)
protocols := $(shell pkg-config --variable=pkgdatadir wayland-protocols)
.PHONY: all check check-native check-bus check-rgb-policy check-fedora-target check-repositories check-sensor-proxy check-copr-trigger clean srpm validate
all: build/uke-control build/uke-control-packages build/uke-control-tests build/ssc-discovery build/ssc-sample build/resize-system build/layout-manager build/uke-control-rgb build/torch-access build/selinux/uke_control_rgb.pp
all: build/uke-control-tui build/uke-control-client.pc
client_sources := src/client.cpp src/client-catalog.cpp src/client-process.cpp
client_flags := $(shell pkg-config --cflags jsoncpp)
client_libs := $(shell pkg-config --libs jsoncpp) -pthread
build/libuke-control-client.so.1: $(client_sources) src/client.hpp src/sysfs.hpp src/version.hpp | build
	$(CXX) $(CXXFLAGS) $(client_flags) $(LDFLAGS) -shared -Wl,-soname,libuke-control-client.so.1 -o $@ $(client_sources) $(client_libs)
build/uke-control-tui: src/tui.cpp src/tui-text.hpp src/tui-menu.hpp src/client.hpp src/version.hpp build/libuke-control-client.so.1
	$(CXX) $(CXXFLAGS) $(client_flags) $(LDFLAGS) -o $@ src/tui.cpp -Lbuild -l:libuke-control-client.so.1 $(shell pkg-config --cflags --libs ncursesw jsoncpp)
libdir ?= /usr/lib64
includedir ?= /usr/include
build/uke-control-client.pc: packaging/uke-control-client.pc.in src/version.hpp | build
	sed -e 's|@VERSION@|$(version)|g' -e 's|@LIBDIR@|$(libdir)|g' -e 's|@INCLUDEDIR@|$(includedir)|g' $< > $@
build/test-client-child: tests/client-child.cpp | build
	$(CXX) $(CXXFLAGS) $(client_flags) $(LDFLAGS) -o $@ $< $(client_libs)
build/test-client: tests/client.cpp $(client_sources) src/client.hpp src/tui-text.hpp src/sysfs.hpp src/version.hpp build/test-client-child
	$(CXX) -Isrc $(CXXFLAGS) -UNDEBUG $(client_flags) $(LDFLAGS) -DUKE_CLIENT_PROGRAM='"$(CURDIR)/build/test-client-child"' -o $@ tests/client.cpp $(client_sources) $(client_libs)
.PHONY: check-client check-tui
check-client: build/test-client build/uke-control
	ulimit -c 0
	timeout 45s build/test-client
	LD_LIBRARY_PATH=build:$${LD_LIBRARY_PATH:-} bash tests/client-cli.sh
build/test-tui: tests/tui.cpp src/tui-menu.hpp src/client.hpp build/libuke-control-client.so.1 | build
	$(CXX) -Isrc $(CXXFLAGS) -UNDEBUG $(LDFLAGS) -o $@ $< -Lbuild -l:libuke-control-client.so.1 -lutil $(shell pkg-config --cflags --libs vterm)
check-tui: build/uke-control-tui build/test-tui
	ulimit -c 0
	LD_LIBRARY_PATH=build:$${LD_LIBRARY_PATH:-} timeout 90s build/test-tui
build/libuke-control.so.0: src/battery.cpp src/battery.hpp src/nfc.cpp src/nfc.hpp
build/test-battery: tests/battery.cpp src/battery.cpp src/battery.hpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) tests/battery.cpp src/battery.cpp $(shell pkg-config --libs libsystemd) -o $@
build/test-nfc: tests/nfc.cpp src/nfc.cpp src/nfc.hpp | build
	$(CXX) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) tests/nfc.cpp src/nfc.cpp -o $@
check-native: build/test-battery build/test-nfc
build:
	mkdir -p build
build/uke-control-rgb: src/rgb-service.cpp src/rgb.hpp shared/uke-rgb.h build/libuke-control.so.0
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ src/rgb-service.cpp -Lbuild -l:libuke-control.so.0 $(shell pkg-config --libs libsystemd libselinux)
build/torch-access: src/torch-access.cpp build/libuke-control.so.0
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $< -Lbuild -l:libuke-control.so.0
build/selinux/uke_control_rgb.pp: packaging/selinux/uke_control_rgb.te packaging/selinux/uke_control_rgb.fc | build
	mkdir -p build/selinux
	cp packaging/selinux/uke_control_rgb.{te,fc} build/selinux/
	$(MAKE) -C build/selinux -f /usr/share/selinux/devel/Makefile uke_control_rgb.pp
build/libuke-control.so.0: src/cli.cpp src/cli.hpp src/backend.cpp src/backend.hpp src/lights.cpp src/lights.hpp src/rgb.cpp src/rgb.hpp src/torch.cpp src/torch.hpp src/charge-policy.cpp src/charge-policy.hpp src/charge-client.cpp src/charge-client.hpp src/usb.cpp src/usb.hpp src/sysfs.hpp src/firmware.cpp src/firmware.hpp src/link-policy.cpp src/link-policy.hpp src/usb-request.cpp src/usb-request.hpp src/front-color.cpp src/front-color.hpp src/rpm-api.hpp src/desktop.cpp src/desktop.hpp src/plasma.cpp src/plasma.hpp src/hardware.cpp src/hardware.hpp src/wake.cpp src/wake.hpp src/vrr.cpp src/vrr.hpp src/display-device.hpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -shared -Wl,-soname,libuke-control.so.0 -o $@ src/cli.cpp src/backend.cpp src/battery.cpp src/nfc.cpp src/lights.cpp src/rgb.cpp src/torch.cpp src/charge-policy.cpp src/charge-client.cpp src/usb.cpp src/firmware.cpp src/usb-request.cpp src/link-policy.cpp src/front-color.cpp src/desktop.cpp src/plasma.cpp src/hardware.cpp src/wake.cpp src/vrr.cpp $(shell pkg-config --libs libselinux libsystemd libdrm)
build/uke-control: src/main.cpp src/overview.cpp src/overview.hpp src/sensors.cpp src/sensors.hpp src/charge-notify.cpp src/charge-notify.hpp src/diagnostic.cpp src/diagnostic.hpp src/sysfs.hpp src/vrr.hpp src/usb-request.hpp src/client.hpp build/libuke-control.so.0 build/libuke-control-client.so.1
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ src/main.cpp src/overview.cpp src/sensors.cpp src/charge-notify.cpp src/diagnostic.cpp -Lbuild -l:libuke-control.so.0 -l:libuke-control-client.so.1 $(shell pkg-config --cflags --libs libsystemd jsoncpp)
build/uke-control-packages: src/packages.cpp src/cli.cpp src/cli.hpp src/fedora-target.hpp | build
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< src/cli.cpp -Isrc $(shell pkg-config --cflags --libs libdnf5 rpm libsystemd)
build/uke-control-packages build/test-package-repos: src/package-operation.hpp src/maintenance.hpp
build/libuke-control.so.0: src/report-budget.hpp src/report-context.hpp
build/libuke-control.so.0: src/usb-transition.hpp
build/libuke-control.so.0: src/charge-intent.hpp
build/libuke-control.so.0: src/version.hpp
build/libuke-control.so.0 build/uke-control build/uke-control-packages build/uke-control-tests build/layout-manager: src/version.hpp src/error.hpp
build/layout-manager build/test-layout build/test-layout-safety: src/maintenance.hpp
build/uke-control-tests: src/contact.hpp
build/resize-system: src/resize-system.cpp shared/userdata-grow.hpp src/maintenance.hpp | build
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ src/resize-system.cpp
build/xdg-shell-client.h: | build
	wayland-scanner client-header $(protocols)/stable/xdg-shell/xdg-shell.xml $@
build/presentation-time-client.h: | build
	wayland-scanner client-header $(protocols)/stable/presentation-time/presentation-time.xml $@
build/%-protocol.c: | build
	case '$*' in xdg-shell) xml=$(protocols)/stable/xdg-shell/xdg-shell.xml;; presentation-time) xml=$(protocols)/stable/presentation-time/presentation-time.xml;; esac
	wayland-scanner private-code "$$xml" $@
build/%-protocol.o: build/%-protocol.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@ $(shell pkg-config --cflags wayland-client)
build/uke-control-tests: src/cli.cpp src/cli.hpp src/diagnostics.cpp src/display-device.hpp src/cadence.cpp src/contact.cpp src/desktop.cpp src/desktop.hpp src/hardware.cpp src/hardware.hpp build/xdg-shell-client.h build/presentation-time-client.h build/xdg-shell-protocol.o build/presentation-time-protocol.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ src/cli.cpp src/diagnostics.cpp src/cadence.cpp src/contact.cpp src/desktop.cpp src/hardware.cpp build/*-protocol.o $(LDLIBS)
build/ssc-discovery: shared/ssc-discovery.cpp | build
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<
build/ssc-sample: shared/ssc-sample.cpp shared/ssc-discovery.cpp | build
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<
build/test-front-color: tests/front-color.cpp src/front-color.cpp src/front-color.hpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/front-color.cpp $(shell pkg-config --libs libsystemd) -o $@
build/test-execute: tests/execute.cpp src/backend.hpp build/libuke-control.so.0
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) $< -Lbuild -l:libuke-control.so.0 -o $@
check-fedora-target: | build
	$(CXX) $(CXXFLAGS) -UNDEBUG tests/fedora-target.cpp -o build/test-fedora-target
	build/test-fedora-target
check-rgb-policy: build/selinux/uke_control_rgb.pp
	bash tests/rgb-policy.sh
build/test-cli: tests/cli.cpp src/cli.cpp src/cli.hpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) tests/cli.cpp src/cli.cpp $(shell pkg-config --libs libsystemd) -o $@
build/test-contact: tests/contact.cpp src/contact.hpp | build
	$(CXX) -Isrc $(CXXFLAGS) -UNDEBUG $(LDFLAGS) $< -o $@
build/test-maintenance: tests/maintenance.cpp src/maintenance.hpp | build
	$(CXX) -Isrc $(CXXFLAGS) -UNDEBUG $(LDFLAGS) $< -o $@
build/test-report-budget: tests/report-budget.cpp src/report-budget.hpp src/report-context.hpp | build
	$(CXX) -Isrc $(CXXFLAGS) -UNDEBUG $(LDFLAGS) $< $(shell pkg-config --cflags --libs libsystemd) -o $@
build/test-package-operation: tests/package-operation.cpp src/package-operation.hpp | build
	$(CXX) -Isrc $(CXXFLAGS) -UNDEBUG $(LDFLAGS) $< -o $@
build/test-usb-transition: tests/usb-transition.cpp src/usb-transition.hpp | build
	$(CXX) -Isrc $(CXXFLAGS) -UNDEBUG $(LDFLAGS) tests/usb-transition.cpp -o $@
build/test-charge-intent: tests/charge-intent.cpp src/charge-intent.hpp | build
	$(CXX) -Isrc $(CXXFLAGS) -UNDEBUG $(LDFLAGS) $< -o $@
build/test-roots: tests/roots.cpp src/overview.cpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG -ffunction-sections -fdata-sections $(LDFLAGS) tests/roots.cpp $(shell pkg-config --cflags --libs jsoncpp) -Wl,--gc-sections -o $@
build/test-cadence-handshake: tests/cadence-handshake.cpp src/cadence.cpp build/xdg-shell-client.h build/presentation-time-client.h build/xdg-shell-protocol.o build/presentation-time-protocol.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) tests/cadence-handshake.cpp build/*-protocol.o $(shell pkg-config --libs wayland-client) -o $@
check: check-native check-bus check-client check-tui
check-native check-bus check-repositories check-sensor-proxy: CXXFLAGS += -UNDEBUG
check-native: build/test-contact build/test-maintenance build/test-report-budget build/test-package-operation build/test-usb-transition build/test-charge-intent build/test-roots build/test-cadence-handshake
check-native: all check-fedora-target check-rgb-policy check-layout build/test-front-color build/test-execute build/test-cli
	build/test-battery
	build/test-nfc
	build/test-contact
	build/test-maintenance
	build/test-report-budget
	build/test-package-operation
	if test "$$(id -u)" = 0; then build/test-usb-transition; fi
	build/test-charge-intent
	build/test-roots
	build/test-cadence-handshake
	bash tests/cli.sh
	bash tests/overview.sh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) tests/rgb.cpp -Lbuild -l:libuke-control.so.0 -o build/test-rgb
	LD_LIBRARY_PATH=build build/test-rgb
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) tests/lights.cpp -Lbuild -l:libuke-control.so.0 -o build/test-lights
	LD_LIBRARY_PATH=build build/test-lights
	bash tests/light-worker.sh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) tests/plasma.cpp src/plasma.cpp $(shell pkg-config --libs libsystemd) -o build/test-plasma
	build/test-plasma
	LD_LIBRARY_PATH=build build/test-execute
	$(CXX) $(CXXFLAGS) $(LDFLAGS) tests/resize-system.cpp -o build/test-resize-system
	build/test-resize-system
	$(CXX) $(CXXFLAGS) $(LDFLAGS) tests/diagnostic.cpp src/cli.cpp $(shell pkg-config --libs libsystemd) -o build/test-diagnostic
	build/test-diagnostic
	$(CXX) $(CXXFLAGS) $(LDFLAGS) tests/wake.cpp src/wake.cpp -o build/test-wake
	build/test-wake
	bash tests/wake-policy.sh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/vrr.cpp src/wake.cpp $(shell pkg-config --libs libdrm) -o build/test-vrr
	build/test-vrr
	bash tests/vrr-load.sh
	bash tests/cdc-policy.sh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/display-device.cpp -o build/test-display-device
	build/test-display-device
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/rpm-api.cpp -o build/test-rpm-api
	build/test-rpm-api
	build/test-front-color
	LD_LIBRARY_PATH=build build/uke-control --self-test
	build/uke-control-tests --self-test
	bash tests/charge-policy.sh
	bash tests/swap-discovery.sh
	bash tests/swap-allocation.sh
	bash tests/report.sh
	bash tests/startup.sh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/charge-client.cpp src/charge-client.cpp $(shell pkg-config --libs libsystemd) -o build/test-charge-client
	build/test-charge-client
	bash tests/charge-review.sh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/charge-notify.cpp src/charge-notify.cpp $(shell pkg-config --libs libsystemd) -o build/test-charge-notify
	build/test-charge-notify
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/usb.cpp -Lbuild -l:libuke-control.so.0 -o build/test-usb
	LD_LIBRARY_PATH=build build/test-usb
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) tests/link-policy.cpp -Lbuild -l:libuke-control.so.0 -o build/test-link-policy
	LD_LIBRARY_PATH=build build/test-link-policy
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/firmware.cpp -Lbuild -l:libuke-control.so.0 -o build/test-firmware
	LD_LIBRARY_PATH=build build/test-firmware
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/hardware.cpp src/hardware.cpp -o build/test-hardware
	build/test-hardware
	for name in discovery sample; do $(CXX) $(CXXFLAGS) $(LDFLAGS) tests/ssc-$$name.cpp -o build/test-ssc-$$name; build/test-ssc-$$name; done
	LD_LIBRARY_PATH=build build/uke-control --help
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) tests/sensors.cpp src/sensors.cpp src/front-color.cpp $(shell pkg-config --libs libsystemd) -o build/test-sensors
	build/test-sensors
	$(CXX) $(CXXFLAGS) $(LDFLAGS) tests/packages.cpp src/cli.cpp -Isrc $(shell pkg-config --cflags --libs libdnf5 rpm libsystemd) -o build/test-packages
	build/test-packages
check-bus: build/libuke-control.so.0
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) -pthread tests/rgb-bus.cpp -Lbuild -l:libuke-control.so.0 $(shell pkg-config --libs libsystemd libselinux) -o build/test-rgb-bus
	if test "$$(id -u)" = 0; then
		bus_fixture=$$(mktemp -d)
		trap 'rm -rf -- "$$bus_fixture"' EXIT
		chmod 755 "$$bus_fixture"
		install -m755 build/test-rgb-bus build/libuke-control.so.0 "$$bus_fixture/"
		LD_LIBRARY_PATH="$$bus_fixture" timeout 20s setpriv --reuid=65534 --regid=65534 --clear-groups dbus-run-session -- "$$bus_fixture/test-rgb-bus"
	else LD_LIBRARY_PATH=build timeout 20s dbus-run-session -- build/test-rgb-bus; fi
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) -pthread tests/charge-notify-bus.cpp $(shell pkg-config --libs libsystemd) -o build/test-charge-notify-bus
	dbus-run-session -- build/test-charge-notify-bus
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) -pthread tests/desktop-bus.cpp src/desktop.cpp $(shell pkg-config --libs libsystemd) -o build/test-desktop-bus
	dbus-run-session -- build/test-desktop-bus
check-repositories: build/test-package-repos
	bash tests/package-repos.sh
build/test-package-repos: tests/package-repos.cpp src/packages.cpp src/cli.cpp src/cli.hpp src/fedora-target.hpp | build
	$(CXX) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) tests/package-repos.cpp src/cli.cpp $(shell pkg-config --cflags --libs libdnf5 rpm libsystemd) -o $@
check-sensor-proxy: build/test-sensor-proxy
	bash tests/sensor-proxy.sh
check-copr-trigger:
	bash tests/copr-trigger.sh
build/test-sensor-proxy: tests/sensor-proxy.cpp src/sensors.cpp src/front-color.cpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $< src/front-color.cpp $(shell pkg-config --libs libsystemd) -o $@
validate:
	test -s src/backend.cpp -a -s src/diagnostics.cpp -a -s packaging/uke-control.spec
	test '$(version)' = "$$(awk '/^Version:/ {print $$2}' packaging/uke-control.spec)"
	cmp shared/ssc-discovery.cpp ../uke-sensors/src/ssc-discovery.cpp
	cmp shared/ssc-sample.cpp ../uke-sensors/src/ssc-sample.cpp
srpm: validate
	top="$$(realpath -m '$(outdir)/rpmbuild')"
	mkdir -p "$$top"/{SOURCES,SPECS,SRPMS}
	tar --sort=name --mtime=@1792270800 --owner=0 --group=0 --numeric-owner --exclude=build --transform='s#^#uke-control-$(version)/#' -czf "$$top/SOURCES/uke-control-$(version).tar.gz" src shared tests packaging host Makefile LICENSE README.md CHARGING.md DISPLAY.md CHANNELS.md SENSORS.md DESKTOP.md HARDWARE.md DIAGNOSTIC.md RESIZE.md LIGHTS.md CLI.md BOOT-LAYOUT.md OVERVIEW.md REVIEW-2026-10-30.md TUI.md
	cp packaging/uke-control.spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define '_buildhost uke-builder' --define "_topdir $$top" "$$top/SPECS/uke-control.spec"
	install -m644 "$$top/SRPMS/"*.src.rpm '$(outdir)/'
clean:
	rm -rf build

layout_recovery := $(addprefix shared/recovery/,core.cpp storage.cpp block.cpp gpt.cpp multiboot.cpp layout.cpp)
layout_sources := src/layout-policy.cpp src/layout-runtime.cpp src/layout-execute.cpp src/layout-shutdown.cpp
layout_flags := -ffunction-sections -fdata-sections $(shell pkg-config --cflags jsoncpp blkid openssl)
layout_libs := -Wl,--gc-sections $(shell pkg-config --libs jsoncpp blkid openssl libsystemd)
build/layout-manager: src/layout-ui.cpp src/layout.hpp $(layout_sources) $(layout_recovery) src/cli.cpp | build
	$(CXX) $(CXXFLAGS) $(layout_flags) $(LDFLAGS) -Isrc -o $@ src/layout-ui.cpp $(layout_sources) $(layout_recovery) src/cli.cpp $(layout_libs)
	! nm -C $@ | grep -E 'ure::(gpt_execute|gpt_rollback|multiboot_image_execute|storage_write_gate)\('
build/test-layout: tests/layout.cpp tests/layout-fixture.hpp src/layout.hpp $(layout_sources) $(layout_recovery) | build
	$(CXX) $(CXXFLAGS) -UNDEBUG $(layout_flags) $(LDFLAGS) -Isrc -o $@ tests/layout.cpp $(layout_sources) $(layout_recovery) $(layout_libs)
check-layout: build/layout-manager build/test-layout build/test-layout-safety
	build/test-layout
	build/test-layout-safety
	jq -r '.files | to_entries[] | "\(.value)  shared/recovery/\(.key)"' shared/recovery/source.json | sha256sum -c --status
build/test-layout-vm: tests/layout-vm.cpp tests/layout-vm-common.hpp tests/layout-fixture.hpp src/layout.hpp $(layout_sources) $(layout_recovery) | build
	$(CXX) $(CXXFLAGS) -UNDEBUG $(layout_flags) $(LDFLAGS) -Isrc -o $@ tests/layout-vm.cpp src/layout-policy.cpp src/layout-runtime.cpp $(layout_recovery) $(layout_libs)
build/test-layout-payload: tests/layout-payload.cpp src/layout.hpp $(layout_sources) $(layout_recovery) | build
	$(CXX) $(CXXFLAGS) -UNDEBUG $(layout_flags) $(LDFLAGS) -Isrc -o $@ tests/layout-payload.cpp $(layout_sources) $(layout_recovery) $(layout_libs)
build/test-layout-safety: tests/layout-safety.cpp tests/layout-fixture.hpp src/layout.hpp $(layout_sources) $(layout_recovery) | build
	$(CXX) $(CXXFLAGS) -UNDEBUG $(layout_flags) $(LDFLAGS) -Isrc -o $@ tests/layout-safety.cpp src/layout-policy.cpp src/layout-runtime.cpp src/layout-shutdown.cpp $(layout_recovery) $(layout_libs)
build/test-layout-shutdown-vm: tests/layout-shutdown-vm.cpp tests/layout-vm-common.hpp tests/layout-fixture.hpp src/layout.hpp $(layout_sources) $(layout_recovery) | build
	$(CXX) $(CXXFLAGS) -UNDEBUG $(layout_flags) $(LDFLAGS) -Isrc -o $@ tests/layout-shutdown-vm.cpp src/layout-policy.cpp src/layout-runtime.cpp $(layout_recovery) $(layout_libs)
