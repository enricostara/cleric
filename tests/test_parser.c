#include <string.h>

#include "unity.h"
#include "../src/lexer/lexer.h" // Include Lexer for tokenization
#include "../src/parser/parser.h" // Include Parser for parsing
#include "../src/parser/ast.h"    // Include AST for structure checks
#include "../src/memory/arena.h" // Include Arena for parser

// --- Test Helper Function ---

// Helper function to verify a unary operation node
static void verify_unary_op_node(AstNode *node, UnaryOperatorType expected_op, int expected_value) {
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, node->type);

    UnaryOpNode *unary_node = (UnaryOpNode *) node;
    TEST_ASSERT_EQUAL(expected_op, unary_node->op);
    TEST_ASSERT_NOT_NULL(unary_node->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, unary_node->operand->type);

    IntLiteralNode *int_node = (IntLiteralNode *) unary_node->operand;
    TEST_ASSERT_EQUAL_INT(expected_value, int_node->value);
}

// Helper function to verify a nested unary operation
static void verify_nested_unary_op(AstNode *node, UnaryOperatorType outer_op, UnaryOperatorType inner_op, int value) {
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, node->type);

    UnaryOpNode *outer_node = (UnaryOpNode *) node;
    TEST_ASSERT_EQUAL(outer_op, outer_node->op);
    TEST_ASSERT_NOT_NULL(outer_node->operand);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, outer_node->operand->type);

    UnaryOpNode *inner_node = (UnaryOpNode *) outer_node->operand;
    TEST_ASSERT_EQUAL(inner_op, inner_node->op);
    TEST_ASSERT_NOT_NULL(inner_node->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, inner_node->operand->type);

    IntLiteralNode *int_node = (IntLiteralNode *) inner_node->operand;
    TEST_ASSERT_EQUAL_INT(value, int_node->value);
}

// Helper function to verify a binary operation node where both operands are int literals
static void verify_binary_op_node(AstNode *node, BinaryOperatorType expected_op, int expected_left_val,
                                  int expected_right_val) {
    TEST_ASSERT_NOT_NULL_MESSAGE(node, "BinaryOpNode is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, node->type, "Node type is not NODE_BINARY_OP");

    BinaryOpNode *bin_node = (BinaryOpNode *) node;
    TEST_ASSERT_EQUAL_MESSAGE(expected_op, bin_node->op, "Binary operator type mismatch");

    TEST_ASSERT_NOT_NULL_MESSAGE(bin_node->left, "Left operand is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, bin_node->left->type, "Left operand is not NODE_INT_LITERAL");
    IntLiteralNode *left_int_node = (IntLiteralNode *) bin_node->left;
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected_left_val, left_int_node->value, "Left operand value mismatch");

    TEST_ASSERT_NOT_NULL_MESSAGE(bin_node->right, "Right operand is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, bin_node->right->type, "Right operand is not NODE_INT_LITERAL");
    IntLiteralNode *right_int_node = (IntLiteralNode *) bin_node->right;
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected_right_val, right_int_node->value, "Right operand value mismatch");
}

// --- Helper for Error Tests ---
void verify_parser_error(const char *input, const char *expected_error_substring) {
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser); // This will trigger the parsing

    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser did not report an error as expected.");
    if (expected_error_substring) {
        TEST_ASSERT_NOT_NULL(parser.error_message); // This is where it was failing
        TEST_ASSERT_NOT_NULL_MESSAGE(strstr(parser.error_message, expected_error_substring),
                                     "Error message mismatch or expected substring not found.");
    }
    // ProgramNode might be NULL or partially formed, which is okay for error tests
    arena_destroy(&test_arena);
}

// --- Test Cases ---

// Test parsing a simple valid program: "int main(void) { return 42; }"
void test_parse_valid_program(void) {
    const char *input = "int main(void) { return 42; }";
    // Assert parsing succeeded without error flag
    // TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input");
    // We need access to the parser state after parse_program to check the flag.
    // Let's modify parse_test_input or create the parser outside for this check.

    // Reworking the test slightly to check the flag:
    Lexer lexer;
    Arena test_arena = arena_create(1024); // Create arena for this test
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena

    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program_reworked = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program_reworked, "Parser returned NULL for valid input");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input");

    // --- AST Structure Verification ---
    // Program -> FuncDef
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program_reworked->base.type);
    TEST_ASSERT_NOT_NULL(program_reworked->function);
    FuncDefNode *func_def = program_reworked->function;

    // FuncDef -> ReturnStmt
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);
    AstNode *stmt = func_def->body;
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, stmt->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) stmt;

    // ReturnStmt -> IntLiteral
    TEST_ASSERT_NOT_NULL(return_stmt->expression);
    AstNode *expr = return_stmt->expression;
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, expr->type);
    IntLiteralNode *int_literal = (IntLiteralNode *) expr;

    // Check the integer value
    TEST_ASSERT_EQUAL_INT(42, int_literal->value);

    // Clean up the arena (assuming parse_program used it for allocations)
    arena_destroy(&test_arena);
    // free_ast((AstNode *) program_reworked); // Assuming arena handles cleanup
}

