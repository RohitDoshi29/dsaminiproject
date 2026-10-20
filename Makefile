CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2 -Iinclude

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

TARGET := $(BIN_DIR)/courier_tracker
SRCS := $(SRC_DIR)/main.cpp $(SRC_DIR)/ParcelManager.cpp
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run single test

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Single file build target
single: courier_tracker.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) courier_tracker.cpp -o $(TARGET)

run: all
	./$(TARGET)

test: all
	bash tests/test_demo.sh

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
