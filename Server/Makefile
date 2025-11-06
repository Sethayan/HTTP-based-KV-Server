CXX = g++
CXXFLAGS = -Iinclude -I/usr/include/postgresql -Wall -O2
LDFLAGS = -lpq -lpthread

SRC = src/server.cpp src/db.cpp src/cache.cpp
OBJ = $(SRC:.cpp=.o)

TARGET = app

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: all
	./$(TARGET)
