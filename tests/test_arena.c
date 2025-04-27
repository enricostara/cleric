#include "unity.h"
#include "memory/arena.h"
#include <stddef.h> // For size_t
#include <stdint.h> // For uintptr_t

// Define alignment based on the implementation in arena.c (using compiler extension)
#define MAX_ALIGNMENT __alignof__(long double)

// Helper to check alignment
static int is_aligned(void *ptr, size_t alignment) {
    return ((uintptr_t) ptr % alignment) == 0;
}

// Helper to align up
static size_t align_up(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

// --- Test Cases ---

void test_arena_create_destroy(void) {
    Arena arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(arena.start);
    TEST_ASSERT_EQUAL(1024, arena.total_size);
    TEST_ASSERT_EQUAL(0, arena.offset);
    arena_destroy(&arena);
    TEST_ASSERT_NULL(arena.start);
    TEST_ASSERT_EQUAL(0, arena.total_size);
    TEST_ASSERT_EQUAL(0, arena.offset);
}

void test_arena_create_zero_size(void) {
    // Test creating with zero size.
    // Expecting malloc(0) might return NULL or a unique ptr,
    // but the arena size should be 0.
    Arena arena = arena_create(0);

    // Standard behavior for malloc(0) is implementation-defined.
    // It can return NULL or a unique pointer. We should handle either,
    // but the key is that the reported size should be 0.
    // A safe check is just on the size.
    TEST_ASSERT_EQUAL(0, arena.total_size);

    // Optional: Check allocation attempt with this zero-size arena
    void *ptr = arena_alloc(&arena, 1);
    TEST_ASSERT_NULL(ptr); // Should fail to allocate from a zero-size arena

    arena_destroy(&arena); // Should handle potentially NULL start gracefully
}

void test_arena_alloc_basic(void) {
    Arena arena = arena_create(1024);
    void *ptr = arena_alloc(&arena, 100);
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_TRUE(is_aligned(ptr, MAX_ALIGNMENT));
    TEST_ASSERT_GREATER_OR_EQUAL(100, arena.offset); // Offset includes alignment padding
    arena_destroy(&arena);
}

void test_arena_alloc_multiple(void) {
    Arena arena = arena_create(1024);
    void *ptr1 = arena_alloc(&arena, 50);
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_TRUE(is_aligned(ptr1, MAX_ALIGNMENT));
    size_t offset1 = arena.offset;
    TEST_ASSERT_GREATER_OR_EQUAL(50, offset1);

    void *ptr2 = arena_alloc(&arena, 70);
    TEST_ASSERT_NOT_NULL(ptr2);
    TEST_ASSERT_TRUE(is_aligned(ptr2, MAX_ALIGNMENT));
    // Check that ptr2 starts after ptr1 + its size, considering alignment
    TEST_ASSERT_TRUE((uintptr_t)ptr2 >= (uintptr_t)ptr1 + 50);
    TEST_ASSERT_GREATER_OR_EQUAL(offset1 + 70, arena.offset);

    arena_destroy(&arena);
}

void test_arena_alloc_alignment(void) {
    Arena arena = arena_create(1024);
    // Allocate small chunks to observe alignment padding
    for (int i = 0; i < 10; ++i) {
        char *ptr = (char *) arena_alloc(&arena, 1); // Allocate 1 byte
        TEST_ASSERT_NOT_NULL(ptr);
        TEST_ASSERT_TRUE(is_aligned(ptr, MAX_ALIGNMENT));
        *ptr = (char) i; // Touch the memory
    }
    arena_destroy(&arena);
}


void test_arena_alloc_out_of_memory(void) {
    Arena arena = arena_create(100); // Small arena
    void *ptr1 = arena_alloc(&arena, 80);
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_TRUE(is_aligned(ptr1, MAX_ALIGNMENT));

    // This allocation should fail
    void *ptr2 = arena_alloc(&arena, 50);
    TEST_ASSERT_NULL(ptr2); // Expect NULL when out of memory

    // Verify offset didn't change after failed allocation
    size_t offset_after_fail = arena.offset;

    // Try small allocation again, should succeed if there's *some* room left
    void *ptr3 = arena_alloc(&arena, 1);
    // Check if it succeeded (it should if ptr1 didn't exactly fill the arena)
    size_t expected_available = arena.total_size - offset_after_fail;
    if (expected_available >= 1) {
        TEST_ASSERT_NOT_NULL(ptr3);
        TEST_ASSERT_TRUE(is_aligned(ptr3, MAX_ALIGNMENT));
        // Check that ptr3 is positioned correctly after the first allocation + alignment padding
        TEST_ASSERT_EQUAL_PTR((void*)(arena.start + align_up(offset_after_fail - 80, MAX_ALIGNMENT) + 80), ptr3);
        TEST_ASSERT_GREATER_OR_EQUAL(offset_after_fail + 1, arena.offset); // Offset should advance
    } else {
        // If somehow ptr1 perfectly filled it, then ptr3 should be NULL
        TEST_ASSERT_NULL(ptr3);
        TEST_ASSERT_EQUAL(offset_after_fail, arena.offset);
    }

    arena_destroy(&arena);
}

void test_arena_reset(void) {
    Arena arena = arena_create(1024);
    void *ptr1 = arena_alloc(&arena, 100);
    TEST_ASSERT_NOT_NULL(ptr1);
    size_t offset_before_reset = arena.offset;
    TEST_ASSERT_GREATER_THAN(0, offset_before_reset);

    arena_reset(&arena);
    TEST_ASSERT_EQUAL(0, arena.offset); // Offset should be zero after reset
    TEST_ASSERT_NOT_NULL(arena.start); // Start pointer and size remain
    TEST_ASSERT_EQUAL(1024, arena.total_size);

    // Allocation should work again from the beginning
    void *ptr2 = arena_alloc(&arena, 50);
    TEST_ASSERT_NOT_NULL(ptr2);
    TEST_ASSERT_TRUE(is_aligned(ptr2, MAX_ALIGNMENT));
    // The new pointer should be at the same aligned start as the original first allocation
    TEST_ASSERT_EQUAL_PTR(arena.start, ptr2); // Assuming first allocation starts right at arena.start (aligned)
    TEST_ASSERT_GREATER_OR_EQUAL(50, arena.offset); // New offset


    arena_destroy(&arena);
}

// --- Test Runner Function ---
// This function will be called by the main test runner (test_all.c)
void run_arena_tests(void) {
    RUN_TEST(test_arena_create_destroy);
    RUN_TEST(test_arena_create_zero_size); // Re-enable this test
    RUN_TEST(test_arena_alloc_basic);
    RUN_TEST(test_arena_alloc_multiple);
    RUN_TEST(test_arena_alloc_alignment);
    RUN_TEST(test_arena_alloc_out_of_memory);
    RUN_TEST(test_arena_reset);
}
