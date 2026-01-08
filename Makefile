CXX = g++
CXXFLAGS = -Wall -std=c++17 -g

# Source files
SRC = src/main.cpp src/allocator/memory_manager.cpp src/cache/cache.cpp src/allocator/buddy_allocator.cpp src/virtual_memory/virtual_memory.cpp 
# Output executable
TARGET = memsim_app

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