// Test parsing an invalid program (missing semicolon)
void test_parse_missing_semicolon(void) {
    const char *input = "int main(void) { return 42 }"; // Missing semicolon
    Lexer lexer;
    Arena test_arena = arena_create(1024); // Create arena for this test
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena

    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for missing semicolon");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for missing semicolon");

    // Clean up the arena
    arena_destroy(&test_arena);
}

// Test parsing an invalid program (missing closing brace)
void test_parse_missing_brace(void) {
    const char *input = "int main(void) { return 42;"; // Missing brace
    Lexer lexer;
    Arena test_arena = arena_create(1024); // Create arena for this test
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena

    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for missing brace");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for missing brace");

    // Clean up the arena
    arena_destroy(&test_arena);
}

// Test parsing a function with an empty body: "int main(void) {}"
void test_parse_function_empty_body(void) {
    const char *input = "int main(void) {}";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input with empty function body");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input with empty function body");

    // --- AST Structure Verification ---
    // Program -> FuncDef
    TEST_ASSERT_EQUAL_MESSAGE(NODE_PROGRAM, program->base.type, "Program node type mismatch");
    TEST_ASSERT_NOT_NULL_MESSAGE(program->function, "ProgramNode has no function");
    FuncDefNode *func_def = program->function;

    // FuncDef name and body
    TEST_ASSERT_EQUAL_MESSAGE(NODE_FUNC_DEF, func_def->base.type, "Function definition node type mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("main", func_def->name, "Function name mismatch");

    // Crucial check: The body of an empty function should be NULL
    TEST_ASSERT_NULL_MESSAGE(func_def->body, "Function body is not NULL for an empty function");

    arena_destroy(&test_arena);
}

// Test parsing a program with a negation unary operator
void test_parse_negation_operator(void) {
    const char *input = "int main(void) { return -42; }";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    Lexer lexer;
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input with negation");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input with negation");

    // Verify program structure
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program->base.type);
    TEST_ASSERT_NOT_NULL(program->function);

    // Verify function structure
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);

    // Verify return statement
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_def->body->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);

    // Verify unary operation
    verify_unary_op_node(return_stmt->expression, OPERATOR_NEGATE, 42);

    arena_destroy(&test_arena);
}

// Test parsing a program with a bitwise complement unary operator
void test_parse_complement_operator(void) {
    const char *input = "int main(void) { return ~42; }";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    Lexer lexer;
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input with complement");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input with complement");

    // Verify program structure
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program->base.type);
    TEST_ASSERT_NOT_NULL(program->function);

    // Verify function structure
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);

    // Verify return statement
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_def->body->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);

    // Verify unary operation
    verify_unary_op_node(return_stmt->expression, OPERATOR_COMPLEMENT, 42);

    arena_destroy(&test_arena);
}

// Test parsing a program with a logical NOT unary operator
void test_parse_logical_not_operator(void) {
    const char *input = "int main(void) { return !0; }";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    Lexer lexer;
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input with logical NOT");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input with logical NOT");

    // Verify program structure
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program->base.type);
    TEST_ASSERT_NOT_NULL(program->function);

    // Verify function structure
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);

    // Verify return statement
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_def->body->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);

    // Verify unary operation
    verify_unary_op_node(return_stmt->expression, OPERATOR_LOGICAL_NOT, 0);

    arena_destroy(&test_arena);
}

// Test parsing a program with nested unary operators
void test_parse_nested_unary_operators(void) {
    const char *input = "int main(void) { return -~42; }";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    Lexer lexer;
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input with nested unary ops");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input with nested unary ops");

    // Verify program structure
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program->base.type);
    TEST_ASSERT_NOT_NULL(program->function);

    // Verify function structure
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);

    // Verify return statement
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_def->body->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);

    // Verify nested unary operations
    verify_nested_unary_op(return_stmt->expression, OPERATOR_NEGATE, OPERATOR_COMPLEMENT, 42);

    arena_destroy(&test_arena);
}

