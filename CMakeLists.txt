cmake_minimum_required(VERSION 3.15)
project(cleric C)

# Set the C language standard to C99 for all targets
set(CMAKE_C_STANDARD 99)

# On macOS, force all builds to target the x86_64 architecture (Intel 64-bit)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES x86_64 CACHE STRING "Build architectures for Mac OS X" FORCE)
endif()

# Set the output directory for all executables and libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/build)

# Enable AddressSanitizer flags for debugging memory errors
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address") # If you have shared libs
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fsanitize=address") # If you have modules
endif()

# Enable CTest-based testing support (allows you to run tests with `ctest`)
enable_testing()

# ------------------------
# Add Unity test framework
# ------------------------
# Unity is a unit testing framework for C (https://www.throwtheswitch.org/unity)
add_library(unity tests/_unity/unity.c)

# --------------------------------------
# Main application executable: 'cleric'
# --------------------------------------
# This is the main compiler driver, combining the CLI and driver logic
add_executable(cleric
        src/cleric.c

        src/compiler/driver.c
        src/compiler/compiler.c
        src/lexer/lexer.c
        src/files/files.c
        src/args/args.c
        src/parser/ast.c
        src/parser/parser.c
        src/strings/strings.c
        src/codegen/codegen.c
        src/memory/arena.h
        src/memory/arena.c
        src/ir/tac.c
        src/ir/ast_to_tac.c
)
target_include_directories(cleric PRIVATE src)

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
        tests/parser/test_parser_program.c
        tests/parser/test_parser_unary_expressions.c
        tests/parser/test_parser_binary_expressions.c
        tests/parser/test_parser_relational_logical_expressions.c
        tests/parser/test_parser_precedence.c
        tests/parser/test_parser_errors_and_literals.c
        tests/parser/test_parser_blocks_declarations.c
        tests/parser/test_parser_assignments.c
        tests/test_strings.c
        tests/codegen/test_codegen.c
        tests/codegen/test_codegen_logical.c
        tests/codegen/test_codegen_relational_conditional.c
        tests/test_compiler.c
        tests/test_arena.c
        tests/test_tac.c
        tests/test_ast_to_tac.c
        src/compiler/driver.c
        src/compiler/compiler.c
        src/lexer/lexer.c
        src/files/files.c
        src/args/args.c
        src/parser/ast.c
        src/parser/parser.c
        src/strings/strings.c
        src/codegen/codegen.c
        src/memory/arena.c
        src/ir/tac.c
        src/ir/ast_to_tac.c
)
target_link_libraries(test_all unity)
target_include_directories(test_all PRIVATE include tests/_unity src)

# Register the test executable with CTest, so `ctest` will run it
add_test(test_all test_all)
