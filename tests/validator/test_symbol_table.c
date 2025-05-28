#include "unity.h"
#include "validator/symbol_table.h"
#include "memory/arena.h"
#include "lexer/lexer.h" // For Token struct
#include "strings/strings.h" // For arena_strdup
#include <string.h> // For strcmp

// Helper to create a dummy token for tests
static Token create_dummy_token(Arena* arena, TokenType type, const char* lexeme, int line_unused, int col_unused) {
    Token t;
    t.type = type;
    // Duplicate the lexeme into the provided arena to avoid const issues and manage memory
    // ReSharper disable once CppDFAConstantConditions
    t.lexeme = lexeme ? arena_strdup(arena, lexeme) : NULL;
    t.position = 0; // Initialize position for the dummy token
    return t;
}

static void test_symbol_table_init_and_global_scope(void) {
    Arena local_arena = arena_create(1024 * 4);
    SymbolTable local_st;
    symbol_table_init(&local_st, &local_arena);

    TEST_ASSERT_EQUAL(1, local_st.scope_count); // Global scope should exist
    TEST_ASSERT_NOT_NULL(local_st.scopes);
    TEST_ASSERT_EQUAL(0, local_st.scopes[0].symbol_count);

    arena_destroy(&local_arena);
}

static void test_symbol_table_add_lookup_global(void) {
    Arena local_arena = arena_create(1024 * 4);
    SymbolTable local_st;
    symbol_table_init(&local_st, &local_arena);

    Token token_a = create_dummy_token(&local_arena, TOKEN_IDENTIFIER, "a", 1, 1);
    bool added = symbol_table_add_symbol(&local_st, "a", token_a);
    TEST_ASSERT_TRUE(added);
    TEST_ASSERT_EQUAL(1, local_st.scopes[local_st.scope_count - 1].symbol_count);

    const Symbol *found_symbol = symbol_table_lookup_symbol(&local_st, "a");
    TEST_ASSERT_NOT_NULL(found_symbol);
    TEST_ASSERT_EQUAL_STRING("a", found_symbol->name);

    arena_destroy(&local_arena);
}

static void test_symbol_table_lookup_not_found(void) {
    Arena local_arena = arena_create(1024 * 4);
    SymbolTable local_st;
    symbol_table_init(&local_st, &local_arena);

    const Symbol *found_symbol = symbol_table_lookup_symbol(&local_st, "nonexistent");
    TEST_ASSERT_NULL(found_symbol);

    arena_destroy(&local_arena);
}

static void test_symbol_table_redeclaration_current_scope(void) {
    Arena local_arena = arena_create(1024 * 4);
    SymbolTable local_st;
    symbol_table_init(&local_st, &local_arena);

    Token token_b1 = create_dummy_token(&local_arena, TOKEN_IDENTIFIER, "b", 2, 1);
    symbol_table_add_symbol(&local_st, "b", token_b1);

    Token token_b2 = create_dummy_token(&local_arena, TOKEN_IDENTIFIER, "b", 3, 1);
    bool added_again = symbol_table_add_symbol(&local_st, "b", token_b2);
    TEST_ASSERT_FALSE(added_again); // Should not allow re-declaration in the same scope
    TEST_ASSERT_EQUAL(1, local_st.scopes[local_st.scope_count - 1].symbol_count); // Count should remain 1

    arena_destroy(&local_arena);
}

static void test_symbol_table_enter_exit_scopes(void) {
    Arena local_arena = arena_create(1024 * 4);
    SymbolTable local_st;
    symbol_table_init(&local_st, &local_arena);

    TEST_ASSERT_EQUAL(1, local_st.scope_count); // Global scope

    symbol_table_enter_scope(&local_st);
    TEST_ASSERT_EQUAL(2, local_st.scope_count);

    Token token_c = create_dummy_token(&local_arena, TOKEN_IDENTIFIER, "c_inner", 4, 1);
    symbol_table_add_symbol(&local_st, "c_inner", token_c);
    const Symbol* found_c = symbol_table_lookup_symbol(&local_st, "c_inner");
    TEST_ASSERT_NOT_NULL(found_c);
    TEST_ASSERT_EQUAL_STRING("c_inner", found_c->name);

    symbol_table_exit_scope(&local_st);
    TEST_ASSERT_EQUAL(1, local_st.scope_count);
    const Symbol* found_c_after_exit = symbol_table_lookup_symbol(&local_st, "c_inner");
    TEST_ASSERT_NULL(found_c_after_exit); // c_inner should not be found in global scope

    arena_destroy(&local_arena);
}

static void test_symbol_table_shadowing_and_lookup_order(void) {
    Arena local_arena = arena_create(1024 * 4);
    SymbolTable local_st;
    symbol_table_init(&local_st, &local_arena);

    Token token_x_global = create_dummy_token(&local_arena, TOKEN_IDENTIFIER, "x", 5, 1);
    symbol_table_add_symbol(&local_st, "x", token_x_global);

    symbol_table_enter_scope(&local_st);
    Token token_x_local = create_dummy_token(&local_arena, TOKEN_IDENTIFIER, "x", 6, 1);
    symbol_table_add_symbol(&local_st, "x", token_x_local); // Shadowing x

    const Symbol* found_x = symbol_table_lookup_symbol(&local_st, "x");
    TEST_ASSERT_NOT_NULL(found_x);
    TEST_ASSERT_EQUAL_STRING("x", found_x->name);

    symbol_table_exit_scope(&local_st);
    const Symbol* found_x_global_again = symbol_table_lookup_symbol(&local_st, "x");
    TEST_ASSERT_NOT_NULL(found_x_global_again);

    arena_destroy(&local_arena);
}

static void test_symbol_table_lookup_in_current_scope_only(void) {
    Arena local_arena = arena_create(1024 * 4);
    SymbolTable local_st;
    symbol_table_init(&local_st, &local_arena);

    Token token_g = create_dummy_token(&local_arena, TOKEN_IDENTIFIER, "g_global", 7, 1);
    symbol_table_add_symbol(&local_st, "g_global", token_g);

    symbol_table_enter_scope(&local_st);
    Token token_l = create_dummy_token(&local_arena, TOKEN_IDENTIFIER, "l_local", 8, 1);
    symbol_table_add_symbol(&local_st, "l_local", token_l);

    const Symbol* found_l_curr = symbol_table_lookup_symbol_in_current_scope(&local_st, "l_local");
    TEST_ASSERT_NOT_NULL(found_l_curr);
    TEST_ASSERT_EQUAL_STRING("l_local", found_l_curr->name);

    const Symbol* found_g_curr = symbol_table_lookup_symbol_in_current_scope(&local_st, "g_global");
    TEST_ASSERT_NULL(found_g_curr); // g_global is not in the current (inner) scope

    const Symbol* found_g_any = symbol_table_lookup_symbol(&local_st, "g_global");
    TEST_ASSERT_NOT_NULL(found_g_any); // but it is findable in any scope

    symbol_table_exit_scope(&local_st);

    arena_destroy(&local_arena);
}


void run_symbol_table_tests(void) {
    RUN_TEST(test_symbol_table_init_and_global_scope);
    RUN_TEST(test_symbol_table_add_lookup_global);
    RUN_TEST(test_symbol_table_lookup_not_found);
    RUN_TEST(test_symbol_table_redeclaration_current_scope);
    RUN_TEST(test_symbol_table_enter_exit_scopes);
    RUN_TEST(test_symbol_table_shadowing_and_lookup_order);
    RUN_TEST(test_symbol_table_lookup_in_current_scope_only);
}
