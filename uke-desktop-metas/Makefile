SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
.ONESHELL:
.PHONY: all validate test-native test-applet test-applet-load test-light-client srpm
all: build/qml/org/senemos/uke/controls/libukelightclient.so
build/light-client.moc: src/light-client.cpp
	mkdir -p build
	$$(pkg-config --variable=libexecdir Qt6Core)/moc $< -o $@
build/qml/org/senemos/uke/controls/libukelightclient.so: src/light-client.cpp src/light-client.qmldir build/light-client.moc
	mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Werror -fPIC -shared -Ibuild $(LDFLAGS) src/light-client.cpp $$(pkg-config --cflags --libs Qt6Qml Qt6DBus) -o $@
	cp src/light-client.qmldir $(@D)/qmldir
version := 1.0.0
package := $(notdir $(CURDIR))
outdir ?= $(CURDIR)/build/srpm
# shortcut: timestamps follow the monotonic Release, include Version before resetting Release.
archive_epoch := $(shell awk '/^Release:/ {printf "%.0f", 1791158400 + int($$2); exit}' packaging/uke-desktop-metas.spec)
validate:
	jq -e '.schema_version == 1 and .device == "uke" and .soc == "SM7675" and .hardware_tested == false and .python_target_payload == false' manifests/component.json >/dev/null
	jq -e '.device == "uke" and .hardware_tested == false and .boot_tested == false' src/profile.json >/dev/null
	if grep -Eq '^(Requires|Recommends):.*(python|pypy|libpython|nabu)' packaging/*.spec; then exit 1; fi
	bash tests/plasma-profile.sh
	bash tests/cutefish-profile.sh
test-native: validate
test-applet:
	node tests/applet.cjs
test-applet-load: all
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Werror $(LDFLAGS) tests/applet-load.cpp $$(pkg-config --cflags --libs Qt6Gui Qt6Qml) -o build/applet-load
	runtime=$$(mktemp -d)
	trap 'rm -rf -- "$$runtime"' EXIT
	QML_IMPORT_PATH="$(CURDIR)/build/qml" XDG_RUNTIME_DIR="$$runtime" QT_QPA_PLATFORM=offscreen QT_QPA_PLATFORMTHEME=generic QT_QUICK_BACKEND=software build/applet-load "$(CURDIR)/src/plasma-applet/contents/ui/main.qml"
	QML_IMPORT_PATH="$(CURDIR)/build/qml" bash tests/applet-cache.sh
build/light-client-test.moc: tests/light-client.cpp
	mkdir -p build
	$$(pkg-config --variable=libexecdir Qt6Core)/moc $< -o $@
test-light-client: all build/light-client-test.moc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Werror -UNDEBUG -Ibuild $(LDFLAGS) tests/light-client.cpp $$(pkg-config --cflags --libs Qt6Qml Qt6DBus) -o build/test-light-client
	dbus-run-session -- bash -c 'DBUS_SYSTEM_BUS_ADDRESS="$$DBUS_SESSION_BUS_ADDRESS" build/test-light-client'
srpm: validate
	top="$$(realpath -m "$(outdir)/rpmbuild")"
	mkdir -p "$$top"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS}
	tar --sort=name --mtime=@$(archive_epoch) --owner=0 --group=0 --numeric-owner -cJf "$$top/SOURCES/$(package)-$(version).tar.xz" --transform 's,^,$(package)-$(version)/,' LICENSE README.md Makefile src tests packaging
	cp packaging/$(package).spec "$$top/SPECS/"
	rpmbuild -bs --nodeps --target aarch64 --define '_buildhost uke-builder' --define "_topdir $$top" "$$top/SPECS/$(package).spec"
	mkdir -p "$(outdir)"
	install -m644 "$$top/SRPMS/"*.src.rpm "$(outdir)/"
