#include "unity/unity.h"

void setUp(void) {
}

void tearDown(void) {
}

void test_run_preprocessor_creates_i_file(void);
void test_run_compiler_creates_s_file_and_removes_i(void);

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_run_preprocessor_creates_i_file);
    RUN_TEST(test_run_compiler_creates_s_file_and_removes_i);
    return UNITY_END();
}
