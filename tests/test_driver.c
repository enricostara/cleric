#include "unity/unity.h"
#include "../src/compiler/driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void test_run_preprocessor_creates_i_file(void) {
    // Create a temporary C file
    const char *test_c_file = "test_temp.c";
    FILE *f = fopen(test_c_file, "w");
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "#include <stdio.h>\nint main() { return 2; }\n");
    fclose(f);

    // Call the preprocessor function
    int result = run_preprocessor(test_c_file);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Check that the corresponding .i file exists
    char test_i_file[256];
    strcpy(test_i_file, test_c_file);
    const size_t len = strlen(test_i_file);
    test_i_file[len - 2] = '\0';
    strcat(test_i_file, ".i");
    FILE *i_f = fopen(test_i_file, "r");
    TEST_ASSERT_NOT_NULL(i_f);
    fclose(i_f);

    // Clean up
    remove(test_c_file);
    remove(test_i_file);
}

void test_run_compiler_creates_s_file_and_removes_i(void) {
    // Create a temporary .i file
    const char *test_i_file = "test_temp.i";
    FILE *f = fopen(test_i_file, "w");
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "int main(void) { return 2; }\n");
    fclose(f);

    // Run the compile function
    const int result = run_compiler(test_i_file, false, false, false);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Check that the .s file exists
    char s_file[256];
    strcpy(s_file, test_i_file);
    size_t len = strlen(s_file);
    s_file[len - 2] = '\0';
    strcat(s_file, ".s");
    FILE *sf = fopen(s_file, "r");
    TEST_ASSERT_NOT_NULL(sf);
    fclose(sf);

    // Check that the .i file is removed
    sf = fopen(test_i_file, "r");
    TEST_ASSERT_NULL(sf);

    // Clean up
    remove(s_file);
}

void test_run_compiler_lex_only(void) {
    // Prepare a minimal .i file
    const char *test_i_file = "test_lex_only.i";
    FILE *f = fopen(test_i_file, "w");
    fprintf(f, "int main(void) { return 2; }\n");
    fclose(f);
    // Run the compiler in lex_only mode
    const int result = run_compiler(test_i_file, true, false, false);
    TEST_ASSERT_EQUAL_INT(0, result);
    // Should NOT produce a .s file
    char s_file[64];
    strncpy(s_file, test_i_file, strlen(test_i_file) - 2);
    s_file[strlen(test_i_file) - 2] = '\0';
    strcat(s_file, ".s");
    const FILE *sf = fopen(s_file, "r");
    TEST_ASSERT_NULL(sf);
    // Should NOT remove the .i file
    FILE *ifile = fopen(test_i_file, "r");
    TEST_ASSERT_NOT_NULL(ifile);
    fclose(ifile);
    // Cleanup
    remove(test_i_file);
}

void test_run_compiler_parse_only(void) {
    // Prepare a minimal .i file
    const char *test_i_file = "test_parse_only.i";
    FILE *f = fopen(test_i_file, "w");
    fprintf(f, "int main(void) { return 3; }\n");
    fclose(f);
    // Run the compiler in parse_only mode
    const int result = run_compiler(test_i_file, false, true, false);
    TEST_ASSERT_EQUAL_INT(0, result);
    // Should NOT produce a .s file
    char s_file[64];
    strncpy(s_file, test_i_file, strlen(test_i_file) - 2);
    s_file[strlen(test_i_file) - 2] = '\0';
    strcat(s_file, ".s");
    const FILE *sf = fopen(s_file, "r");
    TEST_ASSERT_NULL(sf);
    // Should NOT remove the .i file
    FILE *ifile = fopen(test_i_file, "r");
    TEST_ASSERT_NOT_NULL(ifile);
    fclose(ifile);
    // Cleanup
    remove(test_i_file);
}

void test_run_compiler_codegen_only(void) {
    // Prepare a minimal .i file
    const char *test_i_file = "test_codegen_only.i";
    FILE *f = fopen(test_i_file, "w");
    fprintf(f, "int main(void) { return 4; }\n");
    fclose(f);
    // Run the compiler in codegen_only mode
    const int result = run_compiler(test_i_file, false, false, true);
    TEST_ASSERT_EQUAL_INT(0, result);
    // Should NOT produce a .s file
    char s_file[64];
    strncpy(s_file, test_i_file, strlen(test_i_file) - 2);
    s_file[strlen(test_i_file) - 2] = '\0';
    strcat(s_file, ".s");
    const FILE *sf = fopen(s_file, "r");
    TEST_ASSERT_NULL(sf);
    // Should NOT remove the .i file
    FILE *ifile = fopen(test_i_file, "r");
    TEST_ASSERT_NOT_NULL(ifile);
    fclose(ifile);
    // Cleanup
    remove(test_i_file);
}

void test_run_assembler_linker_creates_executable_and_removes_s(void) {
    // Create a temporary .s file
    const char *test_s_file = "test_temp.s";
    FILE *f = fopen(test_s_file, "w");
    TEST_ASSERT_NOT_NULL(f);
    // Minimal valid x86-64 assembly for main returning 2
    fprintf(f, ".globl\t_main\n_main:\n\tmovl\t$2, %%eax\n\tretq\n");
    fclose(f);

    // Run the assembler/linker function
    int result = run_assembler_linker(test_s_file);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Check that the executable exists
    char test_exe_file[256];
    strcpy(test_exe_file, test_s_file);
    size_t len = strlen(test_exe_file);
    test_exe_file[len - 2] = '\0'; // remove .s
    struct stat st;
    int stat_result = stat(test_exe_file, &st);
    TEST_ASSERT_EQUAL_INT(0, stat_result);
    TEST_ASSERT_TRUE(st.st_mode & S_IXUSR); // Has user execute permission

    // Check that the .s file is removed
    f = fopen(test_s_file, "r");
    TEST_ASSERT_NULL(f);

    // Clean up
    remove(test_exe_file);
}

void run_driver_tests(void) {
    RUN_TEST(test_run_preprocessor_creates_i_file);
    RUN_TEST(test_run_compiler_creates_s_file_and_removes_i);
    RUN_TEST(test_run_compiler_lex_only);
    RUN_TEST(test_run_compiler_parse_only);
    RUN_TEST(test_run_compiler_codegen_only);
    RUN_TEST(test_run_assembler_linker_creates_executable_and_removes_s);
}
