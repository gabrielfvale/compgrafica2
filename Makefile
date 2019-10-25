TARGET := program.out

BUILD_DIR := build
SRC_DIR := src

CC := g++
CFLAGS := -std=c++11 -O -Wall -g
LDFLAGS := -lGL -lGLU -lglut

SRC_FILES := $(shell find $(SRC_DIR)/ -name "*.cpp")
OBJ_FILES := $(SRC_FILES:%=$(BUILD_DIR)/%.o)

$(BUILD_DIR)/$(TARGET): $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

fresh: clean $(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)
