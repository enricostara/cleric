#include "compiler.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../parser/ast.h" // Needed for AstNode, FuncDefNode etc.
#include "../codegen/codegen.h"
#include "../strings/strings.h"
#include <stdbool.h>
#include <stdio.h>

#include "ir/ast_to_tac.h"
#include "ir/tac.h"

// -----------------------------------------------------------------------------
// Core Compilation Logic (Source String -> Assembly String Buffer)
// -----------------------------------------------------------------------------

// Forward declarations for static helper functions
static bool run_lexer(Lexer *lexer, Arena *arena, const char *source_code, bool print_tokens);

// Takes an initialized lexer

static bool run_parser(Parser *parser, Lexer *lexer, Arena *arena, bool print_ast, ProgramNode **out_program);

static bool run_irgen(ProgramNode *ast_root, Arena *arena, TacProgram **out_tac_program, bool print_tac);

static bool run_codegen(ProgramNode *program, StringBuffer *output_assembly_sb, bool print_assembly);

// Add IRGen step

bool compile(const char *source_code,
             const bool lex_only,
             const bool parse_only,
             const bool irgen_only,
             const bool codegen_only,
             StringBuffer *output_assembly_sb,
             Arena *arena) {

    Lexer lexer;
    // Initialize lexer with the arena
    lexer_init(&lexer, source_code, arena);

    // --- Lexing Phase ---
    // Pass the arena, even if just lexing, as lexemes are allocated into it.
    bool const lex_success = run_lexer(&lexer, arena, source_code, (lex_only || parse_only || codegen_only));
    if (!lex_success) {
        return false; // Lexical error
    }

    if (lex_only) {
        return true; // Lexing succeeded, stop here
    }

    // --- Reset Lexer for Parsing ---
    lexer_reset(&lexer); // Reset the same lexer instance

    Parser parser;
    parser_init(&parser, &lexer, arena); // Pass arena to parser_init

    // --- Parsing Phase ---
    ProgramNode *program;
    const bool parse_success = run_parser(&parser, &lexer, arena, parse_only || codegen_only, &program);
    if (!parse_success) {
        return false;
    }

    if (parse_only) {
        return true; // Parsing succeeded, stop here
    }

    // --- IR Generation Phase (AST -> TAC) ---
    TacProgram *tac_program; // Declare variable to hold the result
    const bool irgen_success = run_irgen(program, arena, &tac_program, codegen_only || irgen_only);
    if (!irgen_success) {
        // Error message printed by run_irgen
        return false; // IR generation failed
    }

    // If irgen_only is requested, stop here after successful IR generation
    if (irgen_only) {
        // todo print tac
        return true;
    }

    // --- Code Generation Phase ---
    // Ensure output buffer is valid if we reach codegen
    if (!output_assembly_sb) {
        fprintf(stderr, "Compiler Error: Output string buffer is NULL during code generation.\n");
        return false;
    }

    const bool codegen_success = run_codegen(program, output_assembly_sb, codegen_only);

    return codegen_success; // Return success status of the final stage
}

// -----------------------------------------------------------------------------
// Helper Functions for Compilation Stages
// -----------------------------------------------------------------------------

static bool run_lexer(Lexer *lexer, Arena *arena, const char *source_code, bool print_tokens) {
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

static bool run_parser(Parser *parser, Lexer *lexer, Arena *arena, bool print_ast, ProgramNode **out_program) {
    // Assume lexer is already initialized and positioned at the start
    printf("Parsing...\n");
    ProgramNode *ast_root_local = parse_program(parser);

    if (parser->error_flag) {
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
static bool run_irgen(ProgramNode *ast_root, Arena *arena, TacProgram **out_tac_program, bool print_tac) {
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

    string_buffer_reset(output_assembly_sb);

    if (!codegen_generate_program(output_assembly_sb, program)) {
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
