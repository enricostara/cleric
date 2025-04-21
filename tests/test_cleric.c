#include "unity/unity.h"

void setUp(void) {
}

void tearDown(void) {
}

void run_driver_tests(void);

void run_files_tests(void);

void run_lexer_tests(void);

int main(void) {
    UNITY_BEGIN();
    run_driver_tests();
    run_files_tests();
    run_lexer_tests();
    return UNITY_END();
}
