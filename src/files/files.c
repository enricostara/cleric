#include "files.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char* filename_replace_ext(const char* input_file, const char* new_ext, char* dest, size_t dest_size) {
    if (!input_file || !new_ext || !dest || dest_size == 0) return NULL;
    size_t len = strlen(input_file);
    // Find last '.' in input_file
    const char* dot = strrchr(input_file, '.');
    size_t base_len = dot ? (size_t)(dot - input_file) : len;
    size_t ext_len = strlen(new_ext);
    if (base_len + ext_len + 1 > dest_size) return NULL; // +1 for null terminator
    memcpy(dest, input_file, base_len);
    memcpy(dest + base_len, new_ext, ext_len);
    dest[base_len + ext_len] = '\0';
    return dest;
}

char *read_entire_file(const char *filename, long *out_file_size) {
    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return NULL; }
    long size = ftell(file);
    if (size < 0) { fclose(file); return NULL; }
    rewind(file);
    char *data = malloc(size + 1);
    if (!data) { fclose(file); return NULL; }
    if (fread(data, 1, size, file) != (size_t)size) { free(data); fclose(file); return NULL; }
    data[size] = '\0';
    fclose(file);
    if (out_file_size) *out_file_size = size;
    return data;
}
