#include "_unity/unity.h"
#include <stdio.h>

// Forward declarations for test runner functions from other files
void run_driver_tests(void);

void run_files_tests(void);

void run_lexer_tests(void);

void run_main_args_tests(void);

void run_ast_tests(void);

void run_parser_tests(void);

void run_strings_tests(void); // Forward declaration for string tests

void run_codegen_tests(void); // Forward declaration for codegen tests

void run_codegen_relational_conditional_tests(void); // Forward declaration for codegen tests

void run_compiler_tests(void); // Forward declaration for integration tests

void run_arena_tests(void); // Forward declaration for arena tests

void run_tac_tests(void);

void run_ast_to_tac_tests(void);

// Global setUp and tearDown for all tests run by this main runner
void setUp(void) {
    // Optional global setup for all tests
}

void tearDown(void) {
    // Optional global teardown for all tests
}

int main(void) {
    UNITY_BEGIN(); // Start Unity test framework

    printf("\n--- Running Driver Tests --- \n");
    run_driver_tests();

    printf("\n--- Running Files Tests --- \n");
    run_files_tests();

    printf("\n--- Running Lexer Tests --- \n");
    run_lexer_tests();

    printf("\n--- Running Main Args Tests --- \n");
    run_main_args_tests();

    printf("\n--- Running AST Tests --- \n");
    run_ast_tests();

    printf("\n--- Running Parser Tests --- \n");
    run_parser_tests();

    printf("\n--- Running Strings Tests --- \n");
    run_strings_tests();

    printf("\n--- Running Arena Tests --- \n");
    run_arena_tests();

    printf("\n--- Running TAC Tests --- \n");
    run_tac_tests();

    printf("\n--- Running AST to TAC Tests --- \n");
    run_ast_to_tac_tests();

    printf("\n--- Running Codegen Tests --- \n");
    run_codegen_tests();

    printf("\n--- Running Codegen Tests --- \n");
    run_codegen_relational_conditional_tests();

    /* -- integration tests -- */
    printf("\n--- Running Compiler Tests --- \n");
    run_compiler_tests();

    return UNITY_END(); // Use UNITY_END() in the main runner
}
