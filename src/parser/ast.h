#ifndef AST_H
#define AST_H

// Define the types of AST nodes we need for "int main(void) { return 2; }"
typedef enum {
    NODE_PROGRAM,       // Represents the entire program (a sequence of top-level declarations)
    NODE_FUNC_DEF,      // Represents a function definition
    NODE_RETURN_STMT,   // Represents a return statement
    NODE_INT_LITERAL    // Represents an integer literal constant
} NodeType;

// Base structure for all AST nodes
typedef struct AstNode {
    NodeType type;
} AstNode;

// Structure for an integer literal node
typedef struct {
    AstNode base; // Inherits from AstNode (type will be NODE_INT_LITERAL)
    long value;   // The integer value
} IntLiteralNode;

// Structure for a return statement node
typedef struct {
    AstNode base;         // type = NODE_RETURN_STMT
    AstNode *expression;  // The expression being returned (e.g., an IntLiteralNode)
} ReturnStmtNode;

// Structure for a function definition node
// Simplified for "int main(void) { return 2; }"
typedef struct {
    AstNode base;       // type = NODE_FUNC_DEF
    char *func_name;    // Name of the function (e.g., "main")
    AstNode *body;      // The statement inside the function (e.g., a ReturnStmtNode)
} FuncDefNode;

// Structure for the program node (root of the AST)
typedef struct {
    AstNode base;       // type = NODE_PROGRAM
    // For now, we assume only one function definition in the program
    FuncDefNode *function;
} ProgramNode;

// Function to create an integer literal node (convenience constructor)
AstNode *create_int_literal_node(long value);

// Function to create a return statement node (convenience constructor)
AstNode *create_return_stmt_node(AstNode *expression);

// Function to create a function definition node (convenience constructor)
// Takes the function name and body statement as input
AstNode *create_func_def_node(const char *name, AstNode *body);

// Function to create the program node (convenience constructor)
ProgramNode *create_program_node(FuncDefNode *function);

// Function to free the entire AST (important for memory management)
void free_ast(AstNode *node);

// Function to pretty-print the AST starting from a given node
void ast_pretty_print(AstNode *node, int initial_indent);


#endif // AST_H
