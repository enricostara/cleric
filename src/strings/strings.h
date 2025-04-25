#ifndef STRINGS_H
#define STRINGS_H

#include <stddef.h> // For size_t
#include <stdbool.h> // For bool

// Generic dynamic string buffer
typedef struct {
    char *buffer;    // Pointer to the allocated buffer
    size_t capacity; // Current allocated capacity
    size_t length;   // Current length of the string (excluding null terminator)
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

/**
 * Returns the final string content and transfers ownership to the caller.
 * The StringBuffer itself should be freed separately if needed, but the
 * internal buffer pointer is detached.
 * Ensures the returned string is null-terminated.
 * @param sb Pointer to the StringBuffer.
 * @return A pointer to the dynamically allocated string buffer. The caller is responsible for freeing this.
 */
char *string_buffer_get_content(StringBuffer *sb);


/**
 * Frees the internal buffer associated with the StringBuffer.
 * Resets the struct members.
 * @param sb Pointer to the StringBuffer.
 */
void string_buffer_clear(StringBuffer *sb);


#endif // STRINGS_H
