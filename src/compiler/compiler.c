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

static bool run_parser(Lexer *lexer, Arena *arena, bool print_ast, ProgramNode **out_program);

static bool run_irgen(ProgramNode *ast_root, Arena *arena, TacProgram **out_tac_program, bool print_tac);

static bool run_codegen(ProgramNode *program, StringBuffer *output_assembly_sb, bool print_assembly);

// Add IRGen step

bool compile(const char *source_code,
             const bool lex_only,
             const bool parse_only,
             const bool irgen_only,
             const bool codegen_only,
             StringBuffer *output_assembly_sb) {
    ProgramNode *program = NULL; // Internal pointer to the AST root

    Lexer lexer;
    // --- Arena Setup ---
    Arena ast_arena = arena_create(1024 * 1024); // 1MB arena
    if (ast_arena.start == NULL) {
        fprintf(stderr, "Compiler Error: Failed to create memory arena for AST.\n");
        return false; // Cannot proceed without memory
    }

    // Initialize lexer with the arena
    lexer_init(&lexer, source_code, &ast_arena);

    // --- Lexing Phase ---
    // Pass the arena, even if just lexing, as lexemes are allocated into it.
    bool const lex_success = run_lexer(&lexer, (lex_only || parse_only || codegen_only));
    if (!lex_success) {
        arena_destroy(&ast_arena); // Clean up arena on lex failure
        return false; // Lexical error
    }

    if (lex_only) {
        arena_destroy(&ast_arena); // Clean up arena
        return true; // Lexing succeeded, stop here
    }

    // --- Reset Lexer for Parsing ---
    lexer_reset(&lexer); // Reset the same lexer instance

    // --- Parsing Phase ---
    const bool parse_success = run_parser(&lexer, &ast_arena, parse_only || codegen_only, &program);
    if (!parse_success) {
        arena_destroy(&ast_arena); // Clean up arena on parse failure
        return false;
    }

    if (parse_only) {
        arena_destroy(&ast_arena); // Clean up arena
        return true; // Parsing succeeded, stop here
    }

    // --- IR Generation Phase (AST -> TAC) ---
    // Note: TAC program shares the same arena as the AST
    // ProgramNode *ast_root_local = (ProgramNode *) ast_root;
    // TacProgram *tac_program; // Declare variable to hold the result
    // const bool irgen_success = run_irgen(ast_root_local, &ast_arena, &tac_program, codegen_only || irgen_only);
    // if (!irgen_success) {
    //     // Error message printed by run_irgen
    //     arena_destroy(&ast_arena); // Clean up arena
    //     return false; // IR generation failed
    // }

    // If irgen_only is requested, stop here after successful IR generation
    // if (irgen_only) {
    //     arena_destroy(&ast_arena);
    //     return true;
    // }

    // --- Code Generation Phase ---
    // Ensure output buffer is valid if we reach codegen
    if (!output_assembly_sb) {
        fprintf(stderr, "Compiler Error: Output string buffer is NULL during code generation.\n");
        arena_destroy(&ast_arena);
        return false;
    }

    const bool codegen_success = run_codegen(program, output_assembly_sb, codegen_only);

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
    printf("Lexing...\n");
    while (true) {
        const bool success = lexer_next_token(lexer, &tok);
        if (!success) {
            // Error message already printed by lexer for allocation failure
            fprintf(stderr, "Lexical error: Lexer failed (likely allocation error).\n");
            return false; // Indicate failure
        }

        if (tok.type == TOKEN_EOF) {
            break; // End of input reached successfully
        }

        if (tok.type == TOKEN_UNKNOWN) {
            char token_str[128];
            token_to_string(tok, token_str, sizeof(token_str));
            fprintf(stderr, "Lexical error: unknown token %s at position %zu\n", token_str, tok.position);
            // No token_free needed; lexeme (if any) is in arena

            // Decide whether to continue lexing or stop on first error.
            // For now, stop on first lexical error.
            return false; // Indicate failure
        }

        if (print_tokens) {
            char token_str[128];
            token_to_string(tok, token_str, sizeof(token_str));
            printf("Token: %s\n", token_str);
        }
    }

    printf("Lexing finished.\n");
    return true; // Lexing completed successfully
}

static bool run_parser(Lexer *lexer, Arena *arena, const bool print_ast, ProgramNode **out_program) {
    // Assume lexer is already initialized and positioned at the start
    Parser parser;
    parser_init(&parser, lexer, arena); // Pass arena to parser_init
 
    printf("Parsing...\n");
    ProgramNode *ast_root_local = parse_program(&parser);
 
    if (parser.error_flag) {
        fprintf(stderr, "Parsing failed due to errors.\n");
        return false; // Parsing failed
    }

    printf("Parsing successful.\n");
    if (print_ast) {
        printf("AST:\n");
        ast_pretty_print((AstNode *) ast_root_local, 0);
    }

    *out_program = ast_root_local; // Store AST root

    return true;
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

static bool run_codegen(ProgramNode *program, StringBuffer *output_assembly_sb, const bool print_assembly) {
    printf("Generating code...\n");

    if (!codegen_generate_program(output_assembly_sb,  program)) {
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
