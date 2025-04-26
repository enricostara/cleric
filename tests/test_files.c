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

// --- Tests for filename_has_ext ---
void test_filename_has_ext_true(void) {
    TEST_ASSERT_TRUE(filename_has_ext("file.c", ".c"));
    TEST_ASSERT_TRUE(filename_has_ext("path/to/file.txt", ".txt"));
    TEST_ASSERT_TRUE(filename_has_ext(".hidden.c", ".c"));
    TEST_ASSERT_TRUE(filename_has_ext("a.very.long.name.s", ".s"));
}

void test_filename_has_ext_false(void) {
    TEST_ASSERT_FALSE(filename_has_ext("file.c", ".C")); // Case sensitive
    TEST_ASSERT_FALSE(filename_has_ext("file.c", ".cpp"));
    TEST_ASSERT_FALSE(filename_has_ext("file.c", "file.c")); // Ext must be shorter
    TEST_ASSERT_FALSE(filename_has_ext("file", ".c"));
    TEST_ASSERT_FALSE(filename_has_ext("file.c.txt", ".c"));
    TEST_ASSERT_FALSE(filename_has_ext(".c", ".c")); // Ext must be shorter
}

void test_filename_has_ext_edge_cases(void) {
    TEST_ASSERT_FALSE(filename_has_ext(NULL, ".c"));
    TEST_ASSERT_FALSE(filename_has_ext("file.c", NULL));
    TEST_ASSERT_FALSE(filename_has_ext("", ".c"));
    TEST_ASSERT_FALSE(filename_has_ext("file.c", ""));
    TEST_ASSERT_FALSE(filename_has_ext("", ""));
}

// --- Tests for write_string_to_file ---
void test_write_string_to_file_success(void) {
    const char *filename = "./test_write_output.txt";
    const char *content = "Hello, World!\nThis is a test.";
    bool result = write_string_to_file(filename, content);
    TEST_ASSERT_TRUE(result);

    // Verify content
    long size = 0;
    char *read_content = read_entire_file(filename, &size);
    TEST_ASSERT_NOT_NULL(read_content);
    TEST_ASSERT_EQUAL_STRING(content, read_content);
    TEST_ASSERT_EQUAL(strlen(content), size);

    free(read_content);
    remove(filename); // Clean up
}

void test_write_string_to_file_overwrite(void) {
    const char *filename = "./test_write_overwrite.txt";
    const char *content1 = "Initial content.";
    const char *content2 = "Overwritten content.";

    // Write initial
    TEST_ASSERT_TRUE(write_string_to_file(filename, content1));
    // Overwrite
    TEST_ASSERT_TRUE(write_string_to_file(filename, content2));

    // Verify overwritten content
    long size = 0;
    char *read_content = read_entire_file(filename, &size);
    TEST_ASSERT_NOT_NULL(read_content);
    TEST_ASSERT_EQUAL_STRING(content2, read_content);
    TEST_ASSERT_EQUAL(strlen(content2), size);

    free(read_content);
    remove(filename); // Clean up
}

void test_write_string_to_file_invalid_args(void) {
    TEST_ASSERT_FALSE(write_string_to_file(NULL, "content"));
    TEST_ASSERT_FALSE(write_string_to_file("filename", NULL));
    // Attempting to write to a directory (should fail)
    // Note: Behavior might differ slightly based on OS permissions
    // For simplicity, we check if it returns false, assuming it cannot write.
    TEST_ASSERT_FALSE(write_string_to_file(".", "content")); // Writing to current dir
}

void run_files_tests(void) {
    RUN_TEST(test_filename_replace_ext_basic);
    RUN_TEST(test_filename_replace_ext_long_name);
    RUN_TEST(test_filename_replace_ext_buffer_too_small);
    RUN_TEST(test_read_entire_file_basic);
    RUN_TEST(test_read_entire_file_nonexistent);
    RUN_TEST(test_filename_has_ext_true);
    RUN_TEST(test_filename_has_ext_false);
    RUN_TEST(test_filename_has_ext_edge_cases);
    RUN_TEST(test_write_string_to_file_success);
    RUN_TEST(test_write_string_to_file_overwrite);
    RUN_TEST(test_write_string_to_file_invalid_args);
}
