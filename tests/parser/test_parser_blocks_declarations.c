#include "unity.h"
#include "../../src/lexer/lexer.h"
#include "../../src/parser/parser.h"
#include "../../src/parser/ast.h"
#include "../../src/memory/arena.h"
#include <stdio.h> // For debugging parser errors if necessary

// Helper function to setup parser for a test case
// Note: Parser and Lexer are returned by pointer, Arenas are managed internally by this helper for now.
// Consider a more robust setup/teardown if tests get more complex.
static void setup_parser_for_test(const char *source, Lexer *lexer, Parser *parser, Arena *ast_arena, Arena *lexer_arena) {
    *ast_arena = arena_create(1024 * 10);    // 10KB for AST
    *lexer_arena = arena_create(1024);      // 1KB for Lexer tokens
    // TODO: Add null checks for arena_create results if it can fail gracefully
    // For example: if (ast_arena->start == NULL || lexer_arena->start == NULL) { /* handle error */ }
    lexer_init(lexer, source, lexer_arena);
    parser_init(parser, lexer, ast_arena);
}

// Test for a function with an empty block body: int main() {}
void test_parse_empty_block__empty_function_body(void) {
    Lexer lexer;
    Parser parser;
    Arena ast_arena, lexer_arena;
    setup_parser_for_test("int main() {}", &lexer, &parser, &ast_arena, &lexer_arena);

    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Program node should not be NULL.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message);
    TEST_ASSERT_NOT_NULL_MESSAGE(program->function, "Function node should not be NULL.");
    TEST_ASSERT_EQUAL_STRING("main", program->function->name);
    
    TEST_ASSERT_NOT_NULL_MESSAGE(program->function->body, "Function body (BlockNode) should not be NULL.");
    TEST_ASSERT_EQUAL_UINT(NODE_BLOCK, program->function->body->base.type);
    BlockNode* body_block = (BlockNode*)program->function->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, body_block->num_items, "BlockNode should have 0 items for empty function body.");

    arena_destroy(&ast_arena);
    arena_destroy(&lexer_arena);
}

// Test for a function with a simple declaration: int main() { int x; }
void test_parse_simple_declaration__single_int_var(void) {
    Lexer lexer;
    Parser parser;
    Arena ast_arena, lexer_arena;
    setup_parser_for_test("int main() { int x; }", &lexer, &parser, &ast_arena, &lexer_arena);

    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Program node should not be NULL.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message);
    
    BlockNode *body = (BlockNode*)program->function->body;
    TEST_ASSERT_NOT_NULL_MESSAGE(body, "Function body should not be NULL.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, body->num_items, "Block should contain 1 item.");
    
    AstNode *item0 = body->items[0];
    TEST_ASSERT_NOT_NULL_MESSAGE(item0, "First item in block should not be NULL.");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(NODE_VAR_DECL, item0->type, "Item 0 should be VarDeclNode.");
    
    VarDeclNode *var_decl = (VarDeclNode *)item0;
    TEST_ASSERT_EQUAL_STRING("int", var_decl->type_name);
    TEST_ASSERT_EQUAL_STRING("x", var_decl->var_name);
    TEST_ASSERT_NULL_MESSAGE(var_decl->initializer, "Initializer should be NULL.");

    arena_destroy(&ast_arena);
    arena_destroy(&lexer_arena);
}

