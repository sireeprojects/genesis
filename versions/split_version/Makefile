CXX = g++
CXXFLAGS = -fPIC -Wall -Wextra -Iinclude -std=c++17
LDFLAGS = -shared

# Paths
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
LIB_NAME = libcea.so

# Source and Object files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Default: build library and the test application
all: $(LIB_NAME) test_app

# Build the Shared Library from object files
$(LIB_NAME): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

# Incremental Compilation: Only compile changed .cpp files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build the test app linked against the library
test_app: test.cpp $(LIB_NAME)
	$(CXX) $(CXXFLAGS) test.cpp -o $@ -L. -lcea -Wl,-rpath,.

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(LIB_NAME) test_app
