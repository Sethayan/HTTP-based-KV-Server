# ===============================
# Project Settings
# ===============================

CXX      := g++
CC       := gcc
CXXFLAGS := -std=c++17 -O2 -Wall -Icivetweb -Iinclude
CFLAGS   := -std=c11 -DNO_SSL -O2 -Wall
LDFLAGS  := -lpthread -ldl -lmysqlclient

TARGET   := myserver
BUILD    := build

# Source files
CPP_SRC  := src/server.cpp src/cache.cpp src/dbpool.cpp src/async.cpp civetweb/CivetServer.cpp
C_SRC    := civetweb/civetweb.c

# Object files
CPP_OBJ  := $(CPP_SRC:%.cpp=$(BUILD)/%.o)
C_OBJ    := $(C_SRC:%.c=$(BUILD)/%.o)

OBJ      := $(CPP_OBJ) $(C_OBJ)

# ===============================
# Build Rules
# ===============================

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@
	@echo "✔ Build complete → $(TARGET)"

# -------------------------------
# Compile C++ files
# -------------------------------
$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# -------------------------------
# Compile C files
# -------------------------------
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ===============================
# Utility Targets
# ===============================

clean:
	rm -rf $(BUILD) $(TARGET)
	@echo "✔ Cleaned"

# Run server pinned to CPU core 0
run: $(TARGET)
	taskset -c 0 ./$(TARGET)

.PHONY: all clean run
