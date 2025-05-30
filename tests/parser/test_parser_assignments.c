#include <string.h>
#include <stdio.h>

#include "unity.h"
#include "../../src/lexer/lexer.h"
#include "../../src/parser/parser.h"
#include "../../src/parser/ast.h"
#include "../../src/memory/arena.h"

// --- Test Helper Functions ---

// Helper to verify an IdentifierNode
static void verify_identifier_node(AstNode *node, const char *expected_name) {
    TEST_ASSERT_NOT_NULL_MESSAGE(node, "IdentifierNode should not be NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_IDENTIFIER, node->type, "Node type should be NODE_IDENTIFIER");
    IdentifierNode *id_node = (IdentifierNode *) node;
    TEST_ASSERT_NOT_NULL_MESSAGE(id_node->name, "IdentifierNode name should not be NULL");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_name, id_node->name, "Identifier name mismatch");
}

// Helper to verify an IntLiteralNode
static void verify_int_literal_node(AstNode *node, int expected_value) {
    TEST_ASSERT_NOT_NULL_MESSAGE(node, "IntLiteralNode should not be NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, node->type, "Node type should be NODE_INT_LITERAL");
    IntLiteralNode *int_node = (IntLiteralNode *) node;
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected_value, int_node->value, "Integer literal value mismatch");
}

