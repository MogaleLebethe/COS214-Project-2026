CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra
CPPFLAGS += -I../src/include -Ithird_party
BUILD    := build

# GoogleTest: use Homebrew's prefix when it is installed, otherwise rely on system paths.
GTEST_PREFIX := $(shell p=$$(brew --prefix googletest 2>/dev/null); [ -d "$$p/include" ] && echo "$$p")
ifneq ($(GTEST_PREFIX),)
GTEST_CPPFLAGS := -I$(GTEST_PREFIX)/include
GTEST_LDFLAGS  := -L$(GTEST_PREFIX)/lib
endif
GTEST_LIBS := -lgtest -lgtest_main -lpthread

# Pattern sources are compiled once and linked into both binaries.
LIB_SRC := $(wildcard ../src/cpp/*.cpp)
LIB_OBJ := $(patsubst ../src/cpp/%.cpp,$(BUILD)/src/%.o,$(LIB_SRC))

DOCTEST_OBJ := $(BUILD)/doctest_main.o $(BUILD)/test_builder.o
GTEST_OBJ   := $(BUILD)/test_command.o $(BUILD)/test_strategy.o

.PHONY: all run clean

all: doctest_tests gtest_tests

run: doctest_tests gtest_tests
	./doctest_tests
	./gtest_tests

doctest_tests: $(DOCTEST_OBJ) $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

gtest_tests: $(GTEST_OBJ) $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) $(GTEST_LDFLAGS) -o $@ $^ $(GTEST_LIBS)

$(BUILD)/src/%.o: ../src/cpp/%.cpp | $(BUILD)/src
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/test_command.o $(BUILD)/test_strategy.o: $(BUILD)/%.o: %.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(GTEST_CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/%.o: %.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD) $(BUILD)/src:
	mkdir -p $@

clean:
	rm -rf $(BUILD) doctest_tests gtest_tests tests builder_test
