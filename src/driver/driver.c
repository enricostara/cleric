#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "driver.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h" // Include parser header
#include "../parser/ast.h"    // Include AST header (for printing/freeing)
#include "../codegen/codegen.h" // Include Codegen header
#include "../strings/strings.h" // Include StringBuffer header
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

// New function: compilation from .i to .s
int run_compiler(const char *input_file, const bool lex_only, const bool parse_only, const bool codegen_only) {
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

    // --- Lexing Phase --- Use a single lexer instance
    Lexer lexer;
    lexer_init(&lexer, src);
    Token tok;
    int error = 0;
    while ((tok = lexer_next_token(&lexer)).type != TOKEN_EOF) {
        if (tok.type == TOKEN_UNKNOWN) {
            // Use token_to_string for better error message
            char token_str[128];
            token_to_string(tok, token_str, sizeof(token_str));
            fprintf(stderr, "Lexical error: unknown token %s at position %zu\n", token_str, tok.position);
            token_free(&tok);
            error = 1;
            break;
        }
        if (lex_only || parse_only || codegen_only) {
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

    // ReSharper disable once CppDFAUnusedValue
    int result = 1; // Default to error

    if (lex_only) {
        result = 0; // Lexing succeeded
        goto cleanup_source; // Skip parsing and codegen if lex_only
    }

    // --- Reset Lexer for Parsing --- (Only if not lex_only)
    lexer_reset(&lexer); // Reset position to the beginning

    // --- Parsing Phase --- (Only if not lex_only)
    Parser parser;
    parser_init(&parser, &lexer);

    printf("Parsing...\n");
    ProgramNode *ast_root = parse_program(&parser); // Parse the program

    if (parser.error_flag || !ast_root) {
        fprintf(stderr, "Parsing failed.\n");
        result = 1;
        goto cleanup_parser; // Go to clean up
    }

    printf("Parsing successful. AST:\n");
    if (parse_only || codegen_only) {
        ast_pretty_print((AstNode *) ast_root, 0); // Pretty print the AST
    }

    if (parse_only) {
        result = 0; // Success
        goto cleanup_parser; // Skip codegen
    }

    // --- Code Generation Phase --- (Only if not lex_only and not parse_only)
    printf("Generating code...\n");

    StringBuffer sb;
    string_buffer_init(&sb, 1024); // Initialize string buffer

    // Call the actual code generator
    if (!codegen_generate_program(&sb, ast_root)) {
        fprintf(stderr, "Code generation failed.\n");
        string_buffer_destroy(&sb); // Clean up buffer
        result = 1;
        goto cleanup_parser; // Go to cleanup
    }

    // If codegen_only, we stop here successfully after generating code internally.
    // Print the generated code to stdout for inspection.
    if (codegen_only) {
        printf("Codegen successful. Outputting assembly to stdout:\n");
        printf("--- Generated Assembly (stdout) ---\n");
        const char *assembly_code = string_buffer_get_content(&sb);
        printf("%s\n", assembly_code); // Print assembly to stdout
        printf("------------------------------------\n");
        string_buffer_destroy(&sb); // Clean up buffer
        result = 0; // Success
        goto cleanup_parser;
    }

    // --- Write Assembly to File --- (Only if not any "_only" mode)
    printf("Writing assembly code to %s...\n", output_file);
    const char *assembly_code = string_buffer_get_content(&sb);
    if (!write_string_to_file(output_file, assembly_code)) {
        fprintf(stderr, "Failed to write assembly to %s\n", output_file);
        remove(output_file); // Attempt removal
        string_buffer_destroy(&sb);
        result = 1;
        goto cleanup_parser;
    }
    printf("Assembly code written to %s\n", output_file);
    string_buffer_destroy(&sb); // Clean up buffer

    // Remove the intermediate .i file if code generation was successful,
    // and we actually wrote the .s file (i.e., not codegen_only)
    if (remove(input_file) != 0) {
        fprintf(stderr, "Warning: could not remove intermediate file %s\n", input_file);
    }

    result = 0; // Success!

cleanup_parser:
    // Free the AST *after* code generation is done or if skipping codegen
    free_ast((AstNode *) ast_root);
    parser_destroy(&parser); // Clean up parser resources

cleanup_source:
    free(src); // Free source code buffer

    return result;
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
