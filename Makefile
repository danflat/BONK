CXX ?= c++
CPPFLAGS ?= -Iinclude
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror

BUILD_DIR := build
LIB_SOURCES := $(sort $(wildcard src/*.cpp))
HEADERS := $(sort $(wildcard include/bonk/*.hpp))
TEST_BINARY := $(BUILD_DIR)/bonk_tests
DEMO_BINARY := $(BUILD_DIR)/scheduler_demo

.PHONY: all test demo test-sanitize clean

all: test demo

$(BUILD_DIR):
	mkdir -p $@

$(TEST_BINARY): $(LIB_SOURCES) tests/test_main.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIB_SOURCES) tests/test_main.cpp -o $@

$(DEMO_BINARY): $(LIB_SOURCES) examples/scheduler_demo.cpp $(HEADERS) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIB_SOURCES) examples/scheduler_demo.cpp -o $@

test: $(TEST_BINARY)
	./$(TEST_BINARY)

demo: $(DEMO_BINARY)

test-sanitize: CXXFLAGS := -std=c++17 -O1 -g -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror -fsanitize=address,undefined -fno-omit-frame-pointer
test-sanitize: clean $(TEST_BINARY)
	# LeakSanitizer cannot inspect processes in some container/ptrace runners.
	# The portable core performs no dynamic allocation; ASan and UBSan stay on.
	ASAN_OPTIONS=detect_leaks=0 ./$(TEST_BINARY)

clean:
	rm -rf $(BUILD_DIR)
