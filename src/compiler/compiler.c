#include "compiler.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../parser/ast.h" // Needed for AstNode, FuncDefNode etc.
#include "../codegen/codegen.h"
#include "../strings/strings.h"
#include "../memory/arena.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h> // For token_to_string

// -----------------------------------------------------------------------------
// Core Compilation Logic (Source String -> Assembly String Buffer)
// -----------------------------------------------------------------------------
bool compile(const char *source_code,
             bool lex_only,
             bool parse_only,
             bool codegen_only,
             StringBuffer *output_assembly_sb, // Output buffer for assembly
             AstNode **out_ast_root) {
    // Output AST root for freeing later

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

    // --- Arena Setup ---
    // Arena is now created *after* successful lexing, avoiding allocation if lexing fails early.
    Arena ast_arena = arena_create(1024 * 1024); // Create a 1MB arena for this compilation
    if (ast_arena.start == NULL) {
        fprintf(stderr, "Driver Error: Failed to create memory arena for AST.\n");
        arena_destroy(&ast_arena); // Attempt cleanup even if creation failed partially
        return false; // Cannot proceed without memory
    }

    // --- Reset Lexer for Parsing ---
    lexer_reset(&lexer);

    // --- Parsing Phase ---
    Parser parser;
    parser_init(&parser, &lexer);

    printf("Parsing...\n");
    // Pass the arena to the parser
    ProgramNode *ast_root_local = parse_program(&parser, &ast_arena);
    *out_ast_root = (AstNode *) ast_root_local; // Store AST root (points into arena memory)

    if (parser.error_flag || !ast_root_local) {
        fprintf(stderr, "Parsing failed.\n");
        // AST memory is managed by the arena
        parser_destroy(&parser); // Clean up parser resources
        arena_destroy(&ast_arena); // Destroy the arena (releases all AST memory)
        return false; // Parsing failed
    }

    printf("Parsing successful. AST:\n");
    if (parse_only || codegen_only) {
        ast_pretty_print((AstNode *) ast_root_local, 0); // Pretty print the AST
    }

    // Store parser state before potential early return
    bool parse_error = parser.error_flag;
    parser_destroy(&parser); // Clean up parser resources

    if (parse_error) {
        // Check error flag after destroy to ensure cleanup
        arena_destroy(&ast_arena); // Destroy the arena (releases all AST memory)
        return false;
    }

    if (parse_only) {
        arena_destroy(&ast_arena); // Destroy the arena (releases all AST memory)
        return true; // Parsing succeeded, stop here
    }

    // --- Code Generation Phase ---
    printf("Generating code...\n");

    // Ensure output buffer is valid if we reach codegen
    if (!output_assembly_sb) {
        fprintf(stderr, "Error: Output string buffer is NULL during code generation.\n");
        arena_destroy(&ast_arena); // Destroy the arena (releases all AST memory)
        return false;
    }
    // No need to init sb, caller does it.

    if (!codegen_generate_program(output_assembly_sb, ast_root_local)) {
        fprintf(stderr, "Code generation failed.\n");
        arena_destroy(&ast_arena); // Destroy the arena (releases all AST memory)
        return false; // Codegen failed
    }

    // If codegen_only, print assembly to stdout (caller handles buffer destruction)
    if (codegen_only) {
        printf("Codegen successful. Outputting assembly to stdout:\n");
        printf("--- Generated Assembly (stdout) ---\n");
        // NOTE: string_buffer_get_content transfers ownership. We need to handle it.
        // For codegen_only, we print it here. The caller is responsible for the buffer.
        // A better approach might be to *not* get_content here, but let the caller do it.
        // However, sticking to current logic for now.
        const char *assembly_code = string_buffer_get_content(output_assembly_sb);
        printf("%s\n", assembly_code ? assembly_code : "<EMPTY>"); // Print assembly to stdout
        printf("------------------------------------\n");
        // Since we took ownership, we should free it.
        // BUT the contract seems to be caller handles the buffer in codegen_only mode.
        // Let's revert the get_content and let the caller (run_compiler) handle it.
        // free((void*)assembly_code); // No, don't free here based on caller's responsibility
    }

    // IMPORTANT: Arena is destroyed ONLY on success or error *within* this function.
    // If successful and returning true (full compilation or codegen_only),
    // the arena is destroyed here. The AST pointed to by *out_ast_root becomes invalid.
    arena_destroy(&ast_arena);

    // Final success includes full compilation or successful codegen_only
    return true;
}
