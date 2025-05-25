#ifndef AST_H
#define AST_H

#include "memory/arena.h" // Include arena header

// Define the types of AST nodes we need for "int main(void) { return 2; }"
typedef enum {
    NODE_PROGRAM, // Represents the entire program (a sequence of top-level declarations)
    NODE_FUNC_DEF, // Represents a function definition
    NODE_RETURN_STMT, // Represents a return statement
    NODE_INT_LITERAL, // Represents an integer literal constant
    NODE_UNARY_OP, // Represents a unary operation (e.g., -, ~)
    NODE_BINARY_OP, // Represents a binary operation (e.g., +, -, *, /, %)
    NODE_VAR_DECL,    // Represents a variable declaration (e.g., int x; int y = 10;)
    NODE_IDENTIFIER,   // Represents the usage of an identifier (e.g., a variable name in an expression)
    NODE_BLOCK        // Represents a block of code { ... } containing declarations and statements
} NodeType;

// Define the types of Unary Operators
typedef enum {
    OPERATOR_NEGATE, // - (Arithmetic negation)
    OPERATOR_COMPLEMENT, // ~ (Bitwise complement)
    OPERATOR_LOGICAL_NOT // ! (Logical NOT)
} UnaryOperatorType;

// Define the types of Binary Operators
typedef enum {
    OPERATOR_ADD,         // +
    OPERATOR_SUBTRACT,    // - (binary)
    OPERATOR_MULTIPLY,    // *
    OPERATOR_DIVIDE,      // /
    OPERATOR_MODULO,      // %

    // Relational Operators
    OPERATOR_LESS,          // <
    OPERATOR_GREATER,       // >
    OPERATOR_LESS_EQUAL,    // <=
    OPERATOR_GREATER_EQUAL, // >=
    OPERATOR_EQUAL_EQUAL,   // ==
    OPERATOR_NOT_EQUAL,     // !=

    // Logical Operators
    OPERATOR_LOGICAL_AND,   // &&
    OPERATOR_LOGICAL_OR,    // ||

    // Assignment and Sequencing
    OPERATOR_ASSIGN,        // =
    OPERATOR_COMMA          // , (for future use)
} BinaryOperatorType;

// Base structure for all AST nodes
typedef struct AstNode {
    NodeType type;
} AstNode;

// Structure for an integer literal node
typedef struct {
    AstNode base; // Inherits from AstNode (type will be NODE_INT_LITERAL)
    int value; // The integer value
} IntLiteralNode;

// Structure for a unary operation node
typedef struct {
    AstNode base; // type = NODE_UNARY_OP
    UnaryOperatorType op; // The specific unary operator (NEGATE or COMPLEMENT)
    AstNode *operand; // The expression the operator applies to
} UnaryOpNode;

// Structure for a binary operation node
typedef struct {
    AstNode base; // type = NODE_BINARY_OP
    BinaryOperatorType op; // The specific binary operator
    AstNode *left;  // Left-hand side operand
    AstNode *right; // Right-hand side operand
} BinaryOpNode;

// Structure for a variable declaration node
typedef struct {
    AstNode base;          // type = NODE_VAR_DECL
    char *type_name;       // For now, just the string name of the type (e.g., "int")
    char *var_name;        // Name of the variable
    AstNode *initializer;  // Optional expression for initialization (can be NULL)
} VarDeclNode;

// Structure for an identifier node (usage of a variable)
typedef struct {
    AstNode base;          // type = NODE_IDENTIFIER
    char *name;            // Name of the identifier being referenced
} IdentifierNode;

// Structure for a return statement node
typedef struct {
    AstNode base; // type = NODE_RETURN_STMT
    AstNode *expression; // The expression being returned (e.g., an IntLiteralNode, a UnaryOpNode, a BinaryOpNode)
} ReturnStmtNode;

// Structure for a block of code { ... }
// Can contain a sequence of declarations and statements (C99 style)
typedef struct {
    AstNode base;          // type = NODE_BLOCK
    AstNode **items;       // Dynamic array of AstNode pointers (declarations or statements)
    size_t num_items;      // Number of items in the block
    size_t capacity;       // Current capacity of the items array (for dynamic resizing)
} BlockNode;

// Structure for a function definition node
// Simplified for "int main(void) { return 2; }"
typedef struct {
    AstNode base; // type = NODE_FUNC_DEF
    char *name; // Name of the function (e.g., "main")
    BlockNode *body; // The block of code forming the function's body
} FuncDefNode;

// Structure for the program node (root of the AST)
typedef struct {
    AstNode base; // type = NODE_PROGRAM
    // For now, we assume only one function definition in the program
    FuncDefNode *function;
} ProgramNode;

// Function to create an integer literal node (convenience constructor)
IntLiteralNode *create_int_literal_node(int value, Arena* arena);

// Function to create a unary operation node (convenience constructor)
UnaryOpNode *create_unary_op_node(UnaryOperatorType op, AstNode *operand, Arena *arena);

// Function to create a binary operation node (convenience constructor)
BinaryOpNode *create_binary_op_node(BinaryOperatorType op, AstNode *left, AstNode *right, Arena* arena);

// Function to create a variable declaration node (convenience constructor)
VarDeclNode *create_var_decl_node(const char *type_name, const char *var_name, AstNode *initializer, Arena *arena);

// Function to create an identifier node (convenience constructor)
IdentifierNode *create_identifier_node(const char *name, Arena *arena);

// Function to create a return statement node (convenience constructor)
ReturnStmtNode *create_return_stmt_node(AstNode *expression, Arena* arena);

// Function to create a block node (convenience constructor)
BlockNode *create_block_node(Arena *arena);
// Helper function to add an item (declaration or statement) to a block node
// Returns true on success, false on failure (e.g., reallocation error)
bool block_node_add_item(BlockNode *block, AstNode *item, Arena *arena);

// Function to create a function definition node (convenience constructor)
// Takes the function name and body statement as input
FuncDefNode *create_func_def_node(const char *name, BlockNode *body, Arena* arena);

// Function to create the program node (convenience constructor)
ProgramNode *create_program_node(FuncDefNode *function, Arena* arena);

// Function to free the entire AST (important for memory management)
// NOTE: With arena allocation, freeing individual nodes might not be needed,
void free_ast(AstNode *node);

// Function to pretty-print the AST starting from a given node
void ast_pretty_print(AstNode *node, int initial_indent);


#endif // AST_H
