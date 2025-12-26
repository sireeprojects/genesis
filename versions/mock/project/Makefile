CXX      := g++
CXXFLAGS := -Wall -std=c++17 -Iinclude -Iprivate -fPIC
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

# Map sources to object files
LIB_OBJS := $(LIB_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
APP_OBJS := $(APP_SRCS:$(APP_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS     := $(LIB_OBJS:.o=.d) $(APP_OBJS:.o=.d)

all: lib app

# 1. Build the Shared Library
lib: $(BIN_DIR)/$(LIB_OUT)

$(BIN_DIR)/$(LIB_OUT): $(LIB_OBJS) | $(BIN_DIR)
	$(CXX) -shared -o $@ $^

# 2. Build the App and force link the library
app: $(BIN_DIR)/$(APP_OUT)

$(BIN_DIR)/$(APP_OUT): $(APP_OBJS) $(BIN_DIR)/$(LIB_OUT) | $(BIN_DIR)
	$(CXX) $(APP_OBJS) -o $@ -L$(BIN_DIR) -Wl,--no-as-needed -lcea -Wl,--as-needed -Wl,-rpath,'$$ORIGIN'

# Rule for library objects
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Rule for application objects
$(OBJ_DIR)/%.o: $(APP_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# Directory creation
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Dependency tracking
-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all lib app clean
