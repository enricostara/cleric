#include <string.h>

#include "unity.h"
#include "../../src/lexer/lexer.h"      // Adjusted path
#include "../../src/parser/parser.h"    // Adjusted path
#include "../../src/parser/ast.h"       // Adjusted path
#include "../../src/memory/arena.h"   // Adjusted path

// --- Test Cases --- //

// Test parsing a simple valid program: "int main(void) { return 42; }"
void test_parse_valid_program(void) {
    const char *input = "int main(void) { return 42; }";
    Lexer lexer;
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    lexer_init(&lexer, input, &test_arena);

    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program_reworked = parse_program(&parser);

    TEST_ASSERT_NOT_NULL_MESSAGE(program_reworked, "Parser returned NULL for valid input");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, "Parser error flag was set for valid input");

    // --- AST Structure Verification --- //
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program_reworked->base.type);
    TEST_ASSERT_NOT_NULL(program_reworked->function);
    FuncDefNode *func_def = program_reworked->function;

    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def->base.type);
    TEST_ASSERT_NOT_NULL(func_def->body); // Body should now be a BlockNode
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BLOCK, func_def->body->base.type, "Function body should be a BlockNode");

    BlockNode *body_block = func_def->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, body_block->num_items, "BlockNode should contain 1 item for 'return 42;'");
    TEST_ASSERT_NOT_NULL_MESSAGE(body_block->items[0], "First item in BlockNode should not be NULL");

    AstNode *stmt = body_block->items[0]; // Get the first statement from the block
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, stmt->type);
    ReturnStmtNode *return_stmt = (ReturnStmtNode *) stmt;

    TEST_ASSERT_NOT_NULL(return_stmt->expression);
    AstNode *expr = return_stmt->expression;
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, expr->type);
    IntLiteralNode *int_literal = (IntLiteralNode *) expr;

    TEST_ASSERT_EQUAL_INT(42, int_literal->value);

    arena_destroy(&test_arena);
}

// Test parsing an invalid program (missing semicolon)
void test_parse_missing_semicolon(void) {
    const char *input = "int main(void) { return 42 }"; // Missing semicolon
    Lexer lexer;
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    lexer_init(&lexer, input, &test_arena);

    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for missing semicolon");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for missing semicolon");

    arena_destroy(&test_arena);
}

// Test parsing an invalid program (missing closing brace)
void test_parse_missing_brace(void) {
    const char *input = "int main(void) { return 42;"; // Missing brace
    Lexer lexer;
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    lexer_init(&lexer, input, &test_arena);

    Parser parser;
    parser_init(&parser, &lexer, &test_arena);

    ProgramNode *program = parse_program(&parser);

    TEST_ASSERT_NULL_MESSAGE(program, "Parser did not return NULL for missing brace");
    TEST_ASSERT_TRUE_MESSAGE(parser.error_flag, "Parser error flag was not set for missing brace");

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

    TEST_ASSERT_EQUAL_MESSAGE(NODE_PROGRAM, program->base.type, "Program node type mismatch");
    TEST_ASSERT_NOT_NULL_MESSAGE(program->function, "ProgramNode has no function");
    FuncDefNode *func_def = program->function;

    TEST_ASSERT_EQUAL_MESSAGE(NODE_FUNC_DEF, func_def->base.type, "Function definition node type mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("main", func_def->name, "Function name mismatch");

    TEST_ASSERT_NOT_NULL_MESSAGE(func_def->body, "Function body (BlockNode) should not be NULL for an empty function");
    TEST_ASSERT_EQUAL_MESSAGE(NODE_BLOCK, func_def->body->base.type,
                              "Function body should be a BlockNode for an empty function");
    BlockNode *empty_body_block = func_def->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, empty_body_block->num_items,
                                  "BlockNode should have 0 items for an empty function body {}");

    arena_destroy(&test_arena);
}

// --- Test Runner --- //
void run_parser_program_tests(void) {
    RUN_TEST(test_parse_valid_program);
    RUN_TEST(test_parse_missing_semicolon);
    RUN_TEST(test_parse_missing_brace);
    RUN_TEST(test_parse_function_empty_body);
}