// Test for a function with a declaration and a return: int main() { int x; return 0; }
void test_parse_declaration_and_return__int_var_then_return_zero(void) {
    Lexer lexer;
    Parser parser;
    Arena ast_arena, lexer_arena;
    setup_parser_for_test("int main() { int x; return 0; }", &lexer, &parser, &ast_arena, &lexer_arena);

    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Program node should not be NULL.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message);

    BlockNode *body = (BlockNode*)program->function->body;
    TEST_ASSERT_NOT_NULL_MESSAGE(body, "Function body should not be NULL.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, body->num_items, "Block should contain 2 items.");

    AstNode *item0 = body->items[0];
    TEST_ASSERT_EQUAL_UINT_MESSAGE(NODE_VAR_DECL, item0->type, "Item 0 should be VarDeclNode.");
    VarDeclNode *var_decl = (VarDeclNode *)item0;
    TEST_ASSERT_EQUAL_STRING("int", var_decl->type_name);
    TEST_ASSERT_EQUAL_STRING("x", var_decl->var_name);

    AstNode *item1 = body->items[1];
    TEST_ASSERT_EQUAL_UINT_MESSAGE(NODE_RETURN_STMT, item1->type, "Item 1 should be ReturnStmtNode.");
    ReturnStmtNode *ret_stmt = (ReturnStmtNode *)item1;
    TEST_ASSERT_NOT_NULL_MESSAGE(ret_stmt->expression, "Return expression should not be NULL.");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(NODE_INT_LITERAL, ret_stmt->expression->type, "Return expression should be IntLiteralNode.");
    TEST_ASSERT_EQUAL_INT(((IntLiteralNode *)ret_stmt->expression)->value, 0);

    arena_destroy(&ast_arena);
    arena_destroy(&lexer_arena);
}

// Test for an expression statement: int main() { 123; return 0; }
void test_parse_expression_statement__literal_expression(void) {
    Lexer lexer;
    Parser parser;
    Arena ast_arena, lexer_arena;
    setup_parser_for_test("int main() { 123; return 0; }", &lexer, &parser, &ast_arena, &lexer_arena);

    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Program node should not be NULL.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message);

    BlockNode *body = (BlockNode*)program->function->body;
    TEST_ASSERT_NOT_NULL_MESSAGE(body, "Function body should not be NULL.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, body->num_items, "Block should contain 2 items.");

    AstNode *item0_expr_stmt = body->items[0];
    TEST_ASSERT_EQUAL_UINT_MESSAGE(NODE_INT_LITERAL, item0_expr_stmt->type, "Item 0 (expression statement) should be IntLiteralNode.");
    TEST_ASSERT_EQUAL_INT(((IntLiteralNode *)item0_expr_stmt)->value, 123);

    AstNode *item1_ret_stmt = body->items[1];
    TEST_ASSERT_EQUAL_UINT_MESSAGE(NODE_RETURN_STMT, item1_ret_stmt->type, "Item 1 should be ReturnStmtNode.");

    arena_destroy(&ast_arena);
    arena_destroy(&lexer_arena);
}

// Test for an empty statement: int main() { ; return 0; }
// Current parser_block skips empty statements, so they don't appear in BlockNode items.
void test_parse_empty_statement__semicolon_only_is_skipped(void) {
    Lexer lexer;
    Parser parser;
    Arena ast_arena, lexer_arena;
    setup_parser_for_test("int main() { ; return 0; }", &lexer, &parser, &ast_arena, &lexer_arena);

    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Program node should not be NULL.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message);

    BlockNode *body = (BlockNode*)program->function->body;
    TEST_ASSERT_NOT_NULL_MESSAGE(body, "Function body should not be NULL.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, body->num_items, "Block should contain 1 item (empty statement is skipped).");
    
    AstNode *item0_ret_stmt = body->items[0];
    TEST_ASSERT_EQUAL_UINT_MESSAGE(NODE_RETURN_STMT, item0_ret_stmt->type, "Item 0 should be ReturnStmtNode.");

    arena_destroy(&ast_arena);
    arena_destroy(&lexer_arena);
}

