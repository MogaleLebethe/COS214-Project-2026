# Browser GUI build and tests (local, not part of the main build).
#   make tividy_gui     build ./tividy_gui
#   make run            build and start it on http://localhost:8080
#   make test-gui       C++ tests for the API layer (no browser needed)
#   make e2e            drive the real page in headless Chrome (needs Node and Google Chrome)
#   make check-gui      both of the above
CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra
CPPFLAGS += -Isrc/include -Isrc -Itests -Itests/third_party -isystem third_party
LDLIBS   += -pthread
BUILD    := build/gui

ENGINE_SRC := $(wildcard src/cpp/*.cpp)
GUI_SRC    := src/gui/ApiController.cpp src/gui/GuiBootstrap.cpp
LIB_OBJ    := $(patsubst %.cpp,$(BUILD)/%.o,$(ENGINE_SRC) $(GUI_SRC))

.PHONY: all run test-gui e2e check-gui clean-gui

all: tividy_gui

run: tividy_gui
	./tividy_gui

tividy_gui: $(BUILD)/app/gui_main.o $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

gui_tests: $(BUILD)/tests/doctest_main.o $(BUILD)/tests/test_api.o $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

test-gui: gui_tests
	./gui_tests

e2e: tividy_gui
	node tests/gui_e2e.mjs

check-gui: test-gui e2e

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean-gui:
	rm -rf $(BUILD) tividy_gui gui_tests
