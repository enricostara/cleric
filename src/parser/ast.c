#include "ast.h"
#include <stdlib.h> // For malloc, free
#include <stdio.h>  // For error messages (optional)

// Function to create an integer literal node
AstNode *create_int_literal_node(const long value) {
    IntLiteralNode *node = malloc(sizeof(IntLiteralNode));
    if (!node) {
        perror("Failed to allocate memory for IntLiteralNode");
        return NULL;
    }
    node->base.type = NODE_INT_LITERAL;
    node->value = value;
    return (AstNode *)node; // Cast to base pointer
}

// Function to create a return statement node
AstNode *create_return_stmt_node(AstNode *expression) {
    ReturnStmtNode *node = malloc(sizeof(ReturnStmtNode));
    if (!node) {
        perror("Failed to allocate memory for ReturnStmtNode");
        // Free the expression if allocation fails? Depends on ownership rules.
        // For simplicity here, assume caller manages expression lifetime if this fails.
        return NULL;
    }
    node->base.type = NODE_RETURN_STMT;
    node->expression = expression; // Takes ownership of the expression node
    return (AstNode *)node;
}

// Function to create a function definition node
AstNode *create_func_def_node(AstNode *body) {
    FuncDefNode *node = malloc(sizeof(FuncDefNode));
    if (!node) {
        perror("Failed to allocate memory for FuncDefNode");
        return NULL;
    }
    node->base.type = NODE_FUNC_DEF;
    node->body = body; // Takes ownership of the body node
    return (AstNode *)node;
}

// Function to create the program node
ProgramNode *create_program_node(FuncDefNode *function) {
     ProgramNode *node = malloc(sizeof(ProgramNode));
    if (!node) {
        perror("Failed to allocate memory for ProgramNode");
        return NULL;
    }
    node->base.type = NODE_PROGRAM;
    node->function = function; // Takes ownership of the function node
    return node;
}


// Recursive function to free the AST
void free_ast(AstNode *node) { // NOLINT(*-no-recursion)
    if (!node) {
        return;
    }

    switch (node->type) {
        case NODE_PROGRAM: {
            const ProgramNode *prog_node = (ProgramNode *)node;
            // Assume program owns the function definition
            free_ast((AstNode *)prog_node->function); // Free children first
            break;
        }
        case NODE_FUNC_DEF: {
            const FuncDefNode *func_node = (FuncDefNode *)node;
            // Assume func def owns its body
            free_ast(func_node->body); // Free children first
            break;
        }
        case NODE_RETURN_STMT: {
            const ReturnStmtNode *ret_node = (ReturnStmtNode *)node;
            // Assume return stmt owns its expression
            free_ast(ret_node->expression); // Free children first
            break;
        }
        case NODE_INT_LITERAL: {
            // IntLiteralNode has no children that need freeing
            break;
        }
        default:
            // Should not happen if AST is well-formed
            fprintf(stderr, "Warning: Attempting to free unknown node type: %d\n", node->type);
            break;
    }

    // Free the node itself after handling its children
    free(node);
}

// Helper function to print indentation
static void print_indent(int level) {
    for (int i = 0; i < level; ++i) {
        printf("  "); // Print two spaces per indentation level
    }
}

// Recursive function to pretty-print the AST
void ast_pretty_print(AstNode *node, int indent_level) { // NOLINT(*-no-recursion)
    if (!node) {
        print_indent(indent_level);
        printf("NULL_NODE\n");
        return;
    }

    print_indent(indent_level);

    switch (node->type) {
        case NODE_PROGRAM: {
            ProgramNode *prog_node = (ProgramNode *)node;
            printf("Program(\n");
            ast_pretty_print((AstNode *)prog_node->function, indent_level + 1);
            print_indent(indent_level);
            printf(")\n");
            break;
        }
        case NODE_FUNC_DEF: {
            FuncDefNode *func_node = (FuncDefNode *)node;
            // In a real scenario, you'd print function name, return type, params here.
            // For "int main(void)", we simplify:
            printf("Function(name=\"main\",\n");
            print_indent(indent_level + 1);
            printf("body=\n");
            ast_pretty_print(func_node->body, indent_level + 2); // Indent body further
            print_indent(indent_level);
            printf(")\n");
            break;
        }
        case NODE_RETURN_STMT: {
            ReturnStmtNode *ret_node = (ReturnStmtNode *)node;
            printf("Return(\n");
            ast_pretty_print(ret_node->expression, indent_level + 1);
            print_indent(indent_level);
            printf(")\n");
            break;
        }
        case NODE_INT_LITERAL: {
            IntLiteralNode *int_node = (IntLiteralNode *)node;
            printf("Constant(%ld)\n", int_node->value);
            break;
        }
        default:
            printf("UnknownNode(type=%d)\n", node->type);
            break;
    }
}
