#include "unity/unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_run_preprocessor_creates_i_file(void);
void test_run_compiler_creates_s_file_and_removes_i(void);
void test_run_compiler_lex_only(void);
void test_run_assembler_linker_creates_executable_and_removes_s(void);
void test_tokenize_minimal_c(void);
void test_tokenize_unknown_token(void);

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_run_preprocessor_creates_i_file);
    RUN_TEST(test_run_compiler_creates_s_file_and_removes_i);
    RUN_TEST(test_run_compiler_lex_only);
    RUN_TEST(test_run_assembler_linker_creates_executable_and_removes_s);
    RUN_TEST(test_tokenize_minimal_c);
    RUN_TEST(test_tokenize_unknown_token);
    return UNITY_END();
}
