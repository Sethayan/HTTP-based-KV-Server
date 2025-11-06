# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -O2

# Libraries to link
LIBS = -lpthread -lcurl

# Target executable
TARGET = loadgen

# Source files
SRCS = loadgen.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default rule
all: $(TARGET)

# Build binary
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# Compile source to object
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run program
run: $(TARGET)
	./$(TARGET)

# Clean build files
clean:
	rm -f $(TARGET) $(OBJS)
