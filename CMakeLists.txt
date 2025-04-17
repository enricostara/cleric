cmake_minimum_required(VERSION 3.31)
project(cleric C)

set(CMAKE_C_STANDARD 99)

# Set output directory for executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/build)

add_executable(cleric
        src/library.c
)

# Add include directory
include_directories(include)




