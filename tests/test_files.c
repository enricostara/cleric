#include "../src/files/files.h"
#include "unity/unity.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void test_filename_replace_ext_basic(void) {
    char buf[128];
    TEST_ASSERT_NOT_NULL(filename_replace_ext("foo.c", ".i", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("foo.i", buf);
    TEST_ASSERT_NOT_NULL(filename_replace_ext("bar.i", ".s", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("bar.s", buf);
    TEST_ASSERT_NOT_NULL(filename_replace_ext("baz", ".out", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("baz.out", buf);
}

void test_filename_replace_ext_long_name(void) {
    char buf[256];
    const char *longname = "this_is_a_really_long_filename.c";
    TEST_ASSERT_NOT_NULL(filename_replace_ext(longname, ".i", buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("this_is_a_really_long_filename.i", buf);
}

void test_filename_replace_ext_buffer_too_small(void) {
    char buf[6];
    // "foo.i" needs 6 bytes including null
    TEST_ASSERT_NOT_NULL(filename_replace_ext("foo.c", ".i", buf, sizeof(buf)));
    // "foobar.i" needs 8 bytes, so this should fail
    TEST_ASSERT_NULL(filename_replace_ext("foobar.c", ".i", buf, sizeof(buf)));
}

void test_read_entire_file_basic(void) {
    const char *fname = "test_read_file.txt";
    FILE *f = fopen(fname, "w");
    fprintf(f, "Hello, world!\n");
    fclose(f);
    long sz = 0;
    char *contents = read_entire_file(fname, &sz);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_INT(strlen("Hello, world!\n"), sz);
    TEST_ASSERT_EQUAL_STRING("Hello, world!\n", contents);
    free(contents);
    remove(fname);
}

void test_read_entire_file_nonexistent(void) {
    long sz = 123;
    char *contents = read_entire_file("does_not_exist_12345.txt", &sz);
    TEST_ASSERT_NULL(contents);
}

void run_files_tests(void) {
    RUN_TEST(test_filename_replace_ext_basic);
    RUN_TEST(test_filename_replace_ext_long_name);
    RUN_TEST(test_filename_replace_ext_buffer_too_small);
    RUN_TEST(test_read_entire_file_basic);
    RUN_TEST(test_read_entire_file_nonexistent);
}
