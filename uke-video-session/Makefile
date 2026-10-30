CXX ?= c++
CXXFLAGS ?= -O2
CPPFLAGS += -Isrc $(shell pkg-config --cflags libsystemd)
LDLIBS += $(shell pkg-config --libs libsystemd)
PREFIX ?= /usr
.PHONY: all check install clean
all: uke-video-session
uke-video-session: src/main.cpp src/policy.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Werror src/main.cpp $(LDFLAGS) $(LDLIBS) -o $@
policy-test: tests/policy.cpp src/policy.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++20 -Wall -Wextra -Werror tests/policy.cpp $(LDFLAGS) -o $@
check: all policy-test
	./policy-test
	./uke-video-session --version
	! ./uke-video-session file:///tmp/test
	! ./uke-video-session https://www.youtube.com.evil/watch
	desktop-file-validate packaging/org.uke.VideoSession.desktop
install: all
	install -Dm755 uke-video-session $(DESTDIR)$(PREFIX)/bin/uke-video-session
	install -Dm644 packaging/org.uke.VideoSession.desktop $(DESTDIR)$(PREFIX)/share/applications/org.uke.VideoSession.desktop
clean:
	rm -f uke-video-session policy-test
