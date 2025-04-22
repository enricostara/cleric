#ifndef FILES_H
#define FILES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Replaces the extension of a filename.
// Returns dest if successful, NULL otherwise.
char *filename_replace_ext(const char *input_file, const char *new_ext, char *dest, size_t dest_size);

// Reads the entire content of a file into a dynamically allocated buffer.
// Returns the buffer or NULL on error. Sets out_file_size if not NULL.
char *read_entire_file(const char *filename, long *out_file_size);

// Checks if a filename ends with the specified extension (case-sensitive).
// The extension should include the dot (e.g., ".c").
bool filename_has_ext(const char *filename, const char *ext);

// Writes a given string to a file, overwriting it if it exists.
// Returns true on success, false on error.
bool write_string_to_file(const char *filename, const char *content);

#endif // FILES_H
