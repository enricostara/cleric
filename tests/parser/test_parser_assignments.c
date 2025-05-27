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
// For initializers, it currently supports direct check for IntLiteralNode value.
static void verify_var_decl_node(AstNode *node, const char *expected_type_name, const char *expected_var_name, bool expect_initializer, NodeType expected_initializer_type_if_any, int expected_initializer_int_val_if_literal) {
    TEST_ASSERT_NOT_NULL_MESSAGE(node, "VarDeclNode should not be NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_VAR_DECL, node->type, "Node type should be NODE_VAR_DECL");
    VarDeclNode *decl_node = (VarDeclNode *) node;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_type_name, decl_node->type_name, "Variable type name mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_var_name, decl_node->var_name, "Variable name mismatch");

    if (expect_initializer) {
        TEST_ASSERT_NOT_NULL_MESSAGE(decl_node->initializer, "Initializer should not be NULL but was expected");
        TEST_ASSERT_EQUAL_MESSAGE(expected_initializer_type_if_any, decl_node->initializer->type, "Initializer node type mismatch");
        if (expected_initializer_type_if_any == NODE_INT_LITERAL) {
            verify_int_literal_node(decl_node->initializer, expected_initializer_int_val_if_literal);
        }
        // TODO: Add checks for other initializer types (e.g., binary op, identifier) if needed by future tests
    } else {
        TEST_ASSERT_NULL_MESSAGE(decl_node->initializer, "Initializer should be NULL but was present");
    }
}

// Helper function to verify an assignment operation node (BinaryOpNode with OPERATOR_ASSIGN)
// The left side is expected to be an IdentifierNode.
// The right side can be any expression node. If it's an IntLiteral, its value is checked.
static void verify_assignment_node(AstNode *node, const char *expected_left_var_name, NodeType expected_right_node_type, int expected_right_int_val_if_literal) {
    TEST_ASSERT_NOT_NULL_MESSAGE(node, "Assignment Node (BinaryOpNode) should not be NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, node->type, "Node type should be NODE_BINARY_OP for assignment");

    BinaryOpNode *assign_node = (BinaryOpNode *) node;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_ASSIGN, assign_node->op, "Binary operator type should be OPERATOR_ASSIGN");

    // Verify left side (identifier)
    verify_identifier_node(assign_node->left, expected_left_var_name);

    // Verify right side
    TEST_ASSERT_NOT_NULL_MESSAGE(assign_node->right, "Right operand of assignment should not be NULL");
    TEST_ASSERT_EQUAL_MESSAGE(expected_right_node_type, assign_node->right->type, "Right operand node type mismatch for assignment");

    if (expected_right_node_type == NODE_INT_LITERAL) {
        verify_int_literal_node(assign_node->right, expected_right_int_val_if_literal);
    }
    // TODO: Add checks for other right-hand side types (e.g., identifier, another binary op)
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
    verify_var_decl_node(decl_item, "int", "x", true, NODE_INT_LITERAL, 10);

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
    // For NODE_INT_LITERAL (or any valid NodeType), the int val doesn't matter when expect_initializer is false.
    verify_var_decl_node(decl_item, "int", "y", false, NODE_INT_LITERAL, 0); 

    // Verify Assignment Statement: y = 25;
    // This is an expression statement, so the item itself is the BinaryOpNode for assignment.
    AstNode* assign_stmt_item = body_block->items[1];
    verify_assignment_node(assign_stmt_item, "y", NODE_INT_LITERAL, 25);

    // Verify ReturnStmtNode: return y;
    AstNode* return_item = body_block->items[2];
    TEST_ASSERT_NOT_NULL_MESSAGE(return_item, "Third item in block (return statement) is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, return_item->type, "Third item is not NODE_RETURN_STMT");
    ReturnStmtNode *return_stmt = (ReturnStmtNode*) return_item;
    verify_identifier_node(return_stmt->expression, "y");

    arena_destroy(&test_arena);
}


// --- Test Runner ---

void run_parser_assignments_tests(void) {
    RUN_TEST(test_parse_simple_declaration_with_initializer);
    RUN_TEST(test_parse_simple_assignment_statement);
    // Add more tests here
}
