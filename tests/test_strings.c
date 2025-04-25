#include "unity.h"
#include "../src/strings/strings.h"
#include <stdlib.h>
#include <string.h>

// --- Test Cases ---

// Test basic initialization
static void test_string_buffer_init_basic(void) {
    StringBuffer sb;
    string_buffer_init(&sb, 64);
    TEST_ASSERT_NOT_NULL(sb.buffer);
    TEST_ASSERT_EQUAL_size_t(64, sb.capacity);
    TEST_ASSERT_EQUAL_size_t(0, sb.length);
    TEST_ASSERT_EQUAL_STRING("", sb.buffer);
    string_buffer_clear(&sb);
}

// Test initialization with zero capacity (should use a default)
static void test_string_buffer_init_zero_capacity(void) {
    StringBuffer sb;
    string_buffer_init(&sb, 0); // Should use a default internal capacity
    TEST_ASSERT_NOT_NULL(sb.buffer);
    TEST_ASSERT_GREATER_THAN_size_t(0, sb.capacity);
    TEST_ASSERT_EQUAL_size_t(0, sb.length);
    TEST_ASSERT_EQUAL_STRING("", sb.buffer);
    string_buffer_clear(&sb);
}

// Test simple appends without reallocation
static void test_string_buffer_append_simple(void) {
    StringBuffer sb;
    string_buffer_init(&sb, 64);
    string_buffer_append(&sb, "Hello");
    TEST_ASSERT_EQUAL_size_t(5, sb.length);
    TEST_ASSERT_EQUAL_STRING("Hello", sb.buffer);
    string_buffer_append(&sb, ", World!");
    TEST_ASSERT_EQUAL_size_t(13, sb.length);
    TEST_ASSERT_EQUAL_STRING("Hello, World!", sb.buffer);
    string_buffer_clear(&sb);
}

// Test appending formatted strings
static void test_string_buffer_append_formatted(void) {
    StringBuffer sb;
    string_buffer_init(&sb, 64);
    int year = 2024;
    const char *event = "Test";
    string_buffer_append(&sb, "Event: %s, Year: %d", event, year);
    TEST_ASSERT_EQUAL_size_t(strlen("Event: Test, Year: 2024"), sb.length);
    TEST_ASSERT_EQUAL_STRING("Event: Test, Year: 2024", sb.buffer);
    string_buffer_clear(&sb);
}

// Test appending single characters
static void test_string_buffer_append_char_simple(void) {
    StringBuffer sb;
    string_buffer_init(&sb, 8);
    string_buffer_append_char(&sb, 'A');
    string_buffer_append_char(&sb, 'B');
    string_buffer_append_char(&sb, 'C');
    TEST_ASSERT_EQUAL_size_t(3, sb.length);
    TEST_ASSERT_EQUAL_STRING("ABC", sb.buffer);
    string_buffer_clear(&sb);
}

// Test forcing reallocation with append
static void test_string_buffer_append_realloc(void) {
    StringBuffer sb;
    string_buffer_init(&sb, 4); // Small initial capacity
    TEST_ASSERT_EQUAL_size_t(4, sb.capacity);
    string_buffer_append(&sb, "123"); // Fits
    TEST_ASSERT_EQUAL_size_t(3, sb.length);
    TEST_ASSERT_EQUAL_STRING("123", sb.buffer);
    string_buffer_append(&sb, "4567"); // Forces realloc
    TEST_ASSERT_EQUAL_size_t(7, sb.length);
    TEST_ASSERT_GREATER_THAN_size_t(7, sb.capacity); // Capacity should have increased
    TEST_ASSERT_EQUAL_STRING("1234567", sb.buffer);
    string_buffer_clear(&sb);
}

// Test forcing reallocation with append_char
static void test_string_buffer_append_char_realloc(void) {
    StringBuffer sb;
    string_buffer_init(&sb, 3); // Very small capacity
    TEST_ASSERT_EQUAL_size_t(3, sb.capacity);
    string_buffer_append_char(&sb, 'X'); // 1
    string_buffer_append_char(&sb, 'Y'); // 2 - Fits (needs 3 capacity for 'XY\0')
    TEST_ASSERT_EQUAL_size_t(2, sb.length);
    TEST_ASSERT_EQUAL_STRING("XY", sb.buffer);
    TEST_ASSERT_EQUAL_size_t(3, sb.capacity);
    string_buffer_append_char(&sb, 'Z'); // 3 - Forces realloc (needs 4 capacity for 'XYZ\0')
    TEST_ASSERT_EQUAL_size_t(3, sb.length);
    TEST_ASSERT_GREATER_THAN_size_t(3, sb.capacity);
    TEST_ASSERT_EQUAL_STRING("XYZ", sb.buffer);
    string_buffer_clear(&sb);
}

// Test appending empty string
static void test_string_buffer_append_empty(void) {
    StringBuffer sb;
    string_buffer_init(&sb, 16);
    string_buffer_append(&sb, "");
    TEST_ASSERT_EQUAL_size_t(0, sb.length);
    TEST_ASSERT_EQUAL_STRING("", sb.buffer);
    string_buffer_append(&sb, "Data");
    string_buffer_append(&sb, "");
    TEST_ASSERT_EQUAL_size_t(4, sb.length);
    TEST_ASSERT_EQUAL_STRING("Data", sb.buffer);
    string_buffer_clear(&sb);
}

// Test getting content (transfers ownership)
static void test_string_buffer_get_content(void) {
    StringBuffer sb;
    string_buffer_init(&sb, 16);
    string_buffer_append(&sb, "Test Data");
    TEST_ASSERT_EQUAL_size_t(9, sb.length);

    char *content = string_buffer_get_content(&sb);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL_STRING("Test Data", content);

    // Check if sb is reset after getting content
    TEST_ASSERT_NULL(sb.buffer);
    TEST_ASSERT_EQUAL_size_t(0, sb.capacity);
    TEST_ASSERT_EQUAL_size_t(0, sb.length);

    // Caller must free the content
    free(content);
}

// Test freeing data
static void test_string_buffer_free_data_simple(void) {
    StringBuffer sb;
    string_buffer_init(&sb, 16);
    string_buffer_append(&sb, "Some content");
    TEST_ASSERT_NOT_NULL(sb.buffer);

    string_buffer_clear(&sb);
    TEST_ASSERT_NULL(sb.buffer);
    TEST_ASSERT_EQUAL_size_t(0, sb.capacity);
    TEST_ASSERT_EQUAL_size_t(0, sb.length);
}

// Test freeing an already empty/freed buffer (should be safe)
static void test_string_buffer_free_data_empty(void) {
    StringBuffer sb;
    string_buffer_init(&sb, 1);
    string_buffer_clear(&sb);
    TEST_ASSERT_NULL(sb.buffer);
    string_buffer_clear(&sb); // Free again - should not crash
    TEST_PASS(); // If we didn't crash
}

// --- Test Runner --- -

// We need a function to run all tests in this suite
void run_strings_tests(void) {
    RUN_TEST(test_string_buffer_init_basic);
    RUN_TEST(test_string_buffer_init_zero_capacity);
    RUN_TEST(test_string_buffer_append_simple);
    RUN_TEST(test_string_buffer_append_formatted);
    RUN_TEST(test_string_buffer_append_char_simple);
    RUN_TEST(test_string_buffer_append_realloc);
    RUN_TEST(test_string_buffer_append_char_realloc);
    RUN_TEST(test_string_buffer_append_empty);
    RUN_TEST(test_string_buffer_get_content);
    RUN_TEST(test_string_buffer_free_data_simple);
    RUN_TEST(test_string_buffer_free_data_empty);
}

