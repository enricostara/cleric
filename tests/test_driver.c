#include "unity/unity.h"
#include "driver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

void test_run_preprocessor_creates_i_file(void) {
    // Create a temporary C file
    const char *test_c_file = "test_temp.c";
    FILE *f = fopen(test_c_file, "w");
    TEST_ASSERT_NOT_NULL(f);
    fprintf(f, "#include <stdio.h>\nint main() { return 0; }\n");
    fclose(f);

    // Call the preprocessor function
    int result = run_preprocessor(test_c_file);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Check that the .i file exists
    char i_file[256];
    strcpy(i_file, test_c_file);
    size_t len = strlen(i_file);
    i_file[len-2] = '\0';
    strcat(i_file, ".i");
    FILE *i_f = fopen(i_file, "r");
    TEST_ASSERT_NOT_NULL(i_f);
    fclose(i_f);

    // Clean up
    remove(test_c_file);
    remove(i_file);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_run_preprocessor_creates_i_file);
    return UNITY_END();
}
