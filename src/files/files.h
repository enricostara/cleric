#ifndef FILE_H
#define FILE_H
#include <stddef.h>

char *filename_replace_ext(const char *input_file, const char *new_ext, char *dest, size_t dest_size);

char *read_entire_file(const char *filename, long *out_file_size);

#endif // FILE_H
