#include "unity/unity.h"
#include "../src/driver/driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
    fprintf(f, "int main() { return 2; }\n");
    fclose(f);

    // Run the compile function
    int result = run_compiler(test_i_file);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Check that the .s file exists
    char s_file[256];
    strcpy(s_file, test_i_file);
    size_t len = strlen(s_file);
    s_file[len-2] = '\0';
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
