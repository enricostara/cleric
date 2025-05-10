#include "ast.h"
#include "memory/arena.h" // Use arena allocator
#include <stdio.h>         // For error messages (optional)
#include <string.h>        // For strlen, strcpy

// Function to create an integer literal node
IntLiteralNode *create_int_literal_node(const int value, Arena* arena) {
    IntLiteralNode *node = arena_alloc(arena, sizeof(IntLiteralNode));
    if (!node) {
        // arena_alloc already prints errors, just return NULL
        return NULL;
    }
    node->base.type = NODE_INT_LITERAL;
    node->value = value;
    return node;
}

// Function to create a return statement node
ReturnStmtNode *create_return_stmt_node(AstNode *expression, Arena* arena) {
    ReturnStmtNode *node = arena_alloc(arena, sizeof(ReturnStmtNode));
    if (!node) {
        return NULL;
    }
    node->base.type = NODE_RETURN_STMT;
    node->expression = expression; // Expression node itself was allocated previously (likely in same arena)
    return node;
}

// Function to create a function definition node
FuncDefNode *create_func_def_node(const char *name, AstNode *body, Arena* arena) {
    FuncDefNode *node = arena_alloc(arena, sizeof(FuncDefNode));
    if (!node) {
        return NULL;
    }
    node->base.type = NODE_FUNC_DEF;
    node->body = body; // Body node allocated previously (likely in same arena)

    // Allocate space for name in the arena and copy it
    if (name) {
        size_t name_len = strlen(name);
        node->name = arena_alloc(arena, name_len + 1);
        if (!node->name) {
            // Allocation for name failed, even though node allocation succeeded.
            // This is unlikely but possible if arena is exactly full.
            // The FuncDefNode is allocated but unusable without a name.
            // Return NULL to indicate overall failure.
            // The partially allocated FuncDefNode remains in the arena but won't be used.
            return NULL;
        }
        strcpy(node->name, name); // strcpy is safe due to allocating name_len + 1 bytes
    } else {
        node->name = NULL;
    }

    return node;
}

// Function to create the program node
ProgramNode *create_program_node(FuncDefNode *function, Arena* arena) {
    ProgramNode *node = arena_alloc(arena, sizeof(ProgramNode));
    if (!node) {
        return NULL;
    }
    node->base.type = NODE_PROGRAM;
    node->function = function; // Function node allocated previously (likely in same arena)
    return node;
}

// Function to create a unary operation node
UnaryOpNode *create_unary_op_node(const UnaryOperatorType op, AstNode *operand, Arena *arena) {
    UnaryOpNode *node = arena_alloc(arena, sizeof(UnaryOpNode));
    if (!node) {
        return NULL; // Allocation failed
    }
    node->base.type = NODE_UNARY_OP;
    node->op = op;
    node->operand = operand; // Operand node allocated previously
    return node;
}

// Function to create a binary operation node
BinaryOpNode *create_binary_op_node(const BinaryOperatorType op, AstNode *left, AstNode *right, Arena *arena) {
    BinaryOpNode *node = arena_alloc(arena, sizeof(BinaryOpNode));
    if (!node) {
        return NULL; // Allocation failed
    }
    node->base.type = NODE_BINARY_OP;
    node->op = op;
    node->left = left;   // Left operand node allocated previously
    node->right = right; // Right operand node allocated previously
    return node;
}

// Helper function to print indentation
static void print_indent(int level) {
    for (int i = 0; i < level; ++i) {
        printf("  "); // Print two spaces per indentation level
    }
}

// Recursive function to pretty-print the AST
void ast_pretty_print(AstNode *node, const int indent_level) { // NOLINT(*-no-recursion)
    if (!node) {
        print_indent(indent_level);
        printf("NULL_NODE\n");
        return;
    }

    print_indent(indent_level);

    switch (node->type) {
        case NODE_PROGRAM: {
            const ProgramNode *prog_node = (ProgramNode *) node;
            printf("Program(\n");
            ast_pretty_print((AstNode *) prog_node->function, indent_level + 1);
            print_indent(indent_level);
            printf(")\n");
            break;
        }
        case NODE_FUNC_DEF: {
            const FuncDefNode *func_node = (FuncDefNode *) node;
            // In a real scenario, you'd print function name, return type, params here.
            printf("Function(name=\"%s\",\n", func_node->name ? func_node->name : "<null>"); // Print actual name
            print_indent(indent_level + 1);
            printf("body=\n");
            ast_pretty_print(func_node->body, indent_level + 2); // Indent body further
            print_indent(indent_level);
            printf(")\n");
            break;
        }
        case NODE_RETURN_STMT: {
            const ReturnStmtNode *ret_node = (ReturnStmtNode *) node;
            printf("Return(\n");
            ast_pretty_print(ret_node->expression, indent_level + 1);
            print_indent(indent_level);
            printf(")\n");
            break;
        }
        case NODE_UNARY_OP: { // Added case for unary operators
            const UnaryOpNode *unary_node = (UnaryOpNode *) node;
            const char *op_str = (unary_node->op == OPERATOR_NEGATE) ? "Negate" :
                                (unary_node->op == OPERATOR_COMPLEMENT) ? "Complement" :
                                "UnknownOp";
            printf("UnaryOp(op=%s,\n", op_str);
            ast_pretty_print(unary_node->operand, indent_level + 1);
            print_indent(indent_level);
            printf(")\n");
            break;
        }
        case NODE_BINARY_OP: {
            const BinaryOpNode *bin_node = (BinaryOpNode *) node;
            const char *op_str;
            switch (bin_node->op) {
                case OPERATOR_ADD: op_str = "Add"; break;
                case OPERATOR_SUBTRACT: op_str = "Subtract"; break;
                case OPERATOR_MULTIPLY: op_str = "Multiply"; break;
                case OPERATOR_DIVIDE: op_str = "Divide"; break;
                case OPERATOR_MODULO: op_str = "Modulo"; break;
                default: op_str = "UnknownBinaryOp"; break;
            }
            printf("BinaryOp(op=%s,\n", op_str);
            print_indent(indent_level + 1);
            printf("left=\n");
            ast_pretty_print(bin_node->left, indent_level + 2);
            print_indent(indent_level + 1);
            printf("right=\n");
            ast_pretty_print(bin_node->right, indent_level + 2);
            print_indent(indent_level);
            printf(")\n");
            break;
        }
        case NODE_INT_LITERAL: {
            const IntLiteralNode *int_node = (IntLiteralNode *) node;
            printf("Constant(%d)\n", int_node->value);
            break;
        }
        default:
            printf("UnknownNode(type=%d)\n", node->type);
            break;
    }
}