// Test parsing a program with parenthesized expression
void test_parse_parenthesized_expression(void) {
    const char *input = "int main(void) { return (42); }";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    Lexer lexer;
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input with parentheses");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input with parentheses");

    // Verify program structure
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program->base.type);
    TEST_ASSERT_NOT_NULL(program->function);

    // Verify function structure
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);

    // Verify return statement
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_def->body->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);

    // Verify integer literal (parentheses don't create a node, just group)
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, return_stmt->expression->type);
    IntLiteralNode *int_literal = (IntLiteralNode *) return_stmt->expression;
    TEST_ASSERT_EQUAL_INT(42, int_literal->value);

    arena_destroy(&test_arena);
}

// Test parsing a program with unary operator and parentheses
void test_parse_unary_with_parentheses(void) {
    const char *input = "int main(void) { return -(42); }";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    Lexer lexer;
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input with unary op and parentheses");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag,
                              "Parser error flag was set for valid input with unary op and parentheses");

    // Verify program structure
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program->base.type);
    TEST_ASSERT_NOT_NULL(program->function);

    // Verify function structure
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);

    // Verify return statement
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_def->body->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);

    // Verify unary operation
    verify_unary_op_node(return_stmt->expression, OPERATOR_NEGATE, 42);

    arena_destroy(&test_arena);
}

// Test parsing a program with complex nested expressions
void test_parse_complex_nested_expression(void) {
    const char *input = "int main(void) { return ~(-(~42)); }";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    Lexer lexer;
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input with complex nested expression");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag,
                              "Parser error flag was set for valid input with complex nested expression");

    // Verify program structure
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program->base.type);
    TEST_ASSERT_NOT_NULL(program->function);

    // Verify function structure
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);

    // Verify return statement
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_def->body->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);

    // Verify outer complement
    AstNode *expr = return_stmt->expression;
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, expr->type);
    UnaryOpNode *outer_complement = (UnaryOpNode *) expr;
    TEST_ASSERT_EQUAL(OPERATOR_COMPLEMENT, outer_complement->op);

    // Verify middle negation
    TEST_ASSERT_NOT_NULL(outer_complement->operand);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, outer_complement->operand->type);
    UnaryOpNode *middle_negate = (UnaryOpNode *) outer_complement->operand;
    TEST_ASSERT_EQUAL(OPERATOR_NEGATE, middle_negate->op);

    // Verify inner complement
    TEST_ASSERT_NOT_NULL(middle_negate->operand);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, middle_negate->operand->type);
    UnaryOpNode *inner_complement = (UnaryOpNode *) middle_negate->operand;
    TEST_ASSERT_EQUAL(OPERATOR_COMPLEMENT, inner_complement->op);

    // Verify integer literal
    TEST_ASSERT_NOT_NULL(inner_complement->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, inner_complement->operand->type);
    IntLiteralNode *int_literal = (IntLiteralNode *) inner_complement->operand;
    TEST_ASSERT_EQUAL_INT(42, int_literal->value);

    arena_destroy(&test_arena);
}

// Test parsing an invalid unary expression (missing operand)
void test_parse_invalid_unary_expression(void) {
    const char *input = "int main(void) { return -; }"; // Missing operand
    Lexer lexer;
    Arena test_arena = arena_create(1024); // Create arena
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for invalid unary expression");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for invalid unary expression");

    arena_destroy(&test_arena);
}

// Test parsing a program with mismatched parentheses
void test_parse_mismatched_parentheses(void) {
    const char *input = "int main(void) { return (42; }"; // Mismatched parenthesis
    Lexer lexer;
    Arena test_arena = arena_create(1024); // Create arena
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for mismatched parentheses");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for mismatched parentheses");

    arena_destroy(&test_arena);
}

// Test parsing an integer at the boundary of what's representable
void test_parse_integer_bounds(void) {
    const char *input = "int main(void) { return 2147483647; }"; // INT_MAX for 32-bit int
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input with INT_MAX");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input with INT_MAX");

    // Verify program structure
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program->base.type);
    TEST_ASSERT_NOT_NULL(program->function);

    // Verify function structure
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);

    // Verify return statement
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_def->body->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);

    // Verify integer literal
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, return_stmt->expression->type);
    const IntLiteralNode *int_literal = (IntLiteralNode *) return_stmt->expression;
    TEST_ASSERT_EQUAL_INT(2147483647, int_literal->value); // INT_MAX

    arena_destroy(&test_arena);
}

// Test parsing an integer exceeding the representable range
void test_parse_integer_overflow(void) {
    const char *input = "int main(void) { return 2147483648; }"; // One more than INT_MAX
    Lexer lexer;
    Arena test_arena = arena_create(1024); // Create arena
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for integer overflow");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for integer overflow");

    arena_destroy(&test_arena);
}

// --- Binary Operation Tests ---
void test_parse_simple_addition(void) {
    const char *input = "int main(void) { return 1 + 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for simple addition");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for simple addition");

    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    TEST_ASSERT_NOT_NULL_MESSAGE(return_stmt, "Return statement is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_RETURN_STMT, return_stmt->base.type, "Body is not a return statement");

    verify_binary_op_node(return_stmt->expression, OPERATOR_ADD, 1, 2);

    arena_destroy(&test_arena);
}

void test_parse_simple_subtraction(void) {
    const char *input = "int main(void) { return 5 - 3; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_SUBTRACT, 5, 3);
    arena_destroy(&test_arena);
}

void test_parse_simple_multiplication(void) {
    const char *input = "int main(void) { return 4 * 6; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_MULTIPLY, 4, 6);
    arena_destroy(&test_arena);
}

void test_parse_simple_division(void) {
    const char *input = "int main(void) { return 10 / 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_DIVIDE, 10, 2);
    arena_destroy(&test_arena);
}

void test_parse_simple_modulo(void) {
    const char *input = "int main(void) { return 7 % 3; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(return_stmt->expression, OPERATOR_MODULO, 7, 3);
    arena_destroy(&test_arena);
}

void test_parse_precedence_add_mul(void) {
    // AST should be: (1 + (2 * 3)) ->  ADD(1, MUL(2,3))
    const char *input = "int main(void) { return 1 + 2 * 3; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    AstNode *expr_node = return_stmt->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr_node->type);
    BinaryOpNode *add_op = (BinaryOpNode *) expr_node;
    TEST_ASSERT_EQUAL(OPERATOR_ADD, add_op->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, add_op->left->type);
    TEST_ASSERT_EQUAL_INT(1, ((IntLiteralNode*)add_op->left)->value);

    // Right child of ADD should be MUL(2,3)
    verify_binary_op_node(add_op->right, OPERATOR_MULTIPLY, 2, 3);
    arena_destroy(&test_arena);
}

void test_parse_precedence_mul_add(void) {
    // AST should be: ((1 * 2) + 3) -> ADD(MUL(1,2), 3)
    const char *input = "int main(void) { return 1 * 2 + 3; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    AstNode *expr_node = return_stmt->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr_node->type);
    BinaryOpNode *add_op = (BinaryOpNode *) expr_node;
    TEST_ASSERT_EQUAL(OPERATOR_ADD, add_op->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, add_op->right->type);
    TEST_ASSERT_EQUAL_INT(3, ((IntLiteralNode*)add_op->right)->value);

    // Left child of ADD should be MUL(1,2)
    verify_binary_op_node(add_op->left, OPERATOR_MULTIPLY, 1, 2);
    arena_destroy(&test_arena);
}

void test_parse_associativity_subtract(void) {
    // AST should be: ((10 - 3) - 2) -> SUBTRACT(SUBTRACT(10,3), 2)
    const char *input = "int main(void) { return 10 - 3 - 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    AstNode *expr_node = return_stmt->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr_node->type);
    BinaryOpNode *outer_sub_op = (BinaryOpNode *) expr_node;
    TEST_ASSERT_EQUAL(OPERATOR_SUBTRACT, outer_sub_op->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, outer_sub_op->right->type);
    TEST_ASSERT_EQUAL_INT(2, ((IntLiteralNode*)outer_sub_op->right)->value);

    // Left child of outer SUBTRACT should be SUBTRACT(10,3)
    verify_binary_op_node(outer_sub_op->left, OPERATOR_SUBTRACT, 10, 3);
    arena_destroy(&test_arena);
}

void test_parse_associativity_divide(void) {
    // AST should be: ((100 / 10) / 2) -> DIVIDE(DIVIDE(100,10), 2)
    const char *input = "int main(void) { return 100 / 10 / 2; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    AstNode *expr_node = return_stmt->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr_node->type);
    BinaryOpNode *outer_div_op = (BinaryOpNode *) expr_node;
    TEST_ASSERT_EQUAL(OPERATOR_DIVIDE, outer_div_op->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, outer_div_op->right->type);
    TEST_ASSERT_EQUAL_INT(2, ((IntLiteralNode*)outer_div_op->right)->value);

    // Left child of outer DIVIDE should be DIVIDE(100,10)
    verify_binary_op_node(outer_div_op->left, OPERATOR_DIVIDE, 100, 10);
    arena_destroy(&test_arena);
}

void test_parse_parentheses_simple(void) {
    // AST should be: ((1 + 2) * 3) -> MULTIPLY(ADD(1,2), 3)
    const char *input = "int main(void) { return (1 + 2) * 3; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    AstNode *expr_node = return_stmt->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr_node->type);
    BinaryOpNode *mul_op = (BinaryOpNode *) expr_node;
    TEST_ASSERT_EQUAL(OPERATOR_MULTIPLY, mul_op->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, mul_op->right->type);
    TEST_ASSERT_EQUAL_INT(3, ((IntLiteralNode*)mul_op->right)->value);

    // Left child of MULTIPLY should be ADD(1,2)
    verify_binary_op_node(mul_op->left, OPERATOR_ADD, 1, 2);
    arena_destroy(&test_arena);
}

void test_parse_parentheses_nested(void) {
    // AST: 1 * (2 + 3) % 4 -> MODULO(MULTIPLY(1, ADD(2,3)), 4)
    const char *input = "int main(void) { return 1 * (2 + 3) % 4; }";
    Arena test_arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &test_arena);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.error_flag);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) program->function->body;
    AstNode *expr_node = return_stmt->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr_node->type); // MODULO is root
    BinaryOpNode *mod_op = (BinaryOpNode *) expr_node;
    TEST_ASSERT_EQUAL(OPERATOR_MODULO, mod_op->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, mod_op->right->type);
    TEST_ASSERT_EQUAL_INT(4, ((IntLiteralNode*)mod_op->right)->value);

    // Left child of MODULO is MULTIPLY(1, ADD(2,3))
    AstNode *mul_node = mod_op->left;
    TEST_ASSERT_EQUAL(NODE_BINARY_OP, mul_node->type);
    BinaryOpNode *mul_op = (BinaryOpNode *) mul_node;
    TEST_ASSERT_EQUAL(OPERATOR_MULTIPLY, mul_op->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, mul_op->left->type);
    TEST_ASSERT_EQUAL_INT(1, ((IntLiteralNode*)mul_op->left)->value);

    // Right child of MULTIPLY is ADD(2,3)
    verify_binary_op_node(mul_op->right, OPERATOR_ADD, 2, 3);
    arena_destroy(&test_arena);
}

void test_parse_unary_with_binary_simple(void) {
    // Test -1 + 5 -> ADD(NEGATE(1), 5)
    const char *input1 = "int main(void) { return -1 + 5; }";
    Arena arena1 = arena_create(1024);
    Lexer lexer1;
    lexer_init(&lexer1, input1, &arena1);
    Parser parser1;
    parser_init(&parser1, &lexer1, &arena1);
    ProgramNode *program1 = parse_program(&parser1);
    TEST_ASSERT_NOT_NULL(program1);
    TEST_ASSERT_FALSE(parser1.error_flag);
    ReturnStmtNode *return_stmt1 = (ReturnStmtNode *) program1->function->body;
    AstNode *expr1 = return_stmt1->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr1->type);
    BinaryOpNode *add_op1 = (BinaryOpNode *) expr1;
    TEST_ASSERT_EQUAL(OPERATOR_ADD, add_op1->op);
    verify_unary_op_node(add_op1->left, OPERATOR_NEGATE, 1);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, add_op1->right->type);
    TEST_ASSERT_EQUAL_INT(5, ((IntLiteralNode*)add_op1->right)->value);
    arena_destroy(&arena1);

    // Test 10 * -2 -> MULTIPLY(10, NEGATE(2))
    const char *input2 = "int main(void) { return 10 * -2; }";
    Arena arena2 = arena_create(1024);
    Lexer lexer2;
    lexer_init(&lexer2, input2, &arena2);
    Parser parser2;
    parser_init(&parser2, &lexer2, &arena2);
    ProgramNode *program2 = parse_program(&parser2);
    TEST_ASSERT_NOT_NULL(program2);
    TEST_ASSERT_FALSE(parser2.error_flag);
    ReturnStmtNode *return_stmt2 = (ReturnStmtNode *) program2->function->body;
    AstNode *expr2 = return_stmt2->expression;

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, expr2->type);
    BinaryOpNode *mul_op2 = (BinaryOpNode *) expr2;
    TEST_ASSERT_EQUAL(OPERATOR_MULTIPLY, mul_op2->op);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, mul_op2->left->type);
    TEST_ASSERT_EQUAL_INT(10, ((IntLiteralNode*)mul_op2->left)->value);
    verify_unary_op_node(mul_op2->right, OPERATOR_NEGATE, 2);
    arena_destroy(&arena2);
}

void test_parse_unary_on_parenthesized_expr(void) {
    const char *input = "int main(void) { return -(2 + 3); }"; // Equivalent to return -5;
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    Lexer lexer;
    lexer_init(&lexer, input, &test_arena); // Init lexer with arena
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input");

    // Program -> FuncDef -> ReturnStmt -> UnaryOp (-) -> BinaryOp (+)
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program->base.type);
    TEST_ASSERT_NOT_NULL(program->function);
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_def->body->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);

    AstNode *unary_expr = return_stmt->expression;
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, unary_expr->type);
    UnaryOpNode *unary_node = (UnaryOpNode *) unary_expr;
    TEST_ASSERT_EQUAL(OPERATOR_NEGATE, unary_node->op);

    verify_binary_op_node(unary_node->operand, OPERATOR_ADD, 2, 3);

    arena_destroy(&test_arena);
}

// --- Relational and Logical Operator Tests ---

void test_parse_relational_less_than(void) {
    const char *input = "int main(void) { return 1 < 2; }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode is NULL for '<'");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error for '<'");
    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(ret->expression, OPERATOR_LESS, 1, 2);
    arena_destroy(&arena);
}

void test_parse_relational_greater_than(void) {
    const char *input = "int main(void) { return 2 > 1; }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode is NULL for '>'");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error for '>'");
    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(ret->expression, OPERATOR_GREATER, 2, 1);
    arena_destroy(&arena);
}

void test_parse_relational_less_equal(void) {
    const char *input = "int main(void) { return 1 <= 2; }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode is NULL for '<='");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error for '<='");
    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(ret->expression, OPERATOR_LESS_EQUAL, 1, 2);
    arena_destroy(&arena);
}

void test_parse_relational_greater_equal(void) {
    const char *input = "int main(void) { return 2 >= 1; }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode is NULL for '>='");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error for '>='");
    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(ret->expression, OPERATOR_GREATER_EQUAL, 2, 1);
    arena_destroy(&arena);
}

void test_parse_relational_equal_equal(void) {
    const char *input = "int main(void) { return 1 == 1; }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode is NULL for '=='");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error for '=='");
    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(ret->expression, OPERATOR_EQUAL_EQUAL, 1, 1);
    arena_destroy(&arena);
}

void test_parse_relational_not_equal(void) {
    const char *input = "int main(void) { return 1 != 2; }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode is NULL for '!='");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error for '!='");
    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(ret->expression, OPERATOR_NOT_EQUAL, 1, 2);
    arena_destroy(&arena);
}

void test_parse_logical_and(void) {
    const char *input = "int main(void) { return 1 && 0; }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode is NULL for '&&'");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error for '&&'");
    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(ret->expression, OPERATOR_LOGICAL_AND, 1, 0);
    arena_destroy(&arena);
}

void test_parse_logical_or(void) {
    const char *input = "int main(void) { return 0 || 1; }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode is NULL for '||'");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error for '||'");
    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    verify_binary_op_node(ret->expression, OPERATOR_LOGICAL_OR, 0, 1);
    arena_destroy(&arena);
}

// --- Complex Precedence and Associativity Tests ---

// Test: 1 || 0 && 1  =>  1 || (0 && 1)
void test_precedence_logical_or_and(void) {
    const char *input = "int main(void) { return 1 || 0 && 1; }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode NULL");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error");

    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    AstNode *expr = ret->expression;
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, expr->type, "Root not BINARY_OP");

    BinaryOpNode *or_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_LOGICAL_OR, or_node->op, "Root op not ||");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, or_node->left->type, "OR left not INT_LITERAL");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ((IntLiteralNode *)or_node->left)->value, "OR left value mismatch");

    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, or_node->right->type, "OR right not BINARY_OP");
    BinaryOpNode *and_node = (BinaryOpNode *) or_node->right;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_LOGICAL_AND, and_node->op, "Nested op not &&");
    verify_binary_op_node((AstNode *) and_node, OPERATOR_LOGICAL_AND, 0, 1); // Verifies children of AND

    arena_destroy(&arena);
}

// Test: 1 < 2 && 3 > 1  =>  (1 < 2) && (3 > 1)
void test_precedence_relational_and_logical(void) {
    const char *input = "int main(void) { return 1 < 2 && 3 > 1; }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode NULL");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error");

    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    AstNode *expr = ret->expression;
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, expr->type, "Root not BINARY_OP");

    BinaryOpNode *and_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_LOGICAL_AND, and_node->op, "Root op not &&");

    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, and_node->left->type, "&& left not BINARY_OP");
    verify_binary_op_node(and_node->left, OPERATOR_LESS, 1, 2);

    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, and_node->right->type, "&& right not BINARY_OP");
    verify_binary_op_node(and_node->right, OPERATOR_GREATER, 3, 1);

    arena_destroy(&arena);
}

// Test: 1 + 2 < 4 && 5 > 3 - 1  =>  ((1 + 2) < 4) && (5 > (3 - 1))
void test_precedence_arithmetic_relational_logical(void) {
    const char *input = "int main(void) { return 1 + 2 < 4 && 5 > 3 - 1; }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode NULL");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error");

    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    AstNode *expr = ret->expression;
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, expr->type, "Root not BINARY_OP");

    BinaryOpNode *and_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_LOGICAL_AND, and_node->op, "Root op not &&");

    // Left side of &&: (1 + 2) < 4
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, and_node->left->type, "AND left not BINARY_OP");
    BinaryOpNode *less_node = (BinaryOpNode *) and_node->left;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_LESS, less_node->op, "Op not <");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, less_node->left->type, "< left not BINARY_OP");
    verify_binary_op_node(less_node->left, OPERATOR_ADD, 1, 2);
    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, less_node->right->type, "< right not INT_LITERAL");
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, ((IntLiteralNode *)less_node->right)->value, "< right value mismatch");

    // Right side of &&: 5 > (3 - 1)
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, and_node->right->type, "AND right not BINARY_OP");
    BinaryOpNode *greater_node = (BinaryOpNode *) and_node->right;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_GREATER, greater_node->op, "Op not >");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, greater_node->left->type, "> left not INT_LITERAL");
    TEST_ASSERT_EQUAL_INT_MESSAGE(5, ((IntLiteralNode *)greater_node->left)->value, "> left value mismatch");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, greater_node->right->type, "> right not BINARY_OP");
    verify_binary_op_node(greater_node->right, OPERATOR_SUBTRACT, 3, 1);

    arena_destroy(&arena);
}

// Test: !0 && 1  =>  (!0) && 1
void test_precedence_unary_not_with_logical(void) {
    const char *input = "int main(void) { return !0 && 1; }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode NULL");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error");

    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    AstNode *expr = ret->expression;
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, expr->type, "Root not BINARY_OP");

    BinaryOpNode *and_node = (BinaryOpNode *) expr;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_LOGICAL_AND, and_node->op, "Root op not &&");

    TEST_ASSERT_EQUAL_MESSAGE(NODE_UNARY_OP, and_node->left->type, "AND left not UNARY_OP");
    verify_unary_op_node(and_node->left, OPERATOR_LOGICAL_NOT, 0);

    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, and_node->right->type, "AND right not INT_LITERAL");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ((IntLiteralNode *)and_node->right)->value, "AND right value mismatch");

    arena_destroy(&arena);
}

// Test: !(1 < 2 && 0)  =>  !((1 < 2) && 0)
void test_precedence_unary_not_on_parenthesized_logical(void) {
    const char *input = "int main(void) { return !(1 < 2 && 0); }";
    Arena arena = arena_create(1024);
    Lexer lexer;
    lexer_init(&lexer, input, &arena);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program, "ProgramNode NULL");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error");

    ReturnStmtNode *ret = (ReturnStmtNode *) program->function->body;
    AstNode *expr = ret->expression;
    TEST_ASSERT_EQUAL_MESSAGE(NODE_UNARY_OP, expr->type, "Root not UNARY_OP");

    UnaryOpNode *not_node = (UnaryOpNode *) expr;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_LOGICAL_NOT, not_node->op, "Root op not !");

    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, not_node->operand->type, "NOT operand not BINARY_OP");
    BinaryOpNode *and_node = (BinaryOpNode *) not_node->operand;
    TEST_ASSERT_EQUAL_MESSAGE(OPERATOR_LOGICAL_AND, and_node->op, "Nested op not &&");

    TEST_ASSERT_EQUAL_MESSAGE(NODE_BINARY_OP, and_node->left->type, "&& left not BINARY_OP");
    verify_binary_op_node(and_node->left, OPERATOR_LESS, 1, 2);

    TEST_ASSERT_EQUAL_MESSAGE(NODE_INT_LITERAL, and_node->right->type, "&& right not INT_LITERAL");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ((IntLiteralNode *)and_node->right)->value, "&& right value mismatch");

    arena_destroy(&arena);
}


