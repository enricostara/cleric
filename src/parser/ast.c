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
FuncDefNode *create_func_def_node(const char *name, BlockNode *body, Arena* arena) {
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

// Function to create a variable declaration node
VarDeclNode *create_var_decl_node(const char *type_name, const char *var_name, AstNode *initializer, Arena *arena) {
    VarDeclNode *node = arena_alloc(arena, sizeof(VarDeclNode));
    if (!node) {
        return NULL;
    }
    node->base.type = NODE_VAR_DECL;
    node->initializer = initializer; // Initializer is already an AST node from the arena

    // Allocate space for type_name in the arena and copy it
    if (type_name) {
        size_t type_name_len = strlen(type_name);
        node->type_name = arena_alloc(arena, type_name_len + 1);
        if (!node->type_name) {
            return NULL; // Allocation failed
        }
        strcpy(node->type_name, type_name);
    } else {
        node->type_name = NULL; // Should not happen for valid declarations
    }

    // Allocate space for var_name in the arena and copy it
    if (var_name) {
        size_t var_name_len = strlen(var_name);
        node->var_name = arena_alloc(arena, var_name_len + 1);
        if (!node->var_name) {
            return NULL; // Allocation failed
        }
        strcpy(node->var_name, var_name);
    } else {
        node->var_name = NULL; // Should not happen for valid declarations
    }

    return node;
}

// Function to create an identifier node
IdentifierNode *create_identifier_node(const char *name, Arena *arena) {
    IdentifierNode *node = arena_alloc(arena, sizeof(IdentifierNode));
    if (!node) {
        return NULL;
    }
    node->base.type = NODE_IDENTIFIER;

    if (name) {
        size_t name_len = strlen(name);
        node->name = arena_alloc(arena, name_len + 1);
        if (!node->name) {
            return NULL; // Allocation failed
        }
        strcpy(node->name, name);
    } else {
        node->name = NULL; // Should not happen for valid identifiers
    }
    return node;
}

// Function to create a block node
BlockNode *create_block_node(Arena *arena) {
    BlockNode *node = arena_alloc(arena, sizeof(BlockNode));
    if (!node) {
        return NULL;
    }
    node->base.type = NODE_BLOCK;
    node->items = NULL;
    node->num_items = 0;
    node->capacity = 0;
    return node;
}

// Helper function to add an item (declaration or statement) to a block node
// Returns true on success, false on failure (e.g., reallocation error)
#define INITIAL_BLOCK_CAPACITY 8
bool block_node_add_item(BlockNode *block, AstNode *item, Arena *arena) {
    if (!block || !item) {
        return false;
    }

    if (block->num_items >= block->capacity) {
        size_t new_capacity = (block->capacity == 0) ? INITIAL_BLOCK_CAPACITY : block->capacity * 2;
        AstNode **new_items = arena_alloc(arena, new_capacity * sizeof(AstNode *));
        if (!new_items) {
            return false; // Allocation failed
        }

        // Copy existing items to the new array
        if (block->items) { // Check if block->items is not NULL before copying
            for (size_t i = 0; i < block->num_items; ++i) {
                new_items[i] = block->items[i];
            }
            // The old block->items array was allocated from the arena, so it doesn't need to be freed here.
            // It will be reclaimed when the arena is reset or destroyed.
        }
        
        block->items = new_items;
        block->capacity = new_capacity;
    }

    block->items[block->num_items++] = item;
    return true;
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
            ast_pretty_print((AstNode *) func_node->body, indent_level + 2); // Indent body further
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
            const char *op_str;
            switch (unary_node->op) {
                case OPERATOR_NEGATE: op_str = "Negate"; break;
                case OPERATOR_COMPLEMENT: op_str = "Complement"; break;
                case OPERATOR_LOGICAL_NOT: op_str = "LogicalNot"; break;
                default: op_str = "UnknownUnaryOp"; break;
            }
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
                case OPERATOR_LESS: op_str = "Less"; break;
                case OPERATOR_GREATER: op_str = "Greater"; break;
                case OPERATOR_LESS_EQUAL: op_str = "LessEqual"; break;
                case OPERATOR_GREATER_EQUAL: op_str = "GreaterEqual"; break;
                case OPERATOR_EQUAL_EQUAL: op_str = "EqualEqual"; break;
                case OPERATOR_NOT_EQUAL: op_str = "NotEqual"; break;
                case OPERATOR_LOGICAL_AND: op_str = "LogicalAnd"; break;
                case OPERATOR_LOGICAL_OR: op_str = "LogicalOr"; break;
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
        case NODE_BLOCK: { // Added case for block nodes
            const BlockNode *block_node = (BlockNode *) node;
            printf("Block(\n");
            for (size_t i = 0; i < block_node->num_items; ++i) {
                ast_pretty_print(block_node->items[i], indent_level + 1);
            }
            print_indent(indent_level);
            printf(")\n");
            break;
        }
        case NODE_VAR_DECL: { // Added case for variable declarations
            const VarDeclNode *var_decl_node = (VarDeclNode *) node;
            printf("VarDecl(type=%s, name=%s", 
                   var_decl_node->type_name ? var_decl_node->type_name : "<null_type>",
                   var_decl_node->var_name ? var_decl_node->var_name : "<null_name>");
            if (var_decl_node->initializer) {
                printf(",\n");
                print_indent(indent_level + 1);
                printf("initializer=\n");
                ast_pretty_print(var_decl_node->initializer, indent_level + 2);
                print_indent(indent_level);
                printf(")\n");
            } else {
                printf(")\n");
            }
            break;
        }
        case NODE_IDENTIFIER: { // Added case for identifiers
            const IdentifierNode *id_node = (IdentifierNode *) node;
            printf("Identifier(name=%s)\n", id_node->name ? id_node->name : "<null_id_name>");
            break;
        }
        default:
            printf("UnknownNode(type=%d)\n", node->type);
            break;
    }
}
