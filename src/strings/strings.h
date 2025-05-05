#ifndef STRINGS_H
#define STRINGS_H

#include "../memory/arena.h" // Include Arena for memory management

// Generic dynamic string buffer using Arena allocation
typedef struct {
    char *buffer; // Pointer to the allocated buffer within the arena
    size_t capacity; // Current allocated capacity
    size_t length; // Current length of the string (excluding null terminator)
    Arena *arena;  // Pointer to the arena used for allocations
} StringBuffer;

/**
 * Initializes a StringBuffer using the provided Arena.
 * @param sb Pointer to the StringBuffer to initialize.
 * @param arena The Arena to use for allocations.
 * @param initial_capacity The initial size of the buffer to allocate.
 */
void string_buffer_init(StringBuffer *sb, Arena *arena, size_t initial_capacity);

/**
 * Appends a formatted string to the StringBuffer, resizing if necessary using the arena.
 * @param sb Pointer to the StringBuffer.
 * @param format The format string (printf-style).
 * @param ... Variable arguments for the format string.
 */
void string_buffer_append(StringBuffer *sb, const char *format, ...);

/**
 * Appends a single character to the StringBuffer, resizing if necessary using the arena.
 * @param sb Pointer to the StringBuffer.
 * @param c The character to append.
 */
void string_buffer_append_char(StringBuffer *sb, char c);

// Get the content as a C string (read-only, pointer valid until next modification)
const char *string_buffer_content_str(const StringBuffer *sb);

/**
 * Resets the StringBuffer to be empty, allowing reuse of the allocated buffer.
 * Does not free memory (arena handles that).
 * @param sb Pointer to the StringBuffer.
 */
void string_buffer_reset(StringBuffer *sb);

#endif // STRINGS_H
