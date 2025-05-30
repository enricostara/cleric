#include "_unity/unity.h"
#include "../src/parser/ast.h"
#include "../src/memory/arena.h"
#include "../src/lexer/lexer.h" // For Token struct

// Test case for creating and checking an integer literal node
static void test_create_int_literal(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    IntLiteralNode *node = create_int_literal_node(42, &test_arena);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, node->base.type);
    TEST_ASSERT_EQUAL_INT64(42L, node->value);
    arena_destroy(&test_arena);
}

// Test case for creating a return statement node
static void test_create_return_stmt(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    IntLiteralNode *expr_node = create_int_literal_node(5, &test_arena);
    TEST_ASSERT_NOT_NULL(expr_node);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *) expr_node, &test_arena);
    TEST_ASSERT_NOT_NULL(return_node);
    TEST_ASSERT_EQUAL(NODE_RETURN_STMT, return_node->base.type);
    const ReturnStmtNode *ret_node = (ReturnStmtNode *) return_node;
    TEST_ASSERT_EQUAL_PTR(expr_node, ret_node->expression);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, ret_node->expression->type);
    arena_destroy(&test_arena);
}

// Test case for creating a function definition node
static void test_create_func_def(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    // Create a simple return statement for the function body
    IntLiteralNode *expr = create_int_literal_node(2, &test_arena);
    TEST_ASSERT_NOT_NULL(expr);
    ReturnStmtNode *ret_stmt = create_return_stmt_node((AstNode *) expr, &test_arena);
    TEST_ASSERT_NOT_NULL(ret_stmt);

    // Create a block node and add the return statement to it
    BlockNode *body_block = create_block_node(&test_arena);
    TEST_ASSERT_NOT_NULL(body_block);
    TEST_ASSERT_TRUE(block_node_add_item(body_block, (AstNode *)ret_stmt, &test_arena));

    // Create the function definition node with the block node as its body
    FuncDefNode *func_def_node = create_func_def_node("main", body_block, &test_arena);
    TEST_ASSERT_NOT_NULL(func_def_node);
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func_def_node->base.type);
    TEST_ASSERT_EQUAL_STRING("main", func_def_node->name);
    TEST_ASSERT_EQUAL_PTR(body_block, func_def_node->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func_def_node->body->base.type);
    TEST_ASSERT_EQUAL_INT(1, func_def_node->body->num_items);
    TEST_ASSERT_EQUAL_PTR(ret_stmt, func_def_node->body->items[0]);

    arena_destroy(&test_arena);
}

// Test case for creating a full program node (simplified)
static void test_create_program(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    // Create a simple return statement
    IntLiteralNode *expr = create_int_literal_node(2, &test_arena);
    TEST_ASSERT_NOT_NULL(expr);
    ReturnStmtNode *ret_stmt = create_return_stmt_node((AstNode *) expr, &test_arena);
    TEST_ASSERT_NOT_NULL(ret_stmt);

    // Create a block node for the function body
    BlockNode *func_body_block = create_block_node(&test_arena);
    TEST_ASSERT_NOT_NULL(func_body_block);
    TEST_ASSERT_TRUE(block_node_add_item(func_body_block, (AstNode*)ret_stmt, &test_arena));

    // Create a function definition node
    FuncDefNode *func_def = create_func_def_node("main", func_body_block, &test_arena);
    TEST_ASSERT_NOT_NULL(func_def);

    // Create the program node
    ProgramNode *program_node = create_program_node(func_def, &test_arena);
    TEST_ASSERT_NOT_NULL(program_node);
    TEST_ASSERT_EQUAL(NODE_PROGRAM, program_node->base.type);
    TEST_ASSERT_EQUAL_PTR(func_def, (AstNode*)program_node->function);
    arena_destroy(&test_arena);
}

// Test case for creating a unary negate operation node
static void test_create_unary_op_negate(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    IntLiteralNode *operand_node = create_int_literal_node(5, &test_arena);
    TEST_ASSERT_NOT_NULL(operand_node);
    UnaryOpNode *unary_node = create_unary_op_node(OPERATOR_NEGATE, (AstNode *) operand_node, &test_arena);
    TEST_ASSERT_NOT_NULL(unary_node);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, unary_node->base.type);
    TEST_ASSERT_EQUAL(OPERATOR_NEGATE, unary_node->op);
    TEST_ASSERT_EQUAL_PTR(operand_node, unary_node->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, unary_node->operand->type);
    arena_destroy(&test_arena);
}

