cmake_minimum_required(VERSION 3.31)
project(cleric C)

set(CMAKE_C_STANDARD 99)

# Set output directory for executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/build)

add_executable(cleric
        src/main.c
        src/library.c
)

# Add include directory
include_directories(include)

# Enable testing
enable_testing()

# Add Unity framework
add_library(unity tests/unity/unity.c)

# Add test executable
add_executable(test_library tests/test_library.c src/library.c)
target_link_libraries(test_library unity)
target_include_directories(test_library PRIVATE include tests/unity)

# Register test with CTest
add_test(NAME library_test COMMAND test_library)
