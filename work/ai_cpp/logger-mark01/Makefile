CXX = g++
CXXFLAGS = -Wall -Wextra -Iinclude -std=c++11
TARGET = myapp

# Find all source files
SRCS = src/Logger.cpp app/main.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(TARGET) *.log build_output
