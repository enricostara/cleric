cmake_minimum_required(VERSION 3.31)
project(cleric C)

set(CMAKE_C_STANDARD 99)

# Force x86_64 architecture on macOS
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES x86_64 CACHE STRING "Build architectures for Mac OS X" FORCE)
endif()

# Set output directory for executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/build)

# Add include directory
include_directories(include)

# Enable testing
enable_testing()

# Add Unity framework
add_library(unity tests/unity/unity.c)


# Add driver executable
add_executable(driver src/driver.c)

# Add test_driver target
add_executable(test_driver tests/test_driver.c src/driver.c)
target_link_libraries(test_driver unity)
target_include_directories(test_driver PRIVATE include tests/unity)

# Register test with CTest
add_test(NAME driver_test COMMAND test_driver)
