#include "codegen.h"
#include "../strings/strings.h" // Use the new string buffer

#include <stdio.h>

// --- Forward declarations for static helper functions (AST visitors) ---
static void generate_program(ProgramNode *node, StringBuffer *sb);
static void generate_function(FuncDefNode *node, StringBuffer *sb);
static void generate_statement(StmtNode *node, StringBuffer *sb);
static int generate_expression(ExprNode *node); // Returns the value for literals for now

// --- Main function ---
char *codegen_generate_assembly(ProgramNode *program) {
    if (!program) {
        fprintf(stderr, "Codegen Error: Cannot generate assembly from NULL program AST\n");
        return NULL;
    }

    StringBuffer sb;
    string_buffer_init(&sb, 1024);

    generate_program(program, &sb);

    return string_buffer_get_content(&sb); // Transfers ownership
}

// --- Static helper function implementations ---

// AST Visitors
static void generate_program(ProgramNode *node, StringBuffer *sb) {
    // For now, assume program has exactly one function definition
    if (node->functions && node->functions[0]) {
        generate_function(node->functions[0], sb);
    } else {
        fprintf(stderr, "Codegen Error: Program node has no function definitions.\n");
        // Handle error appropriately, maybe append nothing or an error comment
    }
}

static void generate_function(FuncDefNode *node, StringBuffer *sb) {
    // Basic structure for macOS x86-64
    string_buffer_append(sb, ".section .text\n");
    string_buffer_append(sb, ".globl _%s\n", node->name);
    string_buffer_append(sb, "_%s:\n", node->name);

    // TODO: Function Prologue (stack setup, etc.) - Minimal for now

    // Generate code for the function body (statements)
    if (node->body) {
        generate_statement(node->body, sb);
    }

    // Function Epilogue
    // For simple return, the return statement handles moving value to %eax
    string_buffer_append(sb, "    retq\n");
}

static void generate_statement(StmtNode *node, StringBuffer *sb) {
    if (!node) return;

    switch (node->type) {
        case STMT_RETURN:
        {
            ReturnStmtNode *ret_node = (ReturnStmtNode *)node;
            if (ret_node->expression) {
                // Generate code for the expression to get value (into %eax)
                int return_value = generate_expression(ret_node->expression);
                // Move immediate value into return register (%eax)
                string_buffer_append(sb, "    movl    $%d, %%eax\n", return_value);
            } else {
                // Handle void return (e.g., `return;`) - maybe move 0 to eax?
                string_buffer_append(sb, "    movl    $0, %%eax\n");
            }
            break;
        }
        // Add cases for other statement types later (e.g., STMT_EXPRESSION)
        default:
            fprintf(stderr, "Codegen Warning: Unsupported statement type %d\n", node->type);
            break;
    }
}

static int generate_expression(ExprNode *node) {
    if (!node) {
        fprintf(stderr, "Codegen Error: Cannot generate code for NULL expression.\n");
        return 0; // Or some error indicator
    }

    switch (node->type) {
        case EXPR_INT_LITERAL:
        {
            IntLiteralNode *int_node = (IntLiteralNode *)node;
            return int_node->value;
        }
        // Add cases for other expression types later (variables, operators, etc.)
        default:
            fprintf(stderr, "Codegen Warning: Unsupported expression type %d\n", node->type);
            return 0; // Default/error value
    }
}
