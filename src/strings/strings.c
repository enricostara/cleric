#include "strings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Ensure buffer has enough space for `additional_needed` characters (plus null terminator)
// Returns false on failure, true on success.
static bool ensure_capacity(StringBuffer *sb, size_t additional_needed) {
    size_t required_total = sb->length + additional_needed;
    if (required_total < sb->length) { // Check for overflow
        fprintf(stderr, "StringBuffer Error: Size calculation overflow.\n");
        return false;
    }

    size_t required_capacity = required_total + 1; // +1 for null terminator
    if (required_capacity <= sb->capacity) {
        return true; // Enough space already
    }

    // Calculate new capacity (e.g., double until sufficient)
    size_t new_capacity = sb->capacity > 0 ? sb->capacity : 1; // Start with 1 if capacity is 0
    while (new_capacity < required_capacity) {
        new_capacity *= 2;
        if (new_capacity < sb->capacity) { // Check for overflow during doubling
            fprintf(stderr, "StringBuffer Error: Capacity calculation overflow during resize.\n");
             // Try setting to maximum possible size if overflow occurs? Or just fail.
            new_capacity = (size_t)-1; // Max size_t value
            if (new_capacity < required_capacity) return false; // Still not enough
            break; // Use max capacity
        }
    }

    // Reallocate
    char *new_buffer = (char *)realloc(sb->buffer, new_capacity);
    if (!new_buffer) {
        perror("Failed to reallocate string buffer");
        // Consider freeing the old buffer sb->buffer here before exiting?
        // Depending on desired error handling.
        exit(EXIT_FAILURE); // Critical error
    }

    sb->buffer = new_buffer;
    sb->capacity = new_capacity;
    return true;
}


void string_buffer_init(StringBuffer *sb, size_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = 16; // Default small capacity
    }
    sb->buffer = (char *)malloc(initial_capacity);
    if (!sb->buffer) {
        perror("Failed to allocate string buffer");
        exit(EXIT_FAILURE); // Critical error
    }
    sb->capacity = initial_capacity;
    sb->length = 0;
    sb->buffer[0] = '\0'; // Start with an empty, null-terminated string
}

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

    size_t additional_needed = (size_t)required;

    // Ensure buffer has enough capacity
    if (!ensure_capacity(sb, additional_needed)) {
        va_end(args2);
        // Error already printed by ensure_capacity or realloc failed (exit)
        return;
    }

    // Append the formatted string
    int written = vsnprintf(sb->buffer + sb->length, sb->capacity - sb->length, format, args2);
    va_end(args2);

    if (written < 0 || (size_t)written != additional_needed) {
        fprintf(stderr, "StringBuffer Error: vsnprintf writing failed or wrote unexpected size.\n");
        // Handle potential buffer corruption or error state
    } else {
        sb->length += written;
        // Ensure null termination is maintained, though vsnprintf should do this.
        // sb->buffer[sb->length] = '\0'; // Usually not needed if ensure_capacity worked
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

char *string_buffer_get_content(StringBuffer *sb) {
    if (!sb) return NULL;
    char *content = sb->buffer;
    // Detach buffer from the struct to transfer ownership
    sb->buffer = NULL;
    sb->capacity = 0;
    sb->length = 0;
    // The caller is now responsible for freeing `content`
    return content;
}

void string_buffer_free_data(StringBuffer *sb) {
    if (sb && sb->buffer) {
        free(sb->buffer);
        sb->buffer = NULL;
        sb->capacity = 0;
        sb->length = 0;
    }
}
