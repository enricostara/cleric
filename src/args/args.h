#ifndef ARGS_H
#define ARGS_H

#include <stdbool.h>

/**
 * @brief Parses command-line arguments.
 * @param argc The argument count.
 * @param argv The argument vector.
 * @param lex_only Pointer to a boolean flag set to true if --lex is present.
 * @param parse_only Pointer to a boolean flag set to true if --parse is present.
 * @param tac_only Pointer to a boolean flag set to true if --tac (or --irgen-only, --tacky) is present.
 * @param codegen_only Pointer to a boolean flag set to true if --codegen is present.
 * @return The input filename if arguments are valid, otherwise NULL.
 */
const char *parse_args(int argc, char *argv[], bool *lex_only, bool *parse_only, bool *tac_only, bool *codegen_only);

#endif // ARGS_H
