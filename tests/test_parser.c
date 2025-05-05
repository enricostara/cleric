#include "unity.h"
#include "../src/lexer/lexer.h" // Include Lexer for tokenization
#include "../src/parser/parser.h" // Include Parser for parsing
#include "../src/parser/ast.h"    // Include AST for structure checks
#include "../src/memory/arena.h" // Include Arena for parser

// --- Test Helper Function ---

// Helper function to verify a unary operation node
static void verify_unary_op_node(AstNode* node, UnaryOperatorType expected_op, int expected_value) {
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, node->type);
    
    UnaryOpNode* unary_node = (UnaryOpNode*)node;
    TEST_ASSERT_EQUAL(expected_op, unary_node->op);
    TEST_ASSERT_NOT_NULL(unary_node->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, unary_node->operand->type);
    
    IntLiteralNode* int_node = (IntLiteralNode*)unary_node->operand;
    TEST_ASSERT_EQUAL_INT(expected_value, int_node->value);
}

// Helper function to verify a nested unary operation
static void verify_nested_unary_op(AstNode* node, UnaryOperatorType outer_op, UnaryOperatorType inner_op, int value) {
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, node->type);
    
    UnaryOpNode* outer_node = (UnaryOpNode*)node;
    TEST_ASSERT_EQUAL(outer_op, outer_node->op);
    TEST_ASSERT_NOT_NULL(outer_node->operand);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, outer_node->operand->type);
    
    UnaryOpNode* inner_node = (UnaryOpNode*)outer_node->operand;
    TEST_ASSERT_EQUAL(inner_op, inner_node->op);
    TEST_ASSERT_NOT_NULL(inner_node->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, inner_node->operand->type);
    
    IntLiteralNode* int_node = (IntLiteralNode*)inner_node->operand;
    TEST_ASSERT_EQUAL_INT(value, int_node->value);
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
    lexer_init(&lexer, input);
    Parser parser;
    Arena test_arena = arena_create(1024); // Create arena for this test
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program_reworked = parse_program(&parser, &test_arena);

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
    lexer_init(&lexer, input);
    Parser parser;
    Arena test_arena = arena_create(1024); // Create arena for this test
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser, &test_arena);

    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for missing semicolon");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for missing semicolon");

    // Clean up the arena
    arena_destroy(&test_arena);
}

// Test parsing an invalid program (missing closing brace)
void test_parse_missing_brace(void) {
    const char *input = "int main(void) { return 42;"; // Missing brace
    Lexer lexer;
    lexer_init(&lexer, input);
    Parser parser;
    Arena test_arena = arena_create(1024); // Create arena for this test
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser, &test_arena);

    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for missing brace");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for missing brace");

    // Clean up the arena
    arena_destroy(&test_arena);
}

// Test parsing a program with a negation unary operator
void test_parse_negation_operator(void) {
    const char *input = "int main(void) { return -42; }";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    
    Lexer lexer;
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    
    ProgramNode *program = parse_program(&parser, &test_arena);
    
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
    ReturnStmtNode *return_stmt = (ReturnStmtNode*)func_def->body;
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
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    
    ProgramNode *program = parse_program(&parser, &test_arena);
    
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
    ReturnStmtNode *return_stmt = (ReturnStmtNode*)func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);
    
    // Verify unary operation
    verify_unary_op_node(return_stmt->expression, OPERATOR_COMPLEMENT, 42);
    
    arena_destroy(&test_arena);
}

// Test parsing a program with nested unary operators
void test_parse_nested_unary_operators(void) {
    const char *input = "int main(void) { return -~42; }";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    
    Lexer lexer;
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    
    ProgramNode *program = parse_program(&parser, &test_arena);
    
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
    ReturnStmtNode *return_stmt = (ReturnStmtNode*)func_def->body;
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
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    
    ProgramNode *program = parse_program(&parser, &test_arena);
    
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
    ReturnStmtNode *return_stmt = (ReturnStmtNode*)func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);
    
    // Verify integer literal (parentheses don't create a node, just group)
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, return_stmt->expression->type);
    IntLiteralNode *int_literal = (IntLiteralNode*)return_stmt->expression;
    TEST_ASSERT_EQUAL_INT(42, int_literal->value);
    
    arena_destroy(&test_arena);
}