// Helper function to verify a VarDeclNode
// For initializers, it now only checks for presence and general type.
// Detailed verification of the initializer's content is done by the test case.
static void verify_var_decl_node(AstNode *node, const char *expected_type_name, const char *expected_var_name, bool expect_initializer, NodeType expected_initializer_type_if_any) {
    TEST_ASSERT_NOT_NULL_MESSAGE(node, "VarDeclNode should not be NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_VAR_DECL, node->type, "Node type should be NODE_VAR_DECL");
    VarDeclNode *decl_node = (VarDeclNode *) node;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_type_name, decl_node->type_name, "Variable type name mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_var_name, decl_node->var_name, "Variable name mismatch");

    if (expect_initializer) {
        TEST_ASSERT_NOT_NULL_MESSAGE(decl_node->initializer, "Initializer should not be NULL but was expected");
        TEST_ASSERT_EQUAL_MESSAGE(expected_initializer_type_if_any, decl_node->initializer->type, "Initializer node type mismatch");
        // Detailed verification of initializer (e.g., its value or structure) is now done by the calling test case.
    } else {
        TEST_ASSERT_NULL_MESSAGE(decl_node->initializer, "Initializer should be NULL but was present");
    }
}

// Helper function to verify an assignment operation node (AssignmentExpNode)
// The left side is expected to be an IdentifierNode.
// The right side's general type is checked; detailed verification is done by the test case.
static void verify_assignment_node(AstNode *node, const char *expected_left_var_name, NodeType expected_right_node_type) {
    TEST_ASSERT_NOT_NULL_MESSAGE(node, "Assignment node is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_ASSIGNMENT_EXP, node->type, "Node is not an assignment expression.");

    AssignmentExpNode *assign_node = (AssignmentExpNode*) node;

    // Verify left side (target)
    TEST_ASSERT_NOT_NULL_MESSAGE(assign_node->target, "Assignment target (left side) is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_IDENTIFIER, assign_node->target->type, "Assignment target is not an identifier.");
    IdentifierNode *left_ident = (IdentifierNode*) assign_node->target;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_left_var_name, left_ident->name, "Assignment target variable name mismatch.");

    // Verify right side (value) type
    TEST_ASSERT_NOT_NULL_MESSAGE(assign_node->value, "Assignment value (right side) is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(expected_right_node_type, assign_node->value->type, "Assignment value node type mismatch.");
}

// Helper to verify a BinaryOpNode (for arithmetic/logical ops, not assignment)
// For simplicity, this version expects left/right to be IntLiteral or Identifier.
// More complex checks (e.g., nested binary ops) can be added.
static void verify_binary_op_node(AstNode *node, BinaryOperatorType expected_op,
                                  NodeType expected_left_type, const void *expected_left_val,
                                  NodeType expected_right_type, const void *expected_right_val) {
    TEST_ASSERT_NOT_NULL_MESSAGE(node, "BinaryOpNode should not be NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, node->type, "Node type should be NODE_BINARY_OP");
    BinaryOpNode *bin_op_node = (BinaryOpNode *) node;
    TEST_ASSERT_EQUAL_MESSAGE(expected_op, bin_op_node->op, "Binary operator type mismatch");

    TEST_ASSERT_NOT_NULL_MESSAGE(bin_op_node->left, "Left operand of binary op should not be NULL");
    TEST_ASSERT_EQUAL_MESSAGE(expected_left_type, bin_op_node->left->type, "Left operand node type mismatch");
    if (expected_left_type == NODE_INT_LITERAL) {
        verify_int_literal_node(bin_op_node->left, *(int*)expected_left_val);
    } else if (expected_left_type == NODE_IDENTIFIER) {
        verify_identifier_node(bin_op_node->left, (const char*)expected_left_val);
    } else {
        TEST_FAIL_MESSAGE("Unsupported left operand type in verify_binary_op_node helper");
    }

    TEST_ASSERT_NOT_NULL_MESSAGE(bin_op_node->right, "Right operand of binary op should not be NULL");
    TEST_ASSERT_EQUAL_MESSAGE(expected_right_type, bin_op_node->right->type, "Right operand node type mismatch");
    if (expected_right_type == NODE_INT_LITERAL) {
        verify_int_literal_node(bin_op_node->right, *(int*)expected_right_val);
    } else if (expected_right_type == NODE_IDENTIFIER) {
        verify_identifier_node(bin_op_node->right, (const char*)expected_right_val);
    } else {
        TEST_FAIL_MESSAGE("Unsupported right operand type in verify_binary_op_node helper");
    }
}


// --- Test Cases ---

void test_parse_simple_declaration_with_initializer(void) {
    const char *input = "int main(void) { int x = 10; return x; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    if (program == NULL && parser.error_message) {
        fprintf(stderr, "Parser error in test_parse_simple_declaration_with_initializer: %s\n", parser.error_message);
    }

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set.");
    
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_NOT_NULL_MESSAGE(func_def->body, "Function body is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BLOCK, func_def->body->base.type, "Function body is not a NODE_BLOCK");
    BlockNode *body_block = func_def->body;
    
    TEST_ASSERT_GREATER_OR_EQUAL_INT_MESSAGE(2, body_block->num_items, "Block should have at least 2 items (declaration and return)");

    // Verify VarDeclNode: int x = 10;
    AstNode* decl_item = body_block->items[0];
    verify_var_decl_node(decl_item, "int", "x", true, NODE_INT_LITERAL);
    // Detailed check of initializer
    VarDeclNode *decl_node = (VarDeclNode*) decl_item;
    verify_int_literal_node(decl_node->initializer, 10);

    // Verify ReturnStmtNode: return x;
    AstNode* return_item = body_block->items[1];
    TEST_ASSERT_NOT_NULL_MESSAGE(return_item, "Second item in block (return statement) is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, return_item->type, "Second item is not NODE_RETURN_STMT");
    ReturnStmtNode *return_stmt = (ReturnStmtNode*) return_item;
    verify_identifier_node(return_stmt->expression, "x");

    arena_destroy(&test_arena);
}

void test_parse_simple_assignment_statement(void) {
    const char *input = "int main(void) { int y; y = 25; return y; }";
    Arena test_arena = arena_create(2048); // Slightly larger arena just in case
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    if (program == NULL && parser.error_message) {
        fprintf(stderr, "Parser error in test_parse_simple_assignment_statement: %s\n", parser.error_message);
    }

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set.");
    
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_NOT_NULL_MESSAGE(func_def->body, "Function body is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BLOCK, func_def->body->base.type, "Function body is not a NODE_BLOCK");
    BlockNode *body_block = func_def->body;
    
    TEST_ASSERT_GREATER_OR_EQUAL_INT_MESSAGE(3, body_block->num_items, "Block should have at least 3 items (declaration, assignment, return)");

    // Verify VarDeclNode: int y;
    AstNode* decl_item = body_block->items[0];
    verify_var_decl_node(decl_item, "int", "y", false, NODE_INT_LITERAL);

    // Verify Assignment Statement: y = 25;
    // This is an expression statement, so the item itself is the AssignmentExpNode for assignment.
    AstNode* assign_stmt_item = body_block->items[1];
    verify_assignment_node(assign_stmt_item, "y", NODE_INT_LITERAL);
    // Detailed check of RHS
    AssignmentExpNode *assign_node = (AssignmentExpNode*) assign_stmt_item;
    verify_int_literal_node(assign_node->value, 25);

    // Verify ReturnStmtNode: return y;
    AstNode* return_item = body_block->items[2];
    TEST_ASSERT_NOT_NULL_MESSAGE(return_item, "Third item in block (return statement) is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, return_item->type, "Third item is not NODE_RETURN_STMT");
    ReturnStmtNode *return_stmt = (ReturnStmtNode*) return_item;
    verify_identifier_node(return_stmt->expression, "y");

    arena_destroy(&test_arena);
}

void test_parse_declaration_with_identifier_initializer(void) {
    const char *input = "int main(void) { int y = 10; int x = y; return x; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set.");
    
    FuncDefNode *func_def = program->function;
    BlockNode *body_block = func_def->body;
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block, "Function body block is NULL");
    TEST_ASSERT_GREATER_OR_EQUAL_INT_MESSAGE(3, body_block->num_items, "Block should have at least 3 items");

    // 1. Verify VarDeclNode: int y = 10;
    AstNode* decl_y_item = body_block->items[0];
    verify_var_decl_node(decl_y_item, "int", "y", true, NODE_INT_LITERAL);
    VarDeclNode *decl_y_node = (VarDeclNode*) decl_y_item;
    verify_int_literal_node(decl_y_node->initializer, 10);

    // 2. Verify VarDeclNode: int x = y;
    AstNode* decl_x_item = body_block->items[1];
    verify_var_decl_node(decl_x_item, "int", "x", true, NODE_IDENTIFIER);
    VarDeclNode *decl_x_node = (VarDeclNode*) decl_x_item;
    verify_identifier_node(decl_x_node->initializer, "y");
    
    // 3. Verify ReturnStmtNode: return x;
    AstNode* return_item = body_block->items[2];
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, return_item->type, "Third item is not NODE_RETURN_STMT");
    ReturnStmtNode *return_stmt = (ReturnStmtNode*) return_item;
    verify_identifier_node(return_stmt->expression, "x");

    arena_destroy(&test_arena);
}

void test_parse_assignment_with_identifier_rhs(void) {
    const char *input = "int main(void) { int y; int x; y = 10; x = y; return x; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set.");
    
    FuncDefNode *func_def = program->function;
    BlockNode *body_block = func_def->body;
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block, "Function body block is NULL");
    TEST_ASSERT_GREATER_OR_EQUAL_INT_MESSAGE(4, body_block->num_items, "Block should have at least 4 items");

    // 1. Verify VarDeclNode: int y;
    AstNode* decl_y_item = body_block->items[0];
    verify_var_decl_node(decl_y_item, "int", "y", false, NODE_INT_LITERAL);

    // 2. Verify VarDeclNode: int x;
    AstNode* decl_x_item = body_block->items[1];
    verify_var_decl_node(decl_x_item, "int", "x", false, NODE_INT_LITERAL);

    // 3. Verify Assignment Statement: y = 10;
    AstNode* assign_y_item = body_block->items[2];
    verify_assignment_node(assign_y_item, "y", NODE_INT_LITERAL);
    BinaryOpNode *assign_y_node = (BinaryOpNode*) assign_y_item;
    verify_int_literal_node(assign_y_node->right, 10);

    // 4. Verify Assignment Statement: x = y;
    AstNode* assign_x_item = body_block->items[3];
    verify_assignment_node(assign_x_item, "x", NODE_IDENTIFIER);
    BinaryOpNode *assign_x_node = (BinaryOpNode*) assign_x_item;
    verify_identifier_node(assign_x_node->right, "y");
    
    // 5. Verify ReturnStmtNode: return x;
    AstNode* return_item = body_block->items[4];
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, return_item->type, "Fifth item is not NODE_RETURN_STMT");
    ReturnStmtNode *return_stmt = (ReturnStmtNode*) return_item;
    verify_identifier_node(return_stmt->expression, "x");

    arena_destroy(&test_arena);
}

void test_parse_declaration_with_binary_op_initializer(void) {
    const char *input = "int main(void) { int x = 10 + 5; return x; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set.");
    
    FuncDefNode *func_def = program->function;
    BlockNode *body_block = func_def->body;
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block, "Function body block is NULL");
    TEST_ASSERT_GREATER_OR_EQUAL_INT_MESSAGE(2, body_block->num_items, "Block should have at least 2 items");

    // 1. Verify VarDeclNode: int x = 10 + 5;
    AstNode* decl_x_item = body_block->items[0];
    verify_var_decl_node(decl_x_item, "int", "x", true, NODE_BINARY_OP);
    VarDeclNode *decl_x_node = (VarDeclNode*) decl_x_item;
    
    // Detailed check of the binary op initializer: 10 + 5
    // Need to cast void* for literals.
    int val_10 = 10;
    int val_5 = 5;
    verify_binary_op_node(decl_x_node->initializer, OPERATOR_ADD,
                          NODE_INT_LITERAL, &val_10,
                          NODE_INT_LITERAL, &val_5);
    
    // 2. Verify ReturnStmtNode: return x;
    AstNode* return_item = body_block->items[1];
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, return_item->type, "Second item is not NODE_RETURN_STMT");
    ReturnStmtNode *return_stmt = (ReturnStmtNode*) return_item;
    verify_identifier_node(return_stmt->expression, "x");

    arena_destroy(&test_arena);
}

