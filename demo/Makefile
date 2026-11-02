CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra
CPPFLAGS += -I../src/include -I../tests/third_party
BUILD    := build

LIB_SRC := $(wildcard ../src/cpp/*.cpp)
LIB_OBJ := $(patsubst ../src/cpp/%.cpp,$(BUILD)/src/%.o,$(LIB_SRC))

.PHONY: all run hospital agency clean

all: demo

run: demo
	./demo

hospital: demo
	./demo hospital

agency: demo
	./demo agency

demo: $(BUILD)/main.o $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/src/%.o: ../src/cpp/%.cpp | $(BUILD)/src
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/main.o: main.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD) $(BUILD)/src:
	mkdir -p $@

clean:
	rm -rf $(BUILD) demo
