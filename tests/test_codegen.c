#include "unity/unity.h"
#include "../src/codegen/codegen.h"
#include "../src/parser/ast.h"
#include "../src/ir/tac.h" // Include for TacProgram and ast_to_tac
#include "../src/strings/strings.h"
#include "../src/memory/arena.h"
#include "../src/parser/parser.h"
#include "ir/ast_to_tac.h"

// --- Test Cases ---

// Test generating code for a simple function returning an integer literal
static void test_codegen_simple_return(void) {
    // 1. Create AST: program { func main() { return 42; } }
    Arena test_arena = arena_create(1024 * 4);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    IntLiteralNode *return_expr = create_int_literal_node(42, &test_arena);
    ReturnStmtNode *return_stmt = create_return_stmt_node((AstNode *) return_expr, &test_arena);
    FuncDefNode *func_def = create_func_def_node("main", (AstNode *) return_stmt, &test_arena);
    ProgramNode *program = create_program_node(func_def, &test_arena);

    // 2. Convert AST to TAC
    TacProgram *tac_program = ast_to_tac(program, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(tac_program, "ast_to_tac conversion failed");

    // 3. Setup Codegen State (just the buffer)
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 256);

    // 4. Generate Code from TAC
    bool success = codegen_generate_program(tac_program, &sb);

    // 5. Assertions
    TEST_ASSERT_TRUE_MESSAGE(success, "codegen_generate_program from TAC failed");

    // Expected assembly for: func main() { return 42; }
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"       // Placeholder stack allocation
            "    movl $42, %eax\n"      // Return 42
            "    leave\n"
            "    retq\n";

    const char *actual_asm = string_buffer_content_str(&sb); // Use read-only access
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL after content_str");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm, "Generated assembly mismatch");

    // 6. Cleanup
    arena_destroy(&test_arena);
}

// --- Unit Tests for operand_to_assembly_string ---

static void test_operand_to_assembly_string_const_ok(void) {
    TacOperand op;
    op.type = TAC_OPERAND_CONST;
    op.value.constant_value = 123;
    char buffer[64];
    bool success = operand_to_assembly_string(&op, buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("$123", buffer);
}

static void test_operand_to_assembly_string_temp_ok(void) {
    TacOperand op;
    op.type = TAC_OPERAND_TEMP;
    op.value.temp_id = 0; // t0
    char buffer[64];
    bool success = operand_to_assembly_string(&op, buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("-8(%rbp)", buffer); // t0 -> -(0+1)*8 = -8

    op.value.temp_id = 2; // t2
    success = operand_to_assembly_string(&op, buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_EQUAL_STRING("-24(%rbp)", buffer); // t2 -> -(2+1)*8 = -24
}

static void test_operand_to_assembly_string_null_operand(void) {
    char buffer[64];
    bool success = operand_to_assembly_string(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(success);
    // Optionally, check if buffer[0] is '\0' if the function guarantees it on error
    // TEST_ASSERT_EQUAL_CHAR('\0', buffer[0]);
}

static void test_operand_to_assembly_string_small_buffer(void) {
    TacOperand op;
    op.type = TAC_OPERAND_CONST;
    op.value.constant_value = 12345; // Needs $12345 (6 chars + null = 7)
    char buffer[6]; // Too small for "$12345"
    bool success = operand_to_assembly_string(&op, buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(success); // Should fail due to truncation / insufficient space
                                // The function should detect this via snprintf's return value.
}

static void test_operand_to_assembly_string_zero_buffer(void) {
    TacOperand op;
    op.type = TAC_OPERAND_CONST;
    op.value.constant_value = 7;
    char buffer[1]; // Minimal non-zero buffer to pass to function, but it will try to use size 0
    bool success = operand_to_assembly_string(&op, buffer, 0); // Pass buffer_size = 0
    TEST_ASSERT_FALSE(success);
}

static void test_operand_to_assembly_string_unhandled_type(void) {
    TacOperand op;
    op.type = (TacOperandType)99; // An unhandled type
    op.value.constant_value = 0;
    char buffer[64];
    bool success = operand_to_assembly_string(&op, buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_EQUAL_STRING("<UNHANDLED_OPERAND_TYPE_99>", buffer);
}

// --- Test Runner ---

void run_codegen_tests(void) {
    RUN_TEST(test_codegen_simple_return);
    // Add more RUN_TEST calls for other codegen tests here

    // Tests for operand_to_assembly_string
    RUN_TEST(test_operand_to_assembly_string_const_ok);
    RUN_TEST(test_operand_to_assembly_string_temp_ok);
    RUN_TEST(test_operand_to_assembly_string_null_operand);
    RUN_TEST(test_operand_to_assembly_string_small_buffer);
    RUN_TEST(test_operand_to_assembly_string_zero_buffer);
    RUN_TEST(test_operand_to_assembly_string_unhandled_type);
}
