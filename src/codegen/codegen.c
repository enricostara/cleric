#include "codegen.h"
#include "../strings/strings.h" // Use the new string buffer
#include <stdio.h>
#include <stdbool.h> // Needed for bool

// --- Forward declarations for static helper functions (AST visitors) ---
static bool generate_program(const ProgramNode *program, StringBuffer *sb);

static bool generate_function(const FuncDefNode *node, StringBuffer *sb);

static bool generate_statement(AstNode *node, StringBuffer *sb); // Use base AstNode type
static bool generate_expression(AstNode *node, int *out_value); // Modified to return value via out-param

// --- Main function ---

bool codegen_generate_program(StringBuffer *sb, ProgramNode *program) {
    if (!program) {
        fprintf(stderr, "Codegen Error: Cannot generate assembly from NULL program AST\n");
        return false; // Return false on error
    }
    // Ensure buffer is empty before generation
    string_buffer_clear(sb);
    // Start the generation process
    return generate_program(program, sb);
}

// --- Static helper function implementations ---

// AST Visitors
static bool generate_program(const ProgramNode *program, StringBuffer *sb) {
    // For now, assume program has exactly one function definition
    if (program->function) {
        if (!generate_function(program->function, sb)) {
            // Check return value
            return false; // Propagate error
        }
    } else {
        fprintf(stderr, "Codegen Error: Program node has no function definitions.\n");
        return false; // Error: no function
    }
    return true; // Success
}

static bool generate_function(const FuncDefNode *node, StringBuffer *sb) {
    // Basic structure for macOS x86-64
    string_buffer_append(sb, ".globl _%s\n", node->name);
    string_buffer_append(sb, "_%s:\n", node->name);

    // Function Prologue (Minimal example)
    // string_buffer_append(sb, "    pushq %%rbp\n");
    // string_buffer_append(sb, "    movq %%rsp, %%rbp\n");

    // Generate code for the function body (statements)
    if (node->body) {
        if (!generate_statement(node->body, sb)) {
            // Check return value
            return false; // Propagate error
        }
    }

    // Function Epilogue (Minimal example - often handled by return)
    // string_buffer_append(sb, "    popq %%rbp\n");
    // Implicit 'retq' might be added by generate_statement for return, or needed here.
    // For now, the return statement adds `retq`.
    // string_buffer_append(sb, "    retq\n");
    return true; // Success
}

static bool generate_statement(AstNode *node, StringBuffer *sb) {
    // ReSharper disable once CppDFAConstantConditions
    if (!node) {
        // Let's assume true for now (empty statement is ok).
        // ReSharper disable once CppDFAUnreachableCode
        return true;
    }

    switch (node->type) {
        // Check type from the base node
        case NODE_RETURN_STMT: {
            ReturnStmtNode *ret_node = (ReturnStmtNode *) node;
            if (ret_node->expression) {
                int value;
                if (!generate_expression(ret_node->expression, &value)) {
                    // Check return value
                    return false; // Propagate error
                }
                // Move immediate value into return register (%eax)
                string_buffer_append(sb, "    movl    $%d, %%eax\n", value);
            } else {
                // Handle void return (e.g., `return;`) - move 0 to eax
                string_buffer_append(sb, "    movl    $0, %%eax\n");
            }
            // Add the return instruction (important!)
            string_buffer_append(sb, "    retq\n");
            break;
        }
        // Add cases for other statement types later
        default:
            fprintf(stderr, "Codegen Error: Unsupported statement type %d\n", node->type);
            return false; // Error: unsupported statement
    }
    return true; // Success
}

// Modified: Returns bool for success, value via out parameter
static bool generate_expression(AstNode *node, int *out_value) {
    // ReSharper disable once CppDFAConstantConditions
    if (!node) {
        // ReSharper disable once CppDFAUnreachableCode
        fprintf(stderr, "Codegen Error: Cannot generate code for NULL expression.\n");
        return false; // Error: null expression
    }

    switch (node->type) {
        case NODE_INT_LITERAL: {
            const IntLiteralNode *int_node = (IntLiteralNode *) node;
            *out_value = int_node->value;
            break;
        }
        // Add cases for other expression types later
        default:
            fprintf(stderr, "Codegen Error: Unsupported expression type %d\n", node->type);
            return false; // Error: unsupported expression
    }
    return true; // Success
}
