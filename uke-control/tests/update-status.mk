# SPDX-License-Identifier: MIT
.PHONY: check-update-status
all: build/update-status build/set-slot
check: check-update-status
check-update-status: build/test-update-status
	build/test-update-status
build/update-status: src/update-status.cpp | build
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@ $(shell pkg-config --cflags --libs jsoncpp fdisk openssl)
build/set-slot: src/set-slot.cpp src/update-status.cpp | build
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@ $(shell pkg-config --cflags --libs jsoncpp fdisk openssl)
build/test-update-status: tests/update-status.cpp src/update-status.cpp | build
	$(CXX) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) $< -o $@ $(shell pkg-config --cflags --libs jsoncpp fdisk openssl)
