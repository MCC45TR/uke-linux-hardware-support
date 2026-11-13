# SPDX-License-Identifier: MIT
SHELL := /bin/bash
.SHELLFLAGS := -Eeuo pipefail -c
CXX ?= g++
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++20 -Wall -Wextra -Werror
motion_out ?= build/motion
motion_flags := $(shell pkg-config --cflags libsystemd jsoncpp)
motion_libs := $(shell pkg-config --libs libsystemd jsoncpp)
.PHONY: all check
all: $(motion_out)/motion $(motion_out)/selinux/uke_control_motion.pp
$(motion_out)/selinux/uke_control_motion.pp: packaging/selinux/uke_control_motion.te packaging/selinux/uke_control_motion.fc | $(motion_out)
	mkdir -p $(motion_out)/selinux
	cp packaging/selinux/uke_control_motion.{te,fc} $(motion_out)/selinux/
	$(MAKE) -C $(motion_out)/selinux -f /usr/share/selinux/devel/Makefile uke_control_motion.pp
$(motion_out):
	mkdir -p $@
$(motion_out)/motion: src/motion-service.cpp src/motion-broker.cpp src/motion-iio.cpp src/motion-broker.hpp src/motion-source.hpp src/motion-sample.hpp src/motion-ownership.hpp src/motion-journal.hpp | $(motion_out)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(motion_flags) $(LDFLAGS) src/motion-service.cpp src/motion-broker.cpp src/motion-iio.cpp $(motion_libs) -o $@
$(motion_out)/test-sample: tests/motion-sample.cpp src/motion-sample.hpp | $(motion_out)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -UNDEBUG -Isrc $(LDFLAGS) $< -o $@
$(motion_out)/test-ownership: tests/motion-ownership.cpp src/motion-ownership.hpp | $(motion_out)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -UNDEBUG -Isrc $(LDFLAGS) $< -o $@
$(motion_out)/test-journal: tests/motion-journal.cpp src/motion-journal.hpp | $(motion_out)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -UNDEBUG -Isrc $(LDFLAGS) $< -o $@
$(motion_out)/test-bus: tests/motion-bus.cpp src/motion-broker.cpp src/motion-broker.hpp src/motion-source.hpp src/motion-sample.hpp | $(motion_out)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -UNDEBUG -Isrc $(motion_flags) $(LDFLAGS) tests/motion-bus.cpp src/motion-broker.cpp $(motion_libs) -o $@
$(motion_out)/test-polkit: tests/motion-polkit.cpp src/motion-broker.cpp src/motion-broker.hpp src/motion-source.hpp src/motion-sample.hpp | $(motion_out)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -UNDEBUG -Isrc $(motion_flags) $(LDFLAGS) tests/motion-polkit.cpp src/motion-broker.cpp $(motion_libs) -o $@
$(motion_out)/test-watchdog: tests/motion-watchdog.cpp | $(motion_out)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -UNDEBUG $(motion_flags) $(LDFLAGS) $< $(motion_libs) -o $@
check: all $(motion_out)/test-sample $(motion_out)/test-ownership $(motion_out)/test-journal $(motion_out)/test-bus $(motion_out)/test-polkit $(motion_out)/test-watchdog
	ulimit -c 0; $(motion_out)/test-sample
	ulimit -c 0; $(motion_out)/test-ownership
	ulimit -c 0; $(motion_out)/test-journal
	ulimit -c 0; timeout 30s dbus-run-session -- $(motion_out)/test-bus
	ulimit -c 0; timeout 30s dbus-run-session -- $(motion_out)/test-polkit
	ulimit -c 0; timeout 10s $(motion_out)/test-watchdog
