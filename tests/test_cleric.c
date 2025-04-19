#include "unity/unity.h"

void setUp(void) {
}

void tearDown(void) {
}

void test_run_preprocessor_creates_i_file(void);

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_run_preprocessor_creates_i_file);
    return UNITY_END();
}
