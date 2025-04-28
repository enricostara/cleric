#ifndef STRINGS_H
#define STRINGS_H

#include <stddef.h> // For size_t

// Generic dynamic string buffer
typedef struct {
    char *buffer; // Pointer to the allocated buffer
    size_t capacity; // Current allocated capacity
    size_t length; // Current length of the string (excluding null terminator)
} StringBuffer;

/**
 * Initializes a StringBuffer with an initial capacity.
 * Exits on allocation failure.
 * @param sb Pointer to the StringBuffer to initialize.
 * @param initial_capacity The initial size of the buffer.
 */
void string_buffer_init(StringBuffer *sb, size_t initial_capacity);

/**
 * Appends a formatted string to the StringBuffer, resizing if necessary.
 * Exits on reallocation failure.
 * @param sb Pointer to the StringBuffer.
 * @param format The format string (printf-style).
 * @param ... Variable arguments for the format string.
 */
void string_buffer_append(StringBuffer *sb, const char *format, ...);

/**
 * Appends a single character to the StringBuffer, resizing if necessary.
 * Exits on reallocation failure.
 * @param sb Pointer to the StringBuffer.
 * @param c The character to append.
 */
void string_buffer_append_char(StringBuffer *sb, char c);

// Get the content as a C string (read-only, pointer valid until next modification)
const char *string_buffer_content_str(const StringBuffer *sb);

// Get the internal buffer (transfers ownership - USE WITH CAUTION)
// Kept for specific use cases, but prefer string_buffer_c_str or copy
char *string_buffer_release_content(StringBuffer *sb);

/**
 * Frees the internal buffer associated with the StringBuffer.
 * Resets the struct members.
 * @param sb Pointer to the StringBuffer.
 */
void string_buffer_clear(StringBuffer *sb);

// Destroys the string buffer, freeing allocated memory.
void string_buffer_destroy(StringBuffer *sb);

#endif // STRINGS_H
