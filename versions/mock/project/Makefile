CXX = g++
CXXFLAGS = -fPIC -Wall -Wextra -Iinclude -Iprivate -std=c++17
LDFLAGS = -shared

# Paths
SRC_DIR = src
OBJ_DIR = obj
LIB_NAME = libcea.so

# Source and Object files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

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

