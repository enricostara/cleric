#include "unity.h"
#include "../src/strings/strings.h"
#include "../src/memory/arena.h"

// --- Test Cases ---

// Test basic initialization
static void test_string_buffer_init_basic(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(test_arena.start);
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 64);

    TEST_ASSERT_NOT_NULL(sb.buffer);
    TEST_ASSERT_GREATER_OR_EQUAL(64, sb.capacity);
    TEST_ASSERT_EQUAL(0, sb.length);
    TEST_ASSERT_EQUAL('\0', sb.buffer[0]);

    arena_destroy(&test_arena);
}

// Test initialization with zero capacity (should use internal default)
static void test_string_buffer_init_zero_capacity(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(test_arena.start);
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 0); // Should use a default internal capacity

    TEST_ASSERT_NOT_NULL(sb.buffer);
    TEST_ASSERT_GREATER_THAN(0, sb.capacity);
    TEST_ASSERT_EQUAL(0, sb.length);
    TEST_ASSERT_EQUAL('\0', sb.buffer[0]);

    arena_destroy(&test_arena);
}

// Test simple append
static void test_string_buffer_append_simple(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(test_arena.start);
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 64);
    string_buffer_append(&sb, "Hello");

    TEST_ASSERT_EQUAL_STRING("Hello", sb.buffer);
    TEST_ASSERT_EQUAL(5, sb.length);

    string_buffer_append(&sb, ", World!");
    TEST_ASSERT_EQUAL_STRING("Hello, World!", sb.buffer);
    TEST_ASSERT_EQUAL(13, sb.length);

    arena_destroy(&test_arena);
}

// Test formatted append
static void test_string_buffer_append_formatted(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(test_arena.start);
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 64);
    const char *event = "Launch";
    int year = 2024;

    string_buffer_append(&sb, "Event: %s, Year: %d", event, year);
    TEST_ASSERT_EQUAL_STRING("Event: Launch, Year: 2024", sb.buffer);
    TEST_ASSERT_EQUAL(25, sb.length);

    arena_destroy(&test_arena);
}

// Test appending single characters
static void test_string_buffer_append_char_simple(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(test_arena.start);
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 8);
    string_buffer_append_char(&sb, 'A');
    string_buffer_append_char(&sb, 'B');
    string_buffer_append_char(&sb, 'C');

    TEST_ASSERT_EQUAL_STRING("ABC", sb.buffer);
    TEST_ASSERT_EQUAL(3, sb.length);

    arena_destroy(&test_arena);
}

// Test appending that forces reallocation
static void test_string_buffer_append_realloc(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(test_arena.start);
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 4); // Small initial capacity

    string_buffer_append(&sb, "123"); // Fits
    TEST_ASSERT_EQUAL_STRING("123", sb.buffer);
    TEST_ASSERT_EQUAL(3, sb.length);

    string_buffer_append(&sb, "4567"); // Forces realloc
    TEST_ASSERT_EQUAL_STRING("1234567", sb.buffer);
    TEST_ASSERT_EQUAL(7, sb.length);
    TEST_ASSERT_GREATER_OR_EQUAL(8, sb.capacity); // Capacity should have increased

    arena_destroy(&test_arena);
}

// Test append_char that forces reallocation
static void test_string_buffer_append_char_realloc(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(test_arena.start);
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 3); // Very small capacity (needs 3 for "XY\0")

    string_buffer_append_char(&sb, 'X'); // 1
    string_buffer_append_char(&sb, 'Y'); // 2 - Fits (needs 3 capacity for 'XY\0')
    TEST_ASSERT_EQUAL_STRING("XY", sb.buffer);
    TEST_ASSERT_EQUAL(2, sb.length);
    size_t capacity_before = sb.capacity;

    string_buffer_append_char(&sb, 'Z'); // 3 - Forces realloc (needs 4 capacity for 'XYZ\0')
    TEST_ASSERT_EQUAL_STRING("XYZ", sb.buffer);
    TEST_ASSERT_EQUAL(3, sb.length);
    TEST_ASSERT_GREATER_OR_EQUAL(4, sb.capacity); // Capacity must be at least 4 now

    arena_destroy(&test_arena);
}

// Test appending empty strings
static void test_string_buffer_append_empty(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(test_arena.start);
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 16);
    string_buffer_append(&sb, "");

    TEST_ASSERT_EQUAL_STRING("", sb.buffer);
    TEST_ASSERT_EQUAL(0, sb.length);

    string_buffer_append(&sb, "Data");
    string_buffer_append(&sb, "");
    TEST_ASSERT_EQUAL_STRING("Data", sb.buffer);
    TEST_ASSERT_EQUAL(4, sb.length);

    arena_destroy(&test_arena);
}

// Test content access and clearing/resetting
static void test_string_buffer_content_access_and_reset(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(test_arena.start);
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 10);
    string_buffer_append(&sb, "Hello");

    const char *content1 = string_buffer_content_str(&sb);
    TEST_ASSERT_EQUAL_STRING("Hello", content1);
    TEST_ASSERT_EQUAL(5, sb.length);
    TEST_ASSERT_NOT_NULL(content1);

    // Test reset
    string_buffer_reset(&sb);
    TEST_ASSERT_EQUAL(0, sb.length);
    TEST_ASSERT_EQUAL('\0', sb.buffer[0]); // Should be empty string after reset
    TEST_ASSERT_GREATER_OR_EQUAL(10, sb.capacity); // Capacity should remain
    const char *content2 = string_buffer_content_str(&sb);
    TEST_ASSERT_EQUAL_STRING("", content2);

    // Test append after reset
    string_buffer_append(&sb, "World");
    TEST_ASSERT_EQUAL_STRING("World", sb.buffer);
    TEST_ASSERT_EQUAL(5, sb.length);

    arena_destroy(&test_arena);
}

// Test freeing data
static void test_string_buffer_free_data_simple(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(test_arena.start);
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 16);
    string_buffer_append(&sb, "Some content");
    TEST_ASSERT_NOT_NULL(sb.buffer);

    string_buffer_reset(&sb);
    TEST_ASSERT_EQUAL(0, sb.length);
    TEST_ASSERT_GREATER_OR_EQUAL(16, sb.capacity);

    arena_destroy(&test_arena);
}

// Test freeing an already empty/freed buffer (should be safe)
static void test_string_buffer_free_data_empty(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(test_arena.start);
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 1);
    string_buffer_reset(&sb);
    TEST_ASSERT_EQUAL(0, sb.length);
    TEST_ASSERT_GREATER_OR_EQUAL(1, sb.capacity);
    arena_destroy(&test_arena); // Free again - should not crash
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
    RUN_TEST(test_string_buffer_content_access_and_reset);
    RUN_TEST(test_string_buffer_free_data_simple);
    RUN_TEST(test_string_buffer_free_data_empty);
}
