CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

TARGET := $(BIN_DIR)/courier_tracker
SRCS := $(SRC_DIR)/main.cpp $(SRC_DIR)/ParcelManager.cpp $(SRC_DIR)/WebServer.cpp
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

PORT ?= 5000

.PHONY: all clean run web single test unit

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Single file build targets
single: courier_tracker.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) courier_tracker.cpp -o $(TARGET)

system: courier_system.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) courier_system.cpp -o $(BIN_DIR)/courier_system

run: all
	./$(TARGET)

web: all
	./$(TARGET) --web $(PORT)

TEST_TARGET := $(BIN_DIR)/unit_tests

unit: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): tests/unit_tests.cpp $(SRC_DIR)/ParcelManager.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: all unit
	bash tests/test_demo.sh
	bash tests/test_web.sh

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
