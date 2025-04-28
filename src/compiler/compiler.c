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

// Forward declarations for static helper functions
static bool run_lexer(const char *source_code, bool print_tokens);
static bool run_parser(Lexer *lexer, Arena *arena, bool print_ast, AstNode **out_ast_root);
static bool run_codegen(ProgramNode *ast_root, StringBuffer *output_assembly_sb, bool print_assembly);

bool compile(const char *source_code,
             bool lex_only,
             bool parse_only,
             bool codegen_only,
             StringBuffer *output_assembly_sb, // Output buffer for assembly
             AstNode **out_ast_root) {

    *out_ast_root = NULL; // Initialize output AST pointer

    // --- Lexing Phase ---
    if (!run_lexer(source_code, lex_only || parse_only || codegen_only)) {
        return false; // Lexical error
    }

    if (lex_only) {
        return true; // Lexing succeeded, stop here
    }

    // --- Arena Setup ---
    Arena ast_arena = arena_create(1024 * 1024); // 1MB arena
    if (ast_arena.start == NULL) {
        fprintf(stderr, "Compiler Error: Failed to create memory arena for AST.\n");
        return false; // Cannot proceed without memory
    }

    // --- Parsing Phase ---
    Lexer lexer;
    lexer_init(&lexer, source_code);
    lexer_reset(&lexer); // Reset lexer for parsing
    bool parse_success = run_parser(&lexer, &ast_arena, (parse_only || codegen_only), out_ast_root);
    if (!parse_success) {
        arena_destroy(&ast_arena); // Clean up arena on parse failure
        return false;
    }

    if (parse_only) {
        arena_destroy(&ast_arena); // Clean up arena before returning
        return true; // Parsing succeeded, stop here
    }

    // --- Code Generation Phase ---
    // Ensure output buffer is valid if we reach codegen
    if (!output_assembly_sb) {
        fprintf(stderr, "Compiler Error: Output string buffer is NULL during code generation.\n");
        arena_destroy(&ast_arena);
        return false;
    }

    // Get the root node pointer from the output parameter
    ProgramNode *ast_root_local = (ProgramNode *)*out_ast_root;

    bool codegen_success = run_codegen(ast_root_local, output_assembly_sb, codegen_only);

    // Cleanup arena regardless of codegen success/failure *if* we got this far
    arena_destroy(&ast_arena);

    return codegen_success; // Return success status of the final stage
}

// -----------------------------------------------------------------------------
// Helper Functions for Compilation Stages
// -----------------------------------------------------------------------------

static bool run_lexer(const char *source_code, bool print_tokens) {
    Lexer lexer;
    lexer_init(&lexer, source_code);
    Token tok;
    int error = 0;
    printf("Lexing...\n");
    while ((tok = lexer_next_token(&lexer)).type != TOKEN_EOF) {
        if (tok.type == TOKEN_UNKNOWN) {
            char token_str[128];
            token_to_string(tok, token_str, sizeof(token_str));
            fprintf(stderr, "Lexical error: unknown token %s at position %zu\n", token_str, tok.position);
            token_free(&tok);
            error = 1;
            break;
        }
        if (print_tokens) {
            char token_str[128];
            token_to_string(tok, token_str, sizeof(token_str));
            printf("%s, pos=%zu\n", token_str, tok.position);
        }
        token_free(&tok);
    }
    if (error) {
        printf("Lexing failed.\n");
        return false; // Lexical error
    }
    printf("Lexing successful.\n");
    return true;
}

static bool run_parser(Lexer *lexer, Arena *arena, bool print_ast, AstNode **out_ast_root) {
    // Assume lexer is already initialized and positioned at the start
    Parser parser;
    parser_init(&parser, lexer);

    printf("Parsing...\n");
    ProgramNode *ast_root_local = parse_program(&parser, arena);
    *out_ast_root = (AstNode *) ast_root_local; // Store AST root

    if (parser.error_flag || !ast_root_local) {
        fprintf(stderr, "Parsing failed.\n");
        parser_destroy(&parser);
        return false; // Parsing failed
    }

    printf("Parsing successful.\n");
    if (print_ast) {
        printf("AST:\n");
        ast_pretty_print((AstNode *) ast_root_local, 0);
    }

    bool parse_error = parser.error_flag;
    parser_destroy(&parser); // Clean up parser resources

    return !parse_error;
}

static bool run_codegen(ProgramNode *ast_root, StringBuffer *output_assembly_sb, bool print_assembly) {
    printf("Generating code...\n");

    if (!codegen_generate_program(output_assembly_sb, ast_root)) {
        fprintf(stderr, "Code generation failed.\n");
        return false; // Codegen failed
    }

    printf("Code generation successful.\n");
    if (print_assembly) {
        printf("--- Generated Assembly (stdout) ---\n");
        const char *assembly_code = string_buffer_content_str(output_assembly_sb);
        printf("%s\n", assembly_code ? assembly_code : "<EMPTY>");
        printf("------------------------------------\n");
    }

    return true;
}