// Test parsing a program with unary operator and parentheses
void test_parse_unary_with_parentheses(void) {
    const char *input = "int main(void) { return -(42); }";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    
    Lexer lexer;
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    
    ProgramNode *program = parse_program(&parser, &test_arena);
    
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input with unary op and parentheses");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input with unary op and parentheses");
    
    // Verify program structure
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program->base.type);
    TEST_ASSERT_NOT_NULL(program->function);
    
    // Verify function structure
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);
    
    // Verify return statement
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_def->body->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode*)func_def->body;
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
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    
    ProgramNode *program = parse_program(&parser, &test_arena);
    
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Parser returned NULL for valid input with complex nested expression");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input with complex nested expression");
    
    // Verify program structure
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program->base.type);
    TEST_ASSERT_NOT_NULL(program->function);
    
    // Verify function structure
    FuncDefNode *func_def = program->function;
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body);
    
    // Verify return statement
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, func_def->body->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode*)func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);
    
    // Verify outer complement
    AstNode* expr = return_stmt->expression;
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, expr->type);
    UnaryOpNode* outer_complement = (UnaryOpNode*)expr;
    TEST_ASSERT_EQUAL(OPERATOR_COMPLEMENT, outer_complement->op);
    
    // Verify middle negation
    TEST_ASSERT_NOT_NULL(outer_complement->operand);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, outer_complement->operand->type);
    UnaryOpNode* middle_negate = (UnaryOpNode*)outer_complement->operand;
    TEST_ASSERT_EQUAL(OPERATOR_NEGATE, middle_negate->op);
    
    // Verify inner complement
    TEST_ASSERT_NOT_NULL(middle_negate->operand);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, middle_negate->operand->type);
    UnaryOpNode* inner_complement = (UnaryOpNode*)middle_negate->operand;
    TEST_ASSERT_EQUAL(OPERATOR_COMPLEMENT, inner_complement->op);
    
    // Verify integer literal
    TEST_ASSERT_NOT_NULL(inner_complement->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, inner_complement->operand->type);
    IntLiteralNode* int_literal = (IntLiteralNode*)inner_complement->operand;
    TEST_ASSERT_EQUAL_INT(42, int_literal->value);
    
    arena_destroy(&test_arena);
}

// Test parsing a program with invalid unary expression (missing operand)
void test_parse_invalid_unary_expression(void) {
    const char *input = "int main(void) { return -; }";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    
    Lexer lexer;
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    
    ProgramNode *program = parse_program(&parser, &test_arena);
    
    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for invalid unary expression");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for invalid unary expression");
    
    arena_destroy(&test_arena);
}

// Test parsing a program with mismatched parentheses
void test_parse_mismatched_parentheses(void) {
    const char *input = "int main(void) { return (42; }";
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    
    Lexer lexer;
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    
    ProgramNode *program = parse_program(&parser, &test_arena);
    
    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for mismatched parentheses");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for mismatched parentheses");
    
    arena_destroy(&test_arena);
}

// Test parsing a program with integer at the boundary of what's representable
void test_parse_integer_bounds(void) {
    const char *input = "int main(void) { return 2147483647; }"; // INT_MAX for 32-bit int
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    
    Lexer lexer;
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    
    ProgramNode *program = parse_program(&parser, &test_arena);
    
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
    ReturnStmtNode *return_stmt = (ReturnStmtNode*)func_def->body;
    TEST_ASSERT_NOT_NULL(return_stmt->expression);
    
    // Verify integer literal
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, return_stmt->expression->type);
    IntLiteralNode *int_literal = (IntLiteralNode*)return_stmt->expression;
    TEST_ASSERT_EQUAL_INT(2147483647, int_literal->value); // INT_MAX
    
    arena_destroy(&test_arena);
}

// Test parsing a program with integer exceeding the representable range
void test_parse_integer_overflow(void) {
    const char *input = "int main(void) { return 2147483648; }"; // INT_MAX + 1 for 32-bit int
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    
    Lexer lexer;
    lexer_init(&lexer, input);
    Parser parser;
    parser_init(&parser, &lexer, &test_arena);
    
    ProgramNode *program = parse_program(&parser, &test_arena);
    
    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for integer overflow");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for integer overflow");
    
    arena_destroy(&test_arena);
}

// --- Test Runner ---
// Group parser tests into a single runner function
void run_parser_tests(void) {
    printf("\n--- Parser Tests ---\n");
    RUN_TEST(test_parse_valid_program);
    RUN_TEST(test_parse_missing_semicolon);
    RUN_TEST(test_parse_missing_brace);
    
    // New tests for unary operations and expressions
    RUN_TEST(test_parse_negation_operator);
    RUN_TEST(test_parse_complement_operator);
    RUN_TEST(test_parse_nested_unary_operators);
    RUN_TEST(test_parse_parenthesized_expression);
    RUN_TEST(test_parse_unary_with_parentheses);
    RUN_TEST(test_parse_complex_nested_expression);
    RUN_TEST(test_parse_invalid_unary_expression);
    RUN_TEST(test_parse_mismatched_parentheses);
    RUN_TEST(test_parse_integer_bounds);
    RUN_TEST(test_parse_integer_overflow);
}