// Test for a nested block: int main() { { int y; } return 0; }
void test_parse_nested_block__inner_block_with_declaration(void) {
    Lexer lexer;
    Parser parser;
    Arena ast_arena, lexer_arena;
    setup_parser_for_test("int main() { { int y; } return 0; }", &lexer, &parser, &ast_arena, &lexer_arena);

    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL_MESSAGE(program, "Program node should not be NULL.");
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message);

    BlockNode *outer_body = (BlockNode*)program->function->body;
    TEST_ASSERT_NOT_NULL_MESSAGE(outer_body, "Outer function body should not be NULL.");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, outer_body->num_items, "Outer block should contain 2 items.");

    AstNode *item0_nested_block_node = outer_body->items[0];
    TEST_ASSERT_EQUAL_UINT_MESSAGE(NODE_BLOCK, item0_nested_block_node->type, "Item 0 should be BlockNode (nested block).");
    BlockNode *inner_block = (BlockNode *)item0_nested_block_node;
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, inner_block->num_items, "Inner block should contain 1 item.");
    
    AstNode *inner_item0_var_decl = inner_block->items[0];
    TEST_ASSERT_EQUAL_UINT_MESSAGE(NODE_VAR_DECL, inner_item0_var_decl->type, "Inner block item 0 should be VarDeclNode.");
    VarDeclNode *var_decl_y = (VarDeclNode *)inner_item0_var_decl;
    TEST_ASSERT_EQUAL_STRING("int", var_decl_y->type_name);
    TEST_ASSERT_EQUAL_STRING("y", var_decl_y->var_name);

    AstNode *item1_ret_stmt = outer_body->items[1];
    TEST_ASSERT_EQUAL_UINT_MESSAGE(NODE_RETURN_STMT, item1_ret_stmt->type, "Item 1 should be ReturnStmtNode.");

    arena_destroy(&ast_arena);
    arena_destroy(&lexer_arena);
}

// Test for multiple declarations: int main() { int x; int y; return 0; }
void test_parse_multiple_declarations(void) {
    Lexer lexer;
    Parser parser;
    Arena ast_arena, lexer_arena;
    setup_parser_for_test("int main() { int x; int y; return 0; }", &lexer, &parser, &ast_arena, &lexer_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message);
    BlockNode *body = (BlockNode*)program->function->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, body->num_items, "Block should have 3 items.");
    TEST_ASSERT_EQUAL_UINT(NODE_VAR_DECL, body->items[0]->type);
    TEST_ASSERT_EQUAL_STRING("x", ((VarDeclNode*)body->items[0])->var_name);
    TEST_ASSERT_EQUAL_UINT(NODE_VAR_DECL, body->items[1]->type);
    TEST_ASSERT_EQUAL_STRING("y", ((VarDeclNode*)body->items[1])->var_name);
    TEST_ASSERT_EQUAL_UINT(NODE_RETURN_STMT, body->items[2]->type);
    arena_destroy(&ast_arena);
    arena_destroy(&lexer_arena);
}

// Test for mixed declarations and statements: int main() { int x; 123; int y; return 0; }
void test_parse_mixed_declarations_and_statements(void) {
    Lexer lexer;
    Parser parser;
    Arena ast_arena, lexer_arena;
    setup_parser_for_test("int main() { int x; 123; int y; return 0; }", &lexer, &parser, &ast_arena, &lexer_arena);
    ProgramNode *program = parse_program(&parser);
    TEST_ASSERT_FALSE_MESSAGE(parser.error_flag, parser.error_message);
    BlockNode *body = (BlockNode*)program->function->body;
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, body->num_items, "Block should have 4 items.");
    TEST_ASSERT_EQUAL_UINT(NODE_VAR_DECL, body->items[0]->type);
    TEST_ASSERT_EQUAL_STRING("x", ((VarDeclNode*)body->items[0])->var_name);
    TEST_ASSERT_EQUAL_UINT(NODE_INT_LITERAL, body->items[1]->type);
    TEST_ASSERT_EQUAL_INT(123, ((IntLiteralNode*)body->items[1])->value);
    TEST_ASSERT_EQUAL_UINT(NODE_VAR_DECL, body->items[2]->type);
    TEST_ASSERT_EQUAL_STRING("y", ((VarDeclNode*)body->items[2])->var_name);
    TEST_ASSERT_EQUAL_UINT(NODE_RETURN_STMT, body->items[3]->type);
    arena_destroy(&ast_arena);
    arena_destroy(&lexer_arena);
}


void run_parser_blocks_declarations_tests(void) {
    RUN_TEST(test_parse_empty_block__empty_function_body);
    RUN_TEST(test_parse_simple_declaration__single_int_var);
    RUN_TEST(test_parse_declaration_and_return__int_var_then_return_zero);
    RUN_TEST(test_parse_expression_statement__literal_expression);
    RUN_TEST(test_parse_empty_statement__semicolon_only_is_skipped);
    RUN_TEST(test_parse_nested_block__inner_block_with_declaration);
    RUN_TEST(test_parse_multiple_declarations);
    RUN_TEST(test_parse_mixed_declarations_and_statements);
}
