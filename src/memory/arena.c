#include "arena.h"
#include <stdlib.h> // For malloc, free
#include <string.h> // For NULL
#include <stdio.h> // For fprintf, stderr
#include <stddef.h> // For size_t

// Define alignment using compiler extension __alignof__ for C99 compatibility
// Assuming long double has the strictest alignment requirement
#define MAX_ALIGNMENT __alignof__(long double)

// Helper function to align memory addresses
// ReSharper disable once CppDFAConstantParameter
static size_t align_up(size_t size, const size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

static void arena_memset(void *ptr, int c, size_t n);

Arena arena_create(const size_t initial_size) {
    Arena arena = {0}; // Initialize all fields to zero/NULL
    arena.start = (char *) malloc(initial_size);
    if (arena.start == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for arena (size %zu)\n", initial_size);
        arena.total_size = 0;
    } else {
        arena.total_size = initial_size;
    }
    arena.offset = 0;
    return arena;
}

void *arena_alloc(Arena *arena, const size_t size) {
    if (arena == NULL || arena->start == NULL) {
        fprintf(stderr, "Error: Attempt to allocate from uninitialized or failed arena.\n");
        return NULL;
    }

    // Calculate the next aligned offset
    const size_t current_aligned_offset = align_up(arena->offset, MAX_ALIGNMENT);

    // Check if there's enough space
    if (current_aligned_offset + size > arena->total_size) {
        // For now, we fail. Could implement resizing later.
        fprintf(stderr, "Error: Arena out of memory (requested %zu, available %zu)\n",
                size, arena->total_size - current_aligned_offset);
        return NULL;
    }

    // Allocation succeeds: return pointer and update offset
    void *ptr = arena->start + current_aligned_offset;
    arena->offset = current_aligned_offset + size;
    return ptr;
}

void arena_destroy(Arena *arena) {
    if (arena && arena->start) {
        free(arena->start);
        arena->start = NULL;
        arena->total_size = 0;
        arena->offset = 0;
    }
}

void arena_reset(Arena *arena) {
    if (arena) {
        arena->offset = 0;
        // Zero out the memory for security/debugging
        arena_memset(arena->start, 0, arena->total_size);
    }
}

// Custom memset implementation.
// Uses a volatile pointer to try and ensure that the memory zeroing operation
// is not optimized away by the compiler. This is a common technique for
// attempting to securely clear sensitive data, as standard memset might be
// removed if the compiler deems the buffer unused afterwards.
// See SEI CERT C Coding Standard: MSC06-C.
static void arena_memset(void *ptr, int c, size_t n) {
    volatile unsigned char *p = ptr;
    while (n--) {
        *p++ = c;
    }
}