void test_parse_assignment_with_binary_op_rhs(void) {
    const char *input = "int main(void) { int x; x = 10 + 5; return x; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "parse_program() returned NULL.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message ? parser.error_message : "Parser error flag set.");
    
    FuncDefNode *func_def = program->function;
    BlockNode *body_block = func_def->body;
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block, "Function body block is NULL");
    TEST_ASSERT_GREATER_OR_EQUAL_INT_MESSAGE(3, body_block->num_items, "Block should have at least 3 items");

    // 1. Verify VarDeclNode: int x;
    AstNode* decl_x_item = body_block->items[0];
    verify_var_decl_node(decl_x_item, "int", "x", false, NODE_INT_LITERAL);

    // 2. Verify Assignment Statement: x = 10 + 5;
    AstNode* assign_x_item = body_block->items[1];
    verify_assignment_node(assign_x_item, "x", NODE_BINARY_OP);
    BinaryOpNode *assign_x_node = (BinaryOpNode*) assign_x_item;

    // Detailed check of the binary op RHS: 10 + 5
    int val_10 = 10;
    int val_5 = 5;
    verify_binary_op_node(assign_x_node->right, OPERATOR_ADD,
                          NODE_INT_LITERAL, &val_10,
                          NODE_INT_LITERAL, &val_5);
    
    // 3. Verify ReturnStmtNode: return x;
    AstNode* return_item = body_block->items[2];
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, return_item->type, "Third item is not NODE_RETURN_STMT");
    ReturnStmtNode *return_stmt = (ReturnStmtNode*) return_item;
    verify_identifier_node(return_stmt->expression, "x");

    arena_destroy(&test_arena);
}


// --- Test Runner ---

void run_parser_assignments_tests(void) {
    RUN_TEST(test_parse_simple_declaration_with_initializer);
    RUN_TEST(test_parse_simple_assignment_statement);
    RUN_TEST(test_parse_declaration_with_identifier_initializer);
    RUN_TEST(test_parse_assignment_with_identifier_rhs);
    RUN_TEST(test_parse_declaration_with_binary_op_initializer);
    RUN_TEST(test_parse_assignment_with_binary_op_rhs);
    // Add more tests here
}
