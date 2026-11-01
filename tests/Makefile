CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -I../src/include -Ithird_party
BUILD    := build

SRC := $(wildcard ../src/cpp/*.cpp) BuilderPatternTest.cpp
OBJ := $(patsubst %.cpp,$(BUILD)/%.o,$(notdir $(SRC)))

vpath %.cpp ../src/cpp .

.PHONY: all run clean

all: builder_test

run: builder_test
	./builder_test

builder_test: $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

$(BUILD)/%.o: %.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD) builder_test