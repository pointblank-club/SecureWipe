CXX := g++
CXXFLAGS := -O2 -std=c++20 -static -fno-plt
INCLUDES := -Iinclude

LDLIBS := -lpanelw -lmenuw -lformw -lncursesw -ltinfo

SRC_DIR := src
BUILD_DIR := build
BIN_DIR := bin

TARGET := $(BIN_DIR)/sentinel

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
	$(CXX) $(CXXFLAGS) $(OBJS) \
		-Wl,--start-group $(LDLIBS) -Wl,--end-group \
		-o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean
