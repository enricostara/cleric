#include "unity/unity.h"
#include "../src/codegen/codegen.h"
#include "../src/parser/ast.h"
#include "../src/ir/tac.h" // Include for TacProgram and ast_to_tac
#include "../src/strings/strings.h"
#include "../src/memory/arena.h"
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
            "    subq $32, %rsp\n" // Placeholder stack allocation
            "    movl $42, %eax\n" // Return 42
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
    op.type = (TacOperandType) 99; // An unhandled type
    op.value.constant_value = 0;
    char buffer[64];
    bool success = operand_to_assembly_string(&op, buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(success);
    TEST_ASSERT_EQUAL_STRING("<UNHANDLED_OPERAND_TYPE_99>", buffer);
}

// Test for TAC_INS_COPY: const to temp, then return temp
static void test_codegen_copy_const_to_temp_and_return(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for copy_const_to_temp_and_return");

    // 1. Create TAC Program Structure
    TacProgram *tac_program = create_tac_program(&test_arena);
    TacFunction *func_main = create_tac_function("main", &test_arena);

    // 2. Create Operands
    TacOperand op_dest_temp0 = create_tac_operand_temp(0); // t0
    TacOperand op_src_const123 = create_tac_operand_const(123); // $123

    // 3. Create TAC Instructions
    // Instruction 1: t0 = 123 (COPY t0, $123)
    TacInstruction *instr_copy = create_tac_instruction_copy(op_dest_temp0, op_src_const123, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(instr_copy, "Failed to create TAC_INS_COPY instruction");
    add_instruction_to_function(func_main, instr_copy, &test_arena);

    // Instruction 2: return t0 (RETURN t0)
    TacInstruction *instr_return = create_tac_instruction_return(op_dest_temp0, &test_arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(instr_return, "Failed to create TAC_INS_RETURN instruction");
    add_instruction_to_function(func_main, instr_return, &test_arena);

    add_function_to_program(tac_program, func_main, &test_arena);

    // 4. Setup Codegen State
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 256);

    // 5. Generate Code from TAC
    bool success = codegen_generate_program(tac_program, &sb);

    // 6. Assertions
    TEST_ASSERT_TRUE_MESSAGE(success, "codegen_generate_program for copy_const_to_temp_and_return failed");

    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n" // Placeholder stack allocation
            "    movl $123, -8(%rbp)\n" // COPY t0, $123 (t0 -> -8(%rbp))
            "    movl -8(%rbp), %eax\n" // RETURN t0
            "    leave\n"
            "    retq\n";

    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_asm, "Output assembly buffer is NULL for copy_const_to_temp_and_return");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm,
                                     "Generated assembly mismatch for copy_const_to_temp_and_return");

    // 7. Cleanup
    arena_destroy(&test_arena);
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

    // Test for TAC_INS_COPY
    RUN_TEST(test_codegen_copy_const_to_temp_and_return);
}

// Placeholder for the main function for running tests individually if needed
