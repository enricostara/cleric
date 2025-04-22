#include "files.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

char *filename_replace_ext(const char *input_file, const char *new_ext, char *dest, size_t dest_size) {
    // Calculate the length of the input filename
    const size_t len = strlen(input_file);

    // Find the last '.' in the input filename
    const char *dot = strrchr(input_file, '.');

    // Calculate the length of the base name (part before the last dot, or full length if no dot)
    size_t base_len = dot ? (size_t) (dot - input_file) : len;

    // Calculate the length of the new extension
    size_t ext_len = strlen(new_ext);

    // Check if the resulting string fits in the destination buffer
    if (base_len + ext_len + 1 > dest_size) return NULL; // +1 for null terminator

    // Copy the base name to the destination buffer
    memcpy(dest, input_file, base_len);

    // Copy the new extension to the destination buffer
    memcpy(dest + base_len, new_ext, ext_len);

    // Null-terminate the result
    dest[base_len + ext_len] = '\0';

    return dest;
}

char *read_entire_file(const char *filename, long *out_file_size) {
    // Open the file in binary read mode
    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;

    // Seek to the end to determine file size
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    // Get the current position (which is the size)
    const long size = ftell(file);
    if (size < 0) { // Handle ftell error
        fclose(file);
        return NULL;
    }

    // Go back to the beginning of the file
    rewind(file);

    // Allocate buffer (size + 1 for null terminator)
    char *data = malloc(size + 1);
    if (!data) {
        fclose(file);
        return NULL;
    }

    // Read the entire file into the buffer
    if (fread(data, 1, size, file) != (size_t) size) {
        // If read failed, clean up and return NULL
        free(data);
        fclose(file);
        return NULL;
    }

    // Null-terminate the buffer
    data[size] = '\0';

    // Close the file
    fclose(file);

    // Store the size if requested
    if (out_file_size) *out_file_size = size;

    return data;
}


bool filename_has_ext(const char *filename, const char *ext) {
    // Guard against NULL pointers
    if (!filename || !ext) {
        return false;
    }

    // Calculate the length of the filename
    const size_t filename_len = strlen(filename);

    // Calculate the length of the extension
    const size_t ext_len = strlen(ext);

    // Extension cannot be empty or longer than/equal to the filename itself
    if (ext_len == 0 || ext_len >= filename_len) {
        return false;
    }

    // Compare the suffix of the filename with the extension
    return strcmp(filename + filename_len - ext_len, ext) == 0;
}


bool write_string_to_file(const char *filename, const char *content) {
    // Must provide valid filename and content
    if (!filename || !content) {
        return false;
    }

    // Open the file in write mode ("w"), which truncates or creates
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open file for writing");
        fprintf(stderr, "Filename: %s\n", filename);
        return false;
    }

    // Write the content string to the file
    if (fputs(content, file) == EOF) {
        perror("Failed to write content to file");
        fprintf(stderr, "Filename: %s\n", filename);
        fclose(file);
        return false;
    }

    // Close the file
    fclose(file);

    return true;
}
