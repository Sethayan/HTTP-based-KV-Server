# ===============================
# Load Generator Makefile
# ===============================

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall
LDFLAGS  := -lcurl -lpthread

TARGET   := loadgen
SRC      := loadgen.cpp

# ===============================
# Build Rules
# ===============================

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)
	@echo "✔ Build complete → $(TARGET)"

# ===============================
# Run (taskset -c 2-5)
# run with: make run T=4 D=20 M=get-put
# ===============================

run: $(TARGET)
	@if [ -z "$(T)" ] || [ -z "$(D)" ] || [ -z "$(M)" ]; then \
		echo "Usage: make run T=<threads> D=<duration> M=<mode>"; \
		exit 1; \
	fi
	taskset -c 2-5 ./$(TARGET) $(T) $(D) $(M)

# ===============================
# Clean
# ===============================

clean:
	rm -f $(TARGET)
	@echo "✔ Cleaned"

.PHONY: all clean run