// Test case for creating a unary complement operation node
static void test_create_unary_op_complement(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    IntLiteralNode *operand_node = create_int_literal_node(10, &test_arena);
    TEST_ASSERT_NOT_NULL(operand_node);
    UnaryOpNode *unary_node = create_unary_op_node(OPERATOR_COMPLEMENT, (AstNode *) operand_node, &test_arena);
    TEST_ASSERT_NOT_NULL(unary_node);
    TEST_ASSERT_EQUAL(NODE_UNARY_OP, unary_node->base.type);
    TEST_ASSERT_EQUAL(OPERATOR_COMPLEMENT, unary_node->op);
    TEST_ASSERT_EQUAL_PTR(operand_node, unary_node->operand);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, unary_node->operand->type);
    arena_destroy(&test_arena);
}

// Test case for creating a binary ADD operation node
static void test_create_binary_op_add(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for binary_op_add");

    IntLiteralNode *left_operand = create_int_literal_node(10, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(left_operand, "Failed to create left operand for binary_op_add");

    IntLiteralNode *right_operand = create_int_literal_node(5, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(right_operand, "Failed to create right operand for binary_op_add");

    BinaryOpNode *binary_node = create_binary_op_node(OPERATOR_ADD, (AstNode *) left_operand, (AstNode *) right_operand,
                                                      &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(binary_node, "Failed to create binary_op_node for ADD");

    TEST_ASSERT_EQUAL(NODE_BINARY_OP, binary_node->base.type);
    TEST_ASSERT_EQUAL(OPERATOR_ADD, binary_node->op);
    TEST_ASSERT_EQUAL_PTR(left_operand, binary_node->left);
    TEST_ASSERT_EQUAL_PTR(right_operand, binary_node->right);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, binary_node->left->type); // Check operand types too
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, binary_node->right->type);

    arena_destroy(&test_arena);
}

// Test case that calls ast_pretty_print (visual inspection needed)
static void test_ast_pretty_print_output(void) {
    Arena test_arena = arena_create(2048); // Slightly increased arena size
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for pretty_print");

    // Build AST for: int main(void) { return (10 + 5) * 2; } 
    IntLiteralNode *val10 = create_int_literal_node(10, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(val10, "Failed to create val10 for pretty_print");
    IntLiteralNode *val5 = create_int_literal_node(5, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(val5, "Failed to create val5 for pretty_print");

    BinaryOpNode *sum_op = create_binary_op_node(OPERATOR_ADD, (AstNode *) val10, (AstNode *) val5, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(sum_op, "Failed to create sum_op for pretty_print");

    IntLiteralNode *val2 = create_int_literal_node(2, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(val2, "Failed to create val2 for pretty_print");

    BinaryOpNode *mul_op = create_binary_op_node(OPERATOR_MULTIPLY, (AstNode *) sum_op, (AstNode *) val2, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(mul_op, "Failed to create mul_op for pretty_print");

    ReturnStmtNode *ret_stmt = create_return_stmt_node((AstNode *) mul_op, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(ret_stmt, "Failed to create ret_stmt for pretty_print");

    BlockNode *func_body_block = create_block_node(&test_arena);
    TEST_ASSERT_NOT_NULL(func_body_block);
    TEST_ASSERT_TRUE(block_node_add_item(func_body_block, (AstNode*)ret_stmt, &test_arena));

    FuncDefNode *func_def = create_func_def_node("main", func_body_block, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(func_def, "Failed to create func_def for pretty_print");

    ProgramNode *program_node = create_program_node(func_def, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(program_node, "Failed to create program_node for pretty_print");

    printf("\n--- AST Pretty Print Output Start ---\n");
    ast_pretty_print((AstNode *) program_node, 0);
    printf("--- AST Pretty Print Output End ---\n");

    arena_destroy(&test_arena);
    TEST_PASS_MESSAGE("AST Pretty Print test executed. Visually inspect output.");
}

// Test case for creating an identifier node
static void test_create_identifier_node(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    IdentifierNode *node = create_identifier_node("myVar", &test_arena);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_IDENTIFIER, node->base.type);
    TEST_ASSERT_EQUAL_STRING("myVar", node->name);
    arena_destroy(&test_arena);
}

// Test case for creating a variable declaration node without initializer
static void test_create_var_decl_node_no_initializer(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    Token dummy_token_x = {.type = TOKEN_IDENTIFIER, .lexeme = "x", .position = 0}; // Dummy token
    VarDeclNode *node = create_var_decl_node("int", "x", dummy_token_x, NULL, &test_arena);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_VAR_DECL, node->base.type);
    TEST_ASSERT_EQUAL_STRING("int", node->type_name);
    TEST_ASSERT_EQUAL_STRING("x", node->var_name);
    TEST_ASSERT_NULL(node->initializer);
    arena_destroy(&test_arena);
}

// Test case for creating a variable declaration node with initializer
static void test_create_var_decl_node_with_initializer(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    IntLiteralNode *init_expr = create_int_literal_node(100, &test_arena);
    TEST_ASSERT_NOT_NULL(init_expr);
    Token dummy_token_y = {.type = TOKEN_IDENTIFIER, .lexeme = "y", .position = 0}; // Dummy token
    VarDeclNode *node = create_var_decl_node("int", "y", dummy_token_y, (AstNode *) init_expr, &test_arena);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL(NODE_VAR_DECL, node->base.type);
    TEST_ASSERT_EQUAL_STRING("int", node->type_name);
    TEST_ASSERT_EQUAL_STRING("y", node->var_name);
    TEST_ASSERT_EQUAL_PTR(init_expr, node->initializer);
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, node->initializer->type);
    arena_destroy(&test_arena);
}

// Test case for creating an empty block node
static void test_create_block_node_empty(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    BlockNode *block = create_block_node(&test_arena);
    TEST_ASSERT_NOT_NULL(block);
    TEST_ASSERT_EQUAL(NODE_BLOCK, block->base.type);
    TEST_ASSERT_EQUAL_INT(0, block->num_items);
    TEST_ASSERT_EQUAL_INT(0, block->capacity); // Initial capacity is 0 as per ast.c
    TEST_ASSERT_NULL(block->items); // Initial items is NULL
    arena_destroy(&test_arena);
}

// Test case for adding items to a block node and checking capacity
static void test_block_node_add_items(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");
    BlockNode *block = create_block_node(&test_arena);
    TEST_ASSERT_NOT_NULL(block);

    VarDeclNode *decl1 = create_var_decl_node(
        "int", "a", (Token){.type = TOKEN_IDENTIFIER, .lexeme = "a", .position = 0}, NULL, &test_arena);
    TEST_ASSERT_NOT_NULL(decl1);
    IntLiteralNode *val5 = create_int_literal_node(5, &test_arena);
    TEST_ASSERT_NOT_NULL(val5);
    ReturnStmtNode *ret_stmt = create_return_stmt_node((AstNode *) val5, &test_arena);
    TEST_ASSERT_NOT_NULL(ret_stmt);

    TEST_ASSERT_TRUE(block_node_add_item(block, (AstNode*)decl1, &test_arena));
    TEST_ASSERT_EQUAL_INT(1, block->num_items);
    TEST_ASSERT_EQUAL_PTR(decl1, block->items[0]);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(1, block->capacity); // Capacity should be at least 1 (INITIAL_BLOCK_CAPACITY)

    TEST_ASSERT_TRUE(block_node_add_item(block, (AstNode*)ret_stmt, &test_arena));
    TEST_ASSERT_EQUAL_INT(2, block->num_items);
    TEST_ASSERT_EQUAL_PTR(ret_stmt, block->items[1]);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(2, block->capacity);

    // Test adding items to trigger reallocation (INITIAL_BLOCK_CAPACITY is 8)
    for (int i = 0; i < 10; ++i) {
        IntLiteralNode *item_val = create_int_literal_node(i, &test_arena);
        TEST_ASSERT_TRUE(block_node_add_item(block, (AstNode*)item_val, &test_arena));
    }
    TEST_ASSERT_EQUAL_INT(12, block->num_items); // 2 existing + 10 new
    TEST_ASSERT_GREATER_OR_EQUAL_INT(12, block->capacity); // Capacity should have grown
    // Check if the original items are still there
    TEST_ASSERT_EQUAL_PTR(decl1, block->items[0]);
    TEST_ASSERT_EQUAL_PTR(ret_stmt, block->items[1]);
    // Check one of the newly added items
    TEST_ASSERT_EQUAL(NODE_INT_LITERAL, block->items[11]->type);

    arena_destroy(&test_arena);
}

// Test case for creating a function definition node with a block body
static void test_create_func_def_with_block_body(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    BlockNode *body_block = create_block_node(&test_arena);
    TEST_ASSERT_NOT_NULL(body_block);

    VarDeclNode *decl = create_var_decl_node("int", "temp",
                                             (Token){.type = TOKEN_IDENTIFIER, .lexeme = "temp", .position = 0}, NULL,
                                             &test_arena);
    TEST_ASSERT_NOT_NULL(decl);
    IntLiteralNode *ret_val_literal = create_int_literal_node(0, &test_arena);
    TEST_ASSERT_NOT_NULL(ret_val_literal);
    IdentifierNode *ident_temp = create_identifier_node("temp", &test_arena);
    TEST_ASSERT_NOT_NULL(ident_temp);
    ReturnStmtNode *ret_stmt = create_return_stmt_node((AstNode *) ident_temp, &test_arena); // Return the variable
    TEST_ASSERT_NOT_NULL(ret_stmt);

    TEST_ASSERT_TRUE(block_node_add_item(body_block, (AstNode*)decl, &test_arena));
    TEST_ASSERT_TRUE(block_node_add_item(body_block, (AstNode*)ret_stmt, &test_arena));

    FuncDefNode *func = create_func_def_node("test_func", body_block, &test_arena);
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL(NODE_FUNC_DEF, func->base.type);
    TEST_ASSERT_EQUAL_STRING("test_func", func->name);
    TEST_ASSERT_EQUAL_PTR(body_block, func->body);
    TEST_ASSERT_EQUAL(NODE_BLOCK, func->body->base.type);
    TEST_ASSERT_EQUAL_INT(2, func->body->num_items);
    TEST_ASSERT_EQUAL_PTR(decl, func->body->items[0]);
    TEST_ASSERT_EQUAL_PTR(ret_stmt, func->body->items[1]);

    arena_destroy(&test_arena);
}

// Test case that calls ast_pretty_print for new node types (visual inspection needed)
static void test_ast_pretty_print_new_nodes(void) {
    Arena test_arena = arena_create(2048);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for new_nodes_pretty_print");

    // Build AST for: 
    // int my_func(void) {
    //   int x;
    //   int y = 10;
    //   return y;
    // }
    BlockNode *body_block = create_block_node(&test_arena);
    VarDeclNode *decl_x = create_var_decl_node(
        "int", "x", (Token){.type = TOKEN_IDENTIFIER, .lexeme = "x", .position = 0}, NULL, &test_arena);
    IntLiteralNode *val10 = create_int_literal_node(10, &test_arena);
    VarDeclNode *decl_y = create_var_decl_node(
        "int", "y", (Token){.type = TOKEN_IDENTIFIER, .lexeme = "y", .position = 0}, (AstNode *) val10, &test_arena);
    IdentifierNode *ident_y = create_identifier_node("y", &test_arena);
    ReturnStmtNode *ret_y = create_return_stmt_node((AstNode *) ident_y, &test_arena);

    block_node_add_item(body_block, (AstNode *) decl_x, &test_arena);
    block_node_add_item(body_block, (AstNode *) decl_y, &test_arena);
    block_node_add_item(body_block, (AstNode *) ret_y, &test_arena);

    FuncDefNode *func_def = create_func_def_node("my_func", body_block, &test_arena);
    ProgramNode *program_node = create_program_node(func_def, &test_arena);

    printf("\n--- AST Pretty Print New Nodes Output Start ---\n");
    ast_pretty_print((AstNode *) program_node, 0);
    printf("--- AST Pretty Print New Nodes Output End ---\n");

    arena_destroy(&test_arena);
    TEST_PASS_MESSAGE("AST Pretty Print for new nodes executed. Visually inspect output.");
}

// Runner function for AST tests
void run_ast_tests(void) {
    RUN_TEST(test_create_int_literal);
    RUN_TEST(test_create_return_stmt);
    RUN_TEST(test_create_unary_op_negate);
    RUN_TEST(test_create_unary_op_complement);
    RUN_TEST(test_create_binary_op_add);
    RUN_TEST(test_create_func_def); // Updated test
    RUN_TEST(test_create_program); // Updated test
    RUN_TEST(test_ast_pretty_print_output); // Updated test

    // New tests
    RUN_TEST(test_create_identifier_node);
    RUN_TEST(test_create_var_decl_node_no_initializer);
    RUN_TEST(test_create_var_decl_node_with_initializer);
    RUN_TEST(test_create_block_node_empty);
    RUN_TEST(test_block_node_add_items);
    RUN_TEST(test_create_func_def_with_block_body);
    RUN_TEST(test_ast_pretty_print_new_nodes);
}
