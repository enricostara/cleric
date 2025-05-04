#include "compiler.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../parser/ast.h" // Needed for AstNode, FuncDefNode etc.
#include "../codegen/codegen.h"
#include "../strings/strings.h"
#include "../memory/arena.h"
#include "../ir/tac.h"         // Include TAC header
#include "../ir/ast_to_tac.h"  // Include AST-to-TAC header
#include <stdbool.h>
#include <stdio.h>
#include <string.h> // For token_to_string

// -----------------------------------------------------------------------------
// Core Compilation Logic (Source String -> Assembly String Buffer)
// -----------------------------------------------------------------------------

// Forward declarations for static helper functions
static bool run_lexer(Lexer *lexer, bool print_tokens); // Takes an initialized lexer

static bool run_parser(Lexer *lexer, Arena *arena, bool print_ast, AstNode **out_ast_root);

static bool run_irgen(ProgramNode *ast_root, Arena *arena, TacProgram **out_tac_program, bool print_tac);

static bool run_codegen(ProgramNode *ast_root, StringBuffer *output_assembly_sb, bool print_assembly);

// Add IRGen step

bool compile(const char *source_code,
             const bool lex_only,
             const bool parse_only,
             const bool irgen_only,
             const bool codegen_only,
             StringBuffer *output_assembly_sb,
             AstNode **out_ast_root) {
    *out_ast_root = NULL; // Initialize output AST pointer

    Lexer lexer;
    lexer_init(&lexer, source_code);

    // --- Lexing Phase ---
    bool const lex_success = run_lexer(&lexer, (lex_only || parse_only || codegen_only));
    if (!lex_success) {
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

    // --- Reset Lexer for Parsing ---
    lexer_reset(&lexer); // Reset the same lexer instance

    // --- Parsing Phase ---
    bool parse_success = run_parser(&lexer, &ast_arena, (parse_only || codegen_only), out_ast_root);
    if (!parse_success) {
        arena_destroy(&ast_arena); // Clean up arena on parse failure
        return false;
    }

    if (parse_only) {
        return true; // Parsing succeeded, stop here
    }

    // --- IR Generation Phase (AST -> TAC) ---
    // Note: TAC program shares the same arena as the AST
    ProgramNode *ast_root_local = (ProgramNode *) *out_ast_root;
    TacProgram *tac_program; // Declare variable to hold the result
    bool irgen_success = run_irgen(ast_root_local, &ast_arena, &tac_program, false
                                   /* print_tac */);
    if (!irgen_success) {
        // Error message printed by run_irgen
        arena_destroy(&ast_arena); // Clean up arena
        return false; // IR generation failed
    }

    // If irgen_only is requested, stop here after successful IR generation
    if (irgen_only) {
        arena_destroy(&ast_arena);
        return true;
    }

    // --- Code Generation Phase ---
    // Ensure output buffer is valid if we reach codegen
    if (!output_assembly_sb) {
        fprintf(stderr, "Compiler Error: Output string buffer is NULL during code generation.\n");
        arena_destroy(&ast_arena);
        return false;
    }

    const bool codegen_success = run_codegen(ast_root_local, output_assembly_sb, codegen_only);

    // Cleanup arena regardless of codegen success/failure *if* we got this far
    // This cleans up memory for both AST and TAC structures
    arena_destroy(&ast_arena);

    return codegen_success; // Return success status of the final stage
}

// -----------------------------------------------------------------------------
// Helper Functions for Compilation Stages
// -----------------------------------------------------------------------------

static bool run_lexer(Lexer *lexer, const bool print_tokens) {
    // Assumes lexer is already initialized
    Token tok;
    int error = 0;
    printf("Lexing...\n");
    while ((tok = lexer_next_token(lexer)).type != TOKEN_EOF) {
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

static bool run_parser(Lexer *lexer, Arena *arena, const bool print_ast, AstNode **out_ast_root) {
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

// -----------------------------------------------------------------------------
// IR Generation (AST -> TAC)
// -----------------------------------------------------------------------------
static bool run_irgen(ProgramNode *ast_root, Arena *arena, TacProgram **out_tac_program, const bool print_tac) {
    printf("Generating IR (TAC)...\n");

    // The ast_to_tac function uses the same arena provided for the AST
    TacProgram *tac_program = ast_to_tac(ast_root, arena);

    if (!tac_program) {
        fprintf(stderr, "IR generation (AST to TAC) failed.\n");
        return false; // Return failure status
    }

    printf("IR generation successful.\n");

    if (print_tac) {
        // TODO: Implement TAC printing function
        printf("--- Generated TAC (stdout) ---\n");
        printf("TAC printing not yet implemented.\n");
        printf("------------------------------\n");
    }

    *out_tac_program = tac_program; // Assign to output parameter
    return true; // Return success status
}

static bool run_codegen(ProgramNode *ast_root, StringBuffer *output_assembly_sb, const bool print_assembly) {
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
