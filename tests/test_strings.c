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

// Test getting content (both read-only and ownership transfer)
static void test_string_buffer_content_access(void) {
    StringBuffer sb;
    string_buffer_init(&sb, 10);
    string_buffer_append(&sb, "Hello");

    // Test read-only access with content_str
    const char *content_ro = string_buffer_content_str(&sb);
    TEST_ASSERT_EQUAL_STRING("Hello", content_ro);
    TEST_ASSERT_EQUAL(5, sb.length); // Buffer should be unchanged
    TEST_ASSERT_NOT_NULL(sb.buffer); // Buffer should still be attached

    // Test getting content from empty buffer
    string_buffer_clear(&sb); // Clear the buffer first
    const char *empty_ro = string_buffer_content_str(&sb); // Should return ""
    TEST_ASSERT_EQUAL_STRING("", empty_ro);
    TEST_ASSERT_EQUAL(0, sb.length);

    string_buffer_destroy(&sb); // Should be safe even if buffer is NULL
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
    RUN_TEST(test_string_buffer_content_access);
    RUN_TEST(test_string_buffer_free_data_simple);
    RUN_TEST(test_string_buffer_free_data_empty);
}
