cmake_minimum_required(VERSION 3.31)
project(cleric C)

# Set the C language standard to C99 for all targets
set(CMAKE_C_STANDARD 99)

# On macOS, force all builds to target the x86_64 architecture (Intel 64-bit)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES x86_64 CACHE STRING "Build architectures for Mac OS X" FORCE)
endif()

# Set the output directory for all executables and libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/build)


# Enable CTest-based testing support (allows you to run tests with `ctest`)
enable_testing()

# ------------------------
# Add Unity test framework
# ------------------------
# Unity is a unit testing framework for C (https://www.throwtheswitch.org/unity)
add_library(unity tests/unity/unity.c)

# --------------------------------------
# Main application executable: 'cleric'
# --------------------------------------
# This is the main compiler driver, combining the CLI and driver logic
add_executable(cleric
        src/cleric.c

        src/driver/driver.c
        src/lexer/lexer.c
        src/files/files.c
        src/args/args.c
        src/parser/ast.c
        src/parser/parser.c
        src/strings/strings.c
        src/codegen/codegen.c
)

# --------------------------------------
# Test executable: 'test_all'
# --------------------------------------
# This executable runs all unit tests for the project.
# It includes multiple test files and the driver logic.
add_executable(test_all
        tests/test_all.c
        tests/test_driver.c
        tests/test_lexer.c
        tests/test_files.c
        tests/test_args.c
        tests/test_ast.c
        tests/test_parser.c
        tests/test_strings.c
        tests/test_codegen.c
        src/driver/driver.c
        src/lexer/lexer.c
        src/files/files.c
        src/args/args.c
        src/parser/ast.c
        src/parser/parser.c
        src/strings/strings.c
        src/codegen/codegen.c
)
target_link_libraries(test_all unity)
target_include_directories(test_all PRIVATE include tests/unity src)

# Register the test executable with CTest, so `ctest` will run it
add_test(test_all test_all)