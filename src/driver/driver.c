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

// Forward declaration for the core compilation function
static bool run_compiler_core(const char *source_code,
                              bool lex_only,
                              bool parse_only,
                              bool codegen_only,
                              StringBuffer *output_assembly_sb,
                              AstNode **out_ast_root);

// Function: compilation from .i file to .s file (using the core compiler logic)
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

    long f_size = 0;
    char *src = read_entire_file(input_file, &f_size);
    if (!src) {
        fprintf(stderr, "Failed to open or read %s\n", input_file);
        return 1;
    }

    StringBuffer sb;
    string_buffer_init(&sb, 1024); // Initialize buffer for core function output
    AstNode *ast_root = NULL; // To receive the AST root for freeing
    int result = 1; // Default to error

    bool core_success = run_compiler_core(src, lex_only, parse_only, codegen_only, &sb, &ast_root);

    if (core_success) {
        // If only lexing, parsing, or codegen-to-stdout was requested, we are done successfully.
        if (lex_only || parse_only || codegen_only) {
            result = 0;
        } else {
            // Full compilation succeeded, write assembly to file
            printf("Writing assembly code to %s...\n", output_file);
            const char *assembly_code = string_buffer_get_content(&sb);
            if (!write_string_to_file(output_file, assembly_code)) {
                fprintf(stderr, "Failed to write assembly to %s\n", output_file);
                remove(output_file); // Attempt removal
                result = 1; // Mark as failure
            } else {
                printf("Assembly code written to %s\n", output_file);
                // Remove intermediate .i file only on full success
                if (remove(input_file) != 0) {
                    fprintf(stderr, "Warning: could not remove intermediate file %s\n", input_file);
                }
                result = 0; // Mark as success
            }
        }
    } else {
        // Core compilation failed, error messages already printed by core function
        // Make sure output file is removed if it exists from a previous run
        remove(output_file);
        result = 1;
    }

    // --- Cleanup ---
    string_buffer_destroy(&sb); // Always destroy the buffer
    free_ast(ast_root);         // Always free the AST (safe if ast_root is NULL)
    free(src);                  // Always free the source code

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

// -----------------------------------------------------------------------------
// Core Compilation Logic (Source String -> Assembly String Buffer)
// -----------------------------------------------------------------------------
static bool run_compiler_core(const char *source_code,
                              bool lex_only,
                              bool parse_only,
                              bool codegen_only,
                              StringBuffer *output_assembly_sb, // Output buffer for assembly
                              AstNode **out_ast_root) {         // Output AST root for freeing later

    bool success = false; // Default to failure
    *out_ast_root = NULL; // Initialize output AST pointer

    // --- Lexing Phase ---
    Lexer lexer;
    lexer_init(&lexer, source_code);
    Token tok;
    int error = 0;
    while ((tok = lexer_next_token(&lexer)).type != TOKEN_EOF) {
        if (tok.type == TOKEN_UNKNOWN) {
            char token_str[128];
            token_to_string(tok, token_str, sizeof(token_str));
            fprintf(stderr, "Lexical error: unknown token %s at position %zu\n", token_str, tok.position);
            token_free(&tok);
            error = 1;
            break;
        }
        if (lex_only || parse_only || codegen_only) {
            char token_str[128];
            token_to_string(tok, token_str, sizeof(token_str));
            printf("%s, pos=%zu\n", token_str, tok.position);
        }
        token_free(&tok);
    }
    if (error) {
        return false; // Lexical error
    }

    if (lex_only) {
        return true; // Lexing succeeded, stop here
    }

    // --- Reset Lexer for Parsing ---
    lexer_reset(&lexer);

    // --- Parsing Phase ---
    Parser parser;
    parser_init(&parser, &lexer);

    printf("Parsing...\n");
    ProgramNode *ast_root_local = parse_program(&parser);
    *out_ast_root = (AstNode *) ast_root_local; // Store AST root for caller cleanup

    if (parser.error_flag || !ast_root_local) {
        fprintf(stderr, "Parsing failed.\n");
        // AST might be partially created, caller MUST free *out_ast_root
        parser_destroy(&parser); // Clean up parser resources before returning
        return false; // Parsing failed
    }

    printf("Parsing successful. AST:\n");
    if (parse_only || codegen_only) {
        ast_pretty_print((AstNode *) ast_root_local, 0); // Pretty print the AST
    }

    // Store parser state before potential early return
    bool parse_error = parser.error_flag;
    parser_destroy(&parser); // Clean up parser resources now

    if (parse_error) { // Check error flag after destroy to ensure cleanup
         // AST is potentially incomplete, caller must free *out_ast_root
        return false;
    }

    if (parse_only) {
        // AST is complete, caller must free *out_ast_root
        return true; // Parsing succeeded, stop here
    }

    // --- Code Generation Phase ---
    printf("Generating code...\n");

    // Ensure output buffer is valid if we reach codegen
    if (!output_assembly_sb) {
        fprintf(stderr, "Error: Output string buffer is NULL during code generation.\n");
        // Caller must free *out_ast_root
        return false;
    }
    // No need to init sb, caller does it.

    if (!codegen_generate_program(output_assembly_sb, ast_root_local)) {
        fprintf(stderr, "Code generation failed.\n");
        // output_assembly_sb might be partially filled. Caller might want to destroy it.
        // Caller must free *out_ast_root
        return false; // Codegen failed
    }

    // If codegen_only, print assembly to stdout (caller handles buffer destruction)
    if (codegen_only) {
        printf("Codegen successful. Outputting assembly to stdout:\n");
        printf("--- Generated Assembly (stdout) ---\n");
        const char *assembly_code = string_buffer_get_content(output_assembly_sb);
        printf("%s\n", assembly_code ? assembly_code : "<EMPTY>"); // Print assembly to stdout
        printf("------------------------------------\n");
        // Caller must free *out_ast_root and destroy sb
        return true; // Codegen succeeded, stop here
    }

    // Full success (Lex -> Parse -> Codegen completed for writing)
    // Caller must free *out_ast_root and destroy sb (after writing)
    return true;
}
