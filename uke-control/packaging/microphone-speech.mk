# Shared with the retained-SRPM overlay; all compilation runs in COPR.
CPPFLAGS += $(shell pkg-config --cflags libspa-0.2)
build/uke-control: src/microphone-engine.hpp src/microphone-graph.hpp
build/test-microphone-graph: tests/microphone-graph.cpp src/microphone-graph.hpp | build
	$(CXX) -Isrc $(CXXFLAGS) -UNDEBUG $(LDFLAGS) $< $(shell pkg-config --cflags --libs jsoncpp) -o $@
build/test-microphone-engine: tests/microphone-engine.cpp src/microphone-engine.hpp | build
	$(CXX) -Isrc $(CXXFLAGS) -UNDEBUG $(LDFLAGS) $< $(shell pkg-config --cflags --libs jsoncpp libspa-0.2) -ldl -o $@
check-microphone: check-microphone-speech
.PHONY: check-microphone-speech
check-microphone-speech: build/test-microphone-graph build/test-microphone-engine
	build/test-microphone-graph
	build/test-microphone-engine
.PHONY: check-microphone-flow
check: check-microphone-flow
check-microphone-flow:
	bash tests/microphone-pipewire.sh
