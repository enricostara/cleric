#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "driver.h"
#include "../lexer/lexer.h"
#include "../files/files.h"

/**
 * Runs the gcc preprocessor on the input file and writes output to .i file.
 * Returns 0 on success, 1 on failure.
 */
int run_preprocessor(const char *input_file) {
    const size_t len = strlen(input_file);
    if (len < 3 || strcmp(input_file + len - 2, ".c") != 0) {
        fprintf(stderr, "Input file should have a .c extension\n");
        return 1;
    }

    char output_file[1024];
    if (!filename_replace_ext(input_file, ".i", output_file, sizeof(output_file))) {
        fprintf(stderr, "Failed to construct .i filename for %s\n", input_file);
        return 1;
    }

    char command[2048];
    snprintf(command, sizeof(command), "gcc -E -P %s -o %s", input_file, output_file);
    const int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Failed to preprocess %s\n", input_file);
        return 1;
    }
    printf("Preprocessed output written to %s\n", output_file);
    return 0;
}

// Mock assembly payload from return2.s
static const char *mock_asm_payload = ".globl\t_main\n_main:\n\tmovl\t$2, %eax\n\tretq\n";

// New function: mock compilation from .i to .s
int run_compiler(const char *input_file, bool lex_only) {
    const size_t len = strlen(input_file);
    if (len < 3 || strcmp(input_file + len - 2, ".i") != 0) {
        fprintf(stderr, "Input file should have a .i extension\n");
        return 1;
    }

    char output_file[1024];
    if (!filename_replace_ext(input_file, ".s", output_file, sizeof(output_file))) {
        fprintf(stderr, "Failed to construct .s filename for %s\n", input_file);
        return 1;
    }

    // Use utility to read input file
    long fsize = 0;
    char *src = read_entire_file(input_file, &fsize);
    if (!src) {
        fprintf(stderr, "Failed to open or read %s\n", input_file);
        return 1;
    }
    // Lex the input
    Lexer lexer;
    lexer_init(&lexer, src);
    Token tok;
    int error = 0;
    while ((tok = lexer_next_token(&lexer)).type != TOKEN_EOF) {
        if (tok.type == TOKEN_UNKNOWN) {
            fprintf(stderr, "Lexical error: unknown token '%s' at position %zu\n", tok.lexeme ? tok.lexeme : "?", tok.position);
            token_free(&tok);
            error = 1;
            break;
        }
        if (lex_only) {
            printf("Token: type=%d", tok.type);
            if (tok.lexeme) printf(", lexeme='%s'", tok.lexeme);
            printf(", pos=%zu\n", tok.position);
        }
        token_free(&tok);
    }
    free(src);
    if (error) {
        return 1;
    }
    if (lex_only) {
        return 0;
    }
    // Write the mock assembly output
    FILE *f = fopen(output_file, "w");
    if (!f) {
        fprintf(stderr, "Failed to write to %s\n", output_file);
        return 1;
    }
    if (fputs(mock_asm_payload, f) == EOF) {
        fclose(f);
        fprintf(stderr, "Failed to write payload to %s\n", output_file);
        return 1;
    }
    fclose(f);
    // Remove the .i file if all is fine
    if (remove(input_file) != 0) {
        fprintf(stderr, "Warning: could not remove %s\n", input_file);
    }
    printf("Mock compilation output written to %s\n", output_file);
    return 0;
}

// Final step: assemble and link .s to executable, then remove .s if successful
int run_assembler_linker(const char *input_file) {
    const size_t len = strlen(input_file);
    if (len < 3 || strcmp(input_file + len - 2, ".s") != 0) {
        fprintf(stderr, "Input file should have a .s extension\n");
        return 1;
    }

    char output_file[1024];
    // Use empty string to remove the extension
    if (!filename_replace_ext(input_file, "", output_file, sizeof(output_file))) {
        fprintf(stderr, "Failed to construct output executable filename for %s\n", input_file);
        return 1;
    }

    char command[2048];
    snprintf(command, sizeof(command), "gcc %s -o %s", input_file, output_file);
    const int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Failed to assemble/link %s\n", input_file);
        return 1;
    }
    // Remove the .s file if all is fine
    if (remove(input_file) != 0) {
        fprintf(stderr, "Warning: could not remove %s\n", input_file);
    }
    printf("Assembled and linked output: %s\n", output_file);
    return 0;
}
