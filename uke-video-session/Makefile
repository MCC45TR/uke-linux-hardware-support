CXX ?= c++
CXXFLAGS ?= -O2
CPPFLAGS += -Isrc $(shell pkg-config --cflags libsystemd)
LDLIBS += $(shell pkg-config --libs libsystemd)
PREFIX ?= /usr
.PHONY: all check install clean
all: uke-video-session
uke-video-session: src/main.cpp src/affinity.cpp src/affinity.hpp src/policy.hpp src/protocol.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Werror src/main.cpp src/affinity.cpp $(LDFLAGS) $(LDLIBS) -o $@
policy-test: tests/policy.cpp src/policy.hpp src/protocol.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Werror tests/policy.cpp $(LDFLAGS) -o $@
affinity-test: tests/affinity.cpp src/affinity.cpp src/affinity.hpp src/policy.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Werror tests/affinity.cpp src/affinity.cpp $(LDFLAGS) -o $@
check: all policy-test affinity-test
	./policy-test
	./affinity-test
	./uke-video-session --version
	bash tests/bus-smoke.sh "$(CURDIR)/uke-video-session"
install: all
	install -Dm755 uke-video-session $(DESTDIR)$(PREFIX)/libexec/uke-video-session
	install -Dm644 packaging/uke-video-session.service $(DESTDIR)$(PREFIX)/lib/systemd/user/uke-video-session.service
	install -Dm644 packaging/org.uke.VideoSession1.service $(DESTDIR)$(PREFIX)/share/dbus-1/services/org.uke.VideoSession1.service
	install -Dm644 packaging/org.uke.VideoSession1.xml $(DESTDIR)$(PREFIX)/share/dbus-1/interfaces/org.uke.VideoSession1.xml
clean:
	rm -f uke-video-session policy-test affinity-test
