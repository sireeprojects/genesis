cmake_minimum_required(VERSION 3.10)
project(CEA_Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Define Directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(APP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/app)

# 2. Gather Source Files (Ensure they are kept separate)
file(GLOB LIB_SRCS "${SRC_DIR}/*.cpp")
file(GLOB APP_SRCS "${APP_DIR}/*.cpp")

# 3. Create the Shared Library
# The 'SHARED' keyword is what creates the .so file
add_library(cea SHARED ${LIB_SRCS})

# 4. Create the Application
# CRITICAL: We only use APP_SRCS. Do not include LIB_SRCS here.
add_executable(myapp ${APP_SRCS})

# 5. Include Paths
target_include_directories(cea PUBLIC include)
target_include_directories(cea PRIVATE private)
target_include_directories(myapp PRIVATE include)

# 6. Dynamic Linking
# This links myapp to libcea.so
target_link_libraries(myapp PRIVATE cea)

# 7. Force Linker to show dependency (Even if no symbols are used yet)
if(UNIX)
    target_link_options(myapp PRIVATE "-Wl,--no-as-needed")
endif()

# 8. RPATH Settings (So the app finds the .so in the same folder)
set_target_properties(myapp PROPERTIES 
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN"
)

# 9. Output directory matching your original bin/ structure
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
