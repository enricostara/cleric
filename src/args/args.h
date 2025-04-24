#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

// Parses CLI arguments. Sets flags and returns input_file or NULL on error.
const char *parse_args(int argc, char *argv[], bool *lex_only, bool *parse_only);

#endif // ARGS_H
