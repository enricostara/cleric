#ifndef CLERIC_ARENA_H
#define CLERIC_ARENA_H

#include <stddef.h> // For size_t
#include <stdbool.h>

// Basic Arena Allocator
// - Allocates a large block of memory initially.
// - Allocations bump a pointer within the block.
// - No individual free; the entire arena is freed at once.
typedef struct {
    char *start;    // Pointer to the beginning of the allocated memory block
    size_t total_size; // Total size of the block
    size_t offset;     // Current allocation offset from the start
    // We could add resizing logic later if needed, but keep it simple for now.
} Arena;

/**
 * @brief Creates and initializes a new memory arena.
 * @param initial_size The initial capacity of the arena in bytes.
 * @return An initialized Arena struct. Check arena.start != NULL for success.
 */
Arena arena_create(size_t initial_size);

/**
 * @brief Allocates a block of memory within the arena.
 *        Performs basic alignment to MAX_ALIGNMENT.
 * @param arena Pointer to the arena.
 * @param size The number of bytes to allocate.
 * @return Pointer to the allocated memory, or NULL if the arena is full.
 */
void* arena_alloc(Arena *arena, size_t size);

/**
 * @brief Destroys the arena, freeing all its allocated memory.
 * @param arena Pointer to the arena to destroy.
 */
void arena_destroy(Arena *arena);

/**
 * @brief Resets the arena's offset, allowing reuse of its memory without freeing/reallocating.
 * @param arena Pointer to the arena to reset.
 */
void arena_reset(Arena *arena);


#endif //CLERIC_ARENA_H
