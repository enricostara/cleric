#include "strings.h"
#include "../memory/arena.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

/**
 * Ensures that the StringBuffer has enough capacity to fit an additional
 * `additional_needed` bytes of content, including the null terminator.
 * If the buffer is too small, it is reallocated to a new capacity that is
 * at least as large as the required capacity.
 * If reallocation fails, the function prints an error message and exits.
 * @param sb The StringBuffer to ensure has enough capacity, using its arena.
 * @param additional_needed The number of additional bytes to ensure space for.
 * @return true if the buffer had enough capacity or was successfully resized,
 *         false if the buffer was too small and reallocation failed.
 */
static bool ensure_capacity(StringBuffer *sb, const size_t additional_needed) {
    const size_t required_total = sb->length + additional_needed;
    if (required_total < sb->length) {
        // Check for overflow
        fprintf(stderr, "StringBuffer Error: Size calculation overflow.\n");
        return false;
    }

    const size_t required_capacity = required_total + 1; // +1 for null terminator
    if (required_capacity <= sb->capacity) {
        return true; // Enough space already
    }

    // Calculate new capacity (e.g., double until sufficient)
    size_t new_capacity = sb->capacity > 0 ? sb->capacity : 1; // Start with 1 if capacity is 0
    while (new_capacity < required_capacity) {
        new_capacity *= 2;
        if (new_capacity < sb->capacity) {
            // Check for overflow during doubling
            fprintf(stderr, "StringBuffer Error: Capacity calculation overflow during resize.\n");
            // Try setting to maximum possible size if overflow occurs? Or just fail.
            new_capacity = (size_t) -1; // Max size_t value
            if (new_capacity < required_capacity) return false; // Still not enough
            break; // Use max capacity
        }
    }

    // Reallocate
    // Use arena_alloc; resizing means allocating new block and copying
    char *new_buffer = arena_alloc(sb->arena, new_capacity);
    if (!new_buffer) {
        fprintf(stderr, "Arena allocation failed for string buffer resize\n");
        return false;
    }

    // Copy old content to new buffer if there was old content
    if (sb->buffer && sb->length > 0) {
        memcpy(new_buffer, sb->buffer, sb->length);
        // Old buffer remains in the arena, but sb->buffer now points to the new one.
    }

    sb->buffer = new_buffer;
    sb->capacity = new_capacity;
    // Ensure null-termination after potential copy
    sb->buffer[sb->length] = '\0';
    return true;
}


void string_buffer_init(StringBuffer *sb, Arena *arena, size_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = 16; // Default small capacity
    }
    // Allocate initial buffer from the arena
    sb->arena = arena;
    sb->buffer = (char *) arena_alloc(sb->arena, initial_capacity);
    if (!sb->buffer) {
        fprintf(stderr, "Arena allocation failed for string buffer init\n");
        exit(EXIT_FAILURE); // Critical error
    }
    sb->capacity = initial_capacity;
    sb->length = 0;
    sb->buffer[0] = '\0'; // Start with an empty, null-terminated string
}

/**
 * Appends a formatted string to a StringBuffer.
 *
 * This function appends a formatted string to the given StringBuffer. The
 * string is formatted according to the given format string and variable
 * arguments.
 *
 * The function first calculates the size of the new content using vsnprintf
 * and then checks if the buffer has enough capacity. If the buffer has
 * enough capacity, the function appends the formatted string using vsnprintf
 * again. If the buffer does not have enough capacity, the function doubles
 * its capacity until it has enough capacity and then appends the formatted
 * string.
 *
 * If the formatted string is too large to fit in the buffer, the function
 * prints an error message to stderr and does not append the string.
 *
 * @param sb The StringBuffer to append to.
 * @param format The format string for the new content.
 * @param ... The variable arguments for the format string.
 */
void string_buffer_append(StringBuffer *sb, const char *format, ...) {
    va_list args1, args2;
    va_start(args1, format);
    va_copy(args2, args1);

    // Calculate required size for the new content
    int required = vsnprintf(NULL, 0, format, args1);
    va_end(args1);
    if (required < 0) {
        fprintf(stderr, "StringBuffer Error: vsnprintf calculation failed.\n");
        va_end(args2);
        return; // Or handle error more gracefully
    }

    size_t additional_needed = (size_t) required;

    // Ensure buffer has enough capacity
    if (!ensure_capacity(sb, additional_needed)) {
        va_end(args2);
        // Error already printed by ensure_capacity or realloc failed (exit)
        return;
    }

    // Append the formatted string to the buffer
    // We write to sb->buffer + sb->length because we want to append to the existing content
    // We write at most sb->capacity - sb->length bytes to ensure we don't overflow the buffer
    int written = vsnprintf(sb->buffer + sb->length, sb->capacity - sb->length, format, args2);
    va_end(args2);

    if (written < 0 || (size_t) written != additional_needed) {
        fprintf(stderr, "StringBuffer Error: vsnprintf writing failed or wrote unexpected size.\n");
        // Handle potential buffer corruption or error state
    } else {
        sb->length += written;
        // Ensure null termination is maintained, though vsnprintf should do this.
        sb->buffer[sb->length] = '\0'; // Usually not needed if ensure_capacity worked
    }
}

void string_buffer_append_char(StringBuffer *sb, char c) {
    if (!ensure_capacity(sb, 1)) {
        // Error handled by ensure_capacity (exit or return)
        return;
    }
    sb->buffer[sb->length] = c;
    sb->length++;
    sb->buffer[sb->length] = '\0'; // Maintain null termination
}

// --- Get Content ---

// Returns a read-only pointer to the internal buffer.
// Pointer is valid until the buffer is modified or destroyed.
const char *string_buffer_content_str(const StringBuffer *sb) {
    if (!sb || !sb->buffer) {
        // Return an empty string literal if buffer is NULL or sb is NULL
        // or if length is 0 (buffer might be allocated but empty)
        return "";
    }
    // Buffer should already be null-terminated by append functions
    return sb->buffer;
}

// --- Reset ---
void string_buffer_reset(StringBuffer *sb) {
    if (sb && sb->buffer) {
        // Reset length, keep buffer and capacity, ready for reuse
        sb->length = 0;
        sb->buffer[0] = '\0'; // Ensure it's an empty string
    }
}

// --- String Duplication ---
char* arena_strdup(Arena *arena, const char *s) {
    if (!s) return NULL;
    if (!arena) return NULL; // Cannot allocate without an arena

    size_t len = strlen(s);
    char *new_str = (char *)arena_alloc(arena, len + 1);
    if (!new_str) return NULL; // Allocation failed
    
    memcpy(new_str, s, len + 1); // Copy including the null terminator
    return new_str;
}
