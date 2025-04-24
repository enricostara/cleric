#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "driver.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h" // Include parser header
#include "../parser/ast.h"    // Include AST header (for printing/freeing)
#include "../files/files.h"

/**
 * Runs the gcc preprocessor on the input file and writes output to .i file.
 * Returns 0 on success, 1 on failure.
 */
int run_preprocessor(const char *input_file) {
    // Use utility to check extension
    if (!filename_has_ext(input_file, ".c")) {
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
int run_compiler(const char *input_file, const bool lex_only, const bool parse_only) {
    // Use utility to check extension
    if (!filename_has_ext(input_file, ".i")) {
        fprintf(stderr, "Input file should have a .i extension\n");
        return 1;
    }

    char output_file[1024];
    if (!filename_replace_ext(input_file, ".s", output_file, sizeof(output_file))) {
        fprintf(stderr, "Failed to construct .s filename for %s\n", input_file);
        return 1;
    }

    // Read the input file content once
    long f_size = 0;
    char *src = read_entire_file(input_file, &f_size);
    if (!src) {
        fprintf(stderr, "Failed to open or read %s\n", input_file);
        return 1;
    }

    // --- Lexing Phase --- Always lex to check for errors
    Lexer lexer_check; // Use a separate lexer instance for the initial check
    lexer_init(&lexer_check, src);
    Token tok;
    int error = 0;
    while ((tok = lexer_next_token(&lexer_check)).type != TOKEN_EOF) {
        if (tok.type == TOKEN_UNKNOWN) {
            // Use token_to_string for better error message
            char token_str[128];
            token_to_string(tok, token_str, sizeof(token_str));
            fprintf(stderr, "Lexical error: unknown token %s at position %zu\n", token_str, tok.position);
            token_free(&tok);
            error = 1;
            break;
        }
        if (lex_only) {
            // Use token_to_string for consistent output
            char token_str[128];
            // Need to re-lex for printing if lex_only is true
            // This is slightly inefficient but keeps logic simple for now.
            // A better way might be to store tokens first.
            token_to_string(tok, token_str, sizeof(token_str));
            printf("%s, pos=%zu\n", token_str, tok.position);
        }
        token_free(&tok);
    }
    if (error) {
        free(src); // Free source code on error
        return 1;
    }
    if (lex_only) {
        free(src); // Free source code when done
        return 0;
    }

    // --- Parsing Phase --- (Only if not lex_only)
    // We need a new lexer instance because the previous one consumed the tokens
    Lexer lexer_parse;
    lexer_init(&lexer_parse, src); // Initialize with the same source

    Parser parser;
    parser_init(&parser, &lexer_parse);

    printf("Parsing...\n");
    ProgramNode *ast_root = parse_program(&parser); // Parse the program

    free(src); // Free source code

    if (parser.error_flag || !ast_root) {
        fprintf(stderr, "Parsing failed.\n");
        free_ast((AstNode *) ast_root); // Attempt to free potentially partial tree
        parser_destroy(&parser);
        return 1;
    }

    printf("Parsing successful. AST:\n");
    ast_pretty_print((AstNode *) ast_root, 0); // Pretty print the AST

    free_ast((AstNode *) ast_root); // Free the AST
    parser_destroy(&parser); // Clean up parser resources (if any)

    if (parse_only) {
        // If parse_only, we stop here successfully.
        return 0;
    }

    // --- Code Generation Phase --- (Only if not lex_only and not parse_only)
    printf("Proceeding to mock code generation...\n");
    // Write the mock assembly output
    // Use utility to write file
    if (!write_string_to_file(output_file, mock_asm_payload)) {
        fprintf(stderr, "Failed to write assembly to %s\n", output_file);
        // Attempt to remove the potentially incomplete output file
        remove(output_file);
        return 1;
    }
    // Remove the .i file if all is fine
    if (remove(input_file) != 0) {
        fprintf(stderr, "Warning: could not remove %s\n", input_file);
    }
    printf("Mock compilation output written to %s\n", output_file);
    return 0;
}

// Final step: assemble and link .s to executable, then remove .s if successful
int run_assembler_linker(const char *input_file) {
    // Use utility to check extension
    if (!filename_has_ext(input_file, ".s")) {
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
