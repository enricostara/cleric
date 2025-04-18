#include "unity/unity.h"
#include "library.h"

void test_hello(void) {
    hello();
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, 0, "Hello, World!");
}

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UnityBegin("test_library.c");
    UnityDefaultTestRun(test_hello, "test_hello", 5);
    UnityEnd();
    return 0;
}
