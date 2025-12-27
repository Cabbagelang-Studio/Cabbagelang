# Makefile

CMAKE = cmake
MAKE = make

SRC_DIR = src
BUILD_DIR = build
WORKING_DIR = $(shell pwd)

all: cabbage

run: cabbage
	$(WORKING_DIR)/cabbage

cabbage: $(SRC_DIR)/CMakeLists.txt
	$(CMAKE) -S $(SRC_DIR) -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) --config Release

clean:
	rm -rf $(BUILD_DIR) cabbage $(SRC_DIR)/configure/*.h

.PHONY: all clean