# SPDX-License-Identifier: MIT
.PHONY: check-package-policy
check: check-package-policy
check-package-policy: build/test-package-policy
	bash tests/package-policy.sh
build/test-package-policy: tests/package-policy.cpp src/packages.cpp src/package-operation.hpp src/cli.cpp | build
	$(CXX) $(CXXFLAGS) -UNDEBUG $(LDFLAGS) tests/package-policy.cpp src/cli.cpp -Isrc $(shell pkg-config --cflags --libs libdnf5 rpm libsystemd) -o $@
