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
