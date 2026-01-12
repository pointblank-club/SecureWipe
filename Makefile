CXX       := g++
CXXFLAGS  := -Wall -Wextra -O2 -std=c++20
INCLUDES  := -Iinclude
LDFLAGS   :=
LDLIBS    := -lncurses

SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin

TARGET    := $(BIN_DIR)/sentinel

SRCS := \
	$(wildcard $(SRC_DIR)/*.cpp) \
	$(wildcard $(SRC_DIR)/app/*.cpp) \
	$(wildcard $(SRC_DIR)/core/*.cpp) \
	$(wildcard $(SRC_DIR)/flow/*.cpp) \
	$(wildcard $(SRC_DIR)/ui/*.cpp) \
	$(wildcard $(SRC_DIR)/algorithms/*.cpp)

OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

run: all
	sudo ./$(TARGET)

.PHONY: all clean run
