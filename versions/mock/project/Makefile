CXX      := g++
CXXFLAGS := -Wall -std=c++17 -Iinclude -Iprivate -fPIC -shared
DEPFLAGS := -MMD -MP

# Output names
LIB_OUT  := libcea.so
APP_OUT  := myapp

# Directories
SRC_DIR  := src
APP_DIR  := app
OBJ_DIR  := build
BIN_DIR  := bin

# File gathering
LIB_SRCS := $(wildcard $(SRC_DIR)/*.cpp)
APP_SRCS := $(wildcard $(APP_DIR)/*.cpp)

# Map sources to object files in the build directory
LIB_OBJS := $(LIB_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
APP_OBJS := $(APP_SRCS:$(APP_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS     := $(LIB_OBJS:.o=.d) $(APP_OBJS:.o=.d)

# --- Primary Targets ---

all: lib app

# Target 1: Shared Library
lib: $(BIN_DIR)/$(LIB_OUT)

$(BIN_DIR)/$(LIB_OUT): $(LIB_OBJS) | $(BIN_DIR)
	$(CXX) -shared -o $@ $^

# Target 2: Application (links to the library)
app: $(BIN_DIR)/$(APP_OUT)

# We list the .so file as a prerequisite so it builds first
#$(BIN_DIR)/$(APP_OUT): $(APP_OBJS) $(BIN_DIR)/$(LIB_OUT) | $(BIN_DIR)
#	$(CXX) $(APP_OBJS) -o $@ -L$(BIN_DIR) -lcea -Wl,-rpath,'$$ORIGIN'

$(BIN_DIR)/$(APP_OUT): $(APP_OBJS) $(BIN_DIR)/$(LIB_OUT) | $(BIN_DIR)
	$(CXX) $(APP_OBJS) -L$(BIN_DIR) -lcea -Wl,-rpath,'$$ORIGIN' -o $@

# --- Compilation Rules ---

# Rule for library objects
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Rule for application objects
$(OBJ_DIR)/%.o: $(APP_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Directory creation
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Incremental header tracking
-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all lib app clean


#------------------------------------------------------------------------------
#  CXX = g++
#  CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude -Iprivate -std=c++17
#  # 1. Add flags to generate dependency files (.d)
#  DEPFLAGS = -MMD -MP
#  
#  SRC_DIR = src
#  OBJ_DIR = build
#  SRCS = $(wildcard $(SRC_DIR)/*.cpp)
#  OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
#  # 2. Create a list of dependency files
#  DEPS = $(OBJS:.o=.d)
#  
#  TARGET = my_app
#  
#  all: $(TARGET)
#  
#  $(TARGET): $(OBJS)
#  	$(CXX) $(OBJS) -o $(TARGET)
#  
#  # 3. Add $(DEPFLAGS) to the compilation rule
#  $(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
#  	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@
#  
#  $(OBJ_DIR):
#  	mkdir -p $(OBJ_DIR)
#  
#  # 4. Include the generated dependency files
#  -include $(DEPS)
#  
#  clean:
#  	rm -rf $(OBJ_DIR) $(TARGET) libcea.so
#  
#  .PHONY: all clean
#------------------------------------------------------------------------------
#   
#   CXX = g++
#   CXXFLAGS = -fPIC -Wall -Wextra -Iinclude -Iprivate -std=c++17
#   LDFLAGS = -shared
#   
#   # Paths
#   SRC_DIR = src
#   OBJ_DIR = obj
#   LIB_NAME = libcea.so
#   
#   # Source and Object files
#   SRCS = $(wildcard $(SRC_DIR)/*.cpp)
#   OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
#   
#   # Build the Shared Library from object files
#   $(LIB_NAME): $(OBJS)
#   	$(CXX) $(LDFLAGS) -o $@ $^
#   
#   # Incremental Compilation: Only compile changed .cpp files
#   $(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
#   	$(CXX) $(CXXFLAGS) -c $< -o $@
#   
#   # Build the test app linked against the library
#   test_app: test.cpp $(LIB_NAME)
#   	$(CXX) $(CXXFLAGS) test.cpp -o $@ -L. -lcea -Wl,-rpath,.
#   
#   $(OBJ_DIR):
#   	mkdir -p $(OBJ_DIR)
#   
#   clean:
#   	rm -rf $(OBJ_DIR) $(LIB_NAME) test_app
#   