// --- Error Test Cases ---
void test_parse_error_missing_rhs_after_binary_op(void) {
    const char *input = "int main(void) { return 1 + ; }";
    // We'll refine the expected error message once implemented in the parser.
    // For now, just checking error_flag is fine, or a generic part of the message.
    verify_parser_error(input, "Expected expression (integer, unary op, or '(')");
}

void test_parse_error_consecutive_binary_operators(void) {
    const char *input = "int main(void) { return 1 + / 2; }";
    // Error should come from parse_primary_expression when it sees '/' instead of an operand for RHS of '+'
    verify_parser_error(input, "Expected expression (integer, unary op, or '('), but got '/'");
}

void test_parse_error_missing_closing_paren(void) {
    const char *input = "int main(void) { return (1 + 2 * 3; }";
    // Error should come from parser_consume within parse_parenthesized_expression
    // when it expects ')' but finds ';'.
    verify_parser_error(input, "Expected token ')', but got ';'");
}

// --- Test Runner ---
// Group parser tests into a single runner function
void run_parser_tests(void) {
    RUN_TEST(test_parse_valid_program);
    RUN_TEST(test_parse_missing_semicolon);
    RUN_TEST(test_parse_missing_brace);
    RUN_TEST(test_parse_function_empty_body);
    RUN_TEST(test_parse_negation_operator);
    RUN_TEST(test_parse_complement_operator);
    RUN_TEST(test_parse_logical_not_operator);
    RUN_TEST(test_parse_nested_unary_operators);
    RUN_TEST(test_parse_parenthesized_expression);
    RUN_TEST(test_parse_unary_with_parentheses);
    RUN_TEST(test_parse_complex_nested_expression);
    RUN_TEST(test_parse_invalid_unary_expression);
    RUN_TEST(test_parse_mismatched_parentheses);
    RUN_TEST(test_parse_integer_bounds); // Test with INT_MAX and INT_MIN
    RUN_TEST(test_parse_integer_overflow); // Test integer overflow scenario

    // Add new binary operation tests here
    RUN_TEST(test_parse_simple_addition);
    RUN_TEST(test_parse_simple_subtraction);
    RUN_TEST(test_parse_simple_multiplication);
    RUN_TEST(test_parse_simple_division);
    RUN_TEST(test_parse_simple_modulo);
    RUN_TEST(test_parse_precedence_add_mul);
    RUN_TEST(test_parse_precedence_mul_add);

    // Associativity and Parentheses
    RUN_TEST(test_parse_associativity_subtract);
    RUN_TEST(test_parse_associativity_divide);
    RUN_TEST(test_parse_parentheses_simple);
    RUN_TEST(test_parse_parentheses_nested);

    // Unary with Binary
    RUN_TEST(test_parse_unary_with_binary_simple);
    RUN_TEST(test_parse_unary_on_parenthesized_expr);

    // Relational and Logical Operator Tests
    RUN_TEST(test_parse_relational_less_than);
    RUN_TEST(test_parse_relational_greater_than);
    RUN_TEST(test_parse_relational_less_equal);
    RUN_TEST(test_parse_relational_greater_equal);
    RUN_TEST(test_parse_relational_equal_equal);
    RUN_TEST(test_parse_relational_not_equal);
    RUN_TEST(test_parse_logical_and);
    RUN_TEST(test_parse_logical_or);

    // Complex Precedence and Associativity Tests
    RUN_TEST(test_precedence_logical_or_and);
    RUN_TEST(test_precedence_relational_and_logical);
    RUN_TEST(test_precedence_arithmetic_relational_logical);
    RUN_TEST(test_precedence_unary_not_with_logical);
    RUN_TEST(test_precedence_unary_not_on_parenthesized_logical);

    // Error Handling Tests
    RUN_TEST(test_parse_error_missing_rhs_after_binary_op);
    RUN_TEST(test_parse_error_consecutive_binary_operators);
    RUN_TEST(test_parse_error_missing_closing_paren);
}
