#include "unity/unity.h"
#include "../src/codegen/codegen.h"
#include "../src/parser/ast.h"
#include "../src/ir/tac.h" // Include for TacProgram and ast_to_tac
#include "../src/strings/strings.h"
#include "../src/memory/arena.h"
#include "ir/ast_to_tac.h"

// --- Helper Functions for Testing Codegen ---

// Helper function to verify assembly generation for a given TacFunction
static void verify_asm_for_function(
    const char* test_description,      // For TEST_ASSERT_EQUAL_STRING_MESSAGE
    TacFunction* func,                 // Caller creates and populates this with an arena
    Arena* arena,                      // The arena used by the caller for func and its instructions
    const char* expected_assembly
) {
    // 1. Create TacProgram (using the same arena as the function)
    TacProgram *tac_program = create_tac_program(arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(tac_program, "Failed to create TacProgram in verify_asm_for_function");
    add_function_to_program(tac_program, func, arena); // func is already arena-allocated by caller

    // 2. Setup Codegen State (StringBuffer, also using the same arena)
    StringBuffer sb;
    string_buffer_init(&sb, arena, 1024); // Increased buffer size for potentially larger outputs

    // 3. Generate Code from TAC
    bool success = codegen_generate_program(tac_program, &sb);
    TEST_ASSERT_TRUE_MESSAGE(success, "codegen_generate_program from TAC failed in verify_asm_for_function");

    // 4. Assertions
    const char *actual_assembly = string_buffer_content_str(&sb);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual_assembly, "Output assembly buffer is NULL in verify_asm_for_function");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_assembly, actual_assembly, test_description);

    // Arena cleanup is handled by the caller of this helper (the test function itself)
}

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

// --- Unit Tests for calculate_max_temp_id ---

static void test_calculate_max_temp_id_null_function(void) {
    TEST_ASSERT_EQUAL_INT(-1, calculate_max_temp_id(NULL));
}

static void test_calculate_max_temp_id_no_instructions(void) {
    // Arena not strictly needed here as func is stack-allocated and calculate_max_temp_id doesn't use an arena.
    TacFunction func = {.name = "empty_func", .instructions = NULL, .instruction_count = 0, .instruction_capacity = 0};
    TEST_ASSERT_EQUAL_INT(-1, calculate_max_temp_id(&func));
}

static void test_calculate_max_temp_id_no_temporaries(void) {
    Arena arena = arena_create(1024); // Increased from 256
    TacFunction *func = create_tac_function("no_temps_func", &arena);

    TacOperand const_op = create_tac_operand_const(5);
    TacInstruction *ret_instr = create_tac_instruction_return(const_op, &arena);
    add_instruction_to_function(func, ret_instr, &arena);

    TEST_ASSERT_EQUAL_INT(-1, calculate_max_temp_id(func));
    arena_destroy(&arena);
}

static void test_calculate_max_temp_id_one_temporary_dst(void) {
    Arena arena = arena_create(1024); // Increased from 256
    TacFunction *func = create_tac_function("one_temp_dst_func", &arena);

    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const_op = create_tac_operand_const(5);
    TacInstruction *copy_instr = create_tac_instruction_copy(t0, const_op, &arena);
    add_instruction_to_function(func, copy_instr, &arena);
    // Add a return to make it a valid function for codegen if we ever used it for that
    TacInstruction *ret_instr = create_tac_instruction_return(t0, &arena);
    add_instruction_to_function(func, ret_instr, &arena);

    TEST_ASSERT_EQUAL_INT(0, calculate_max_temp_id(func));
    arena_destroy(&arena);
}

static void test_calculate_max_temp_id_one_temporary_src(void) {
    Arena arena = arena_create(1024); // Increased from 256
    TacFunction *func = create_tac_function("one_temp_src_func", &arena);

    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand const_op = create_tac_operand_const(5);

    // t0 = 5 (to have t0 defined)
    TacInstruction *copy_instr1 = create_tac_instruction_copy(t0, const_op, &arena);
    add_instruction_to_function(func, copy_instr1, &arena);

    // t1 = t0
    TacInstruction *copy_instr2 = create_tac_instruction_copy(t1, t0, &arena);
    add_instruction_to_function(func, copy_instr2, &arena);

    TacInstruction *ret_instr = create_tac_instruction_return(t1, &arena);
    add_instruction_to_function(func, ret_instr, &arena);

    TEST_ASSERT_EQUAL_INT(1, calculate_max_temp_id(func));
    arena_destroy(&arena);
}

static void test_calculate_max_temp_id_mixed_operands(void) {
    Arena arena = arena_create(1024);
    TacFunction *func = create_tac_function("mixed_ops_func", &arena);

    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand t2 = create_tac_operand_temp(2);
    TacOperand const10 = create_tac_operand_const(10);
    TacOperand const20 = create_tac_operand_const(20);

    // COPY t1, $10
    add_instruction_to_function(func, create_tac_instruction_copy(t1, const10, &arena), &arena);
    // COPY t0, $20
    add_instruction_to_function(func, create_tac_instruction_copy(t0, const20, &arena), &arena);
    // NEGATE t2, t1
    add_instruction_to_function(func, create_tac_instruction_negate(t2, t1, &arena), &arena);
    // RETURN t0
    add_instruction_to_function(func, create_tac_instruction_return(t0, &arena), &arena);

    TEST_ASSERT_EQUAL_INT(2, calculate_max_temp_id(func));
    arena_destroy(&arena);
}

static void test_calculate_max_temp_id_temp_ids_not_sequential(void) {
    Arena arena = arena_create(1024); // Increased from 256
    TacFunction *func = create_tac_function("non_seq_temps_func", &arena);

    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t3 = create_tac_operand_temp(3);
    TacOperand const1 = create_tac_operand_const(1);

    // COPY t3, $1
    add_instruction_to_function(func, create_tac_instruction_copy(t3, const1, &arena), &arena);
    // RETURN t0 (t0 is used, max is still 3)
    add_instruction_to_function(func, create_tac_instruction_return(t0, &arena), &arena);

    TEST_ASSERT_EQUAL_INT(3, calculate_max_temp_id(func));
    arena_destroy(&arena);
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

// Test for TAC_INS_NEGATE: t0 = 5; t1 = -t0; return t1;
static void test_codegen_negate_temp_from_temp(void) {
    Arena arena = arena_create(2048);
    TEST_ASSERT_NOT_NULL(arena.start);

    TacProgram *prog = create_tac_program(&arena);
    TacFunction *func = create_tac_function("main", &arena);
    add_function_to_program(prog, func, &arena);

    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand const5 = create_tac_operand_const(5);

    // t0 = 5
    add_instruction_to_function(func, create_tac_instruction_copy(t0, const5, &arena), &arena);
    // t1 = -t0
    add_instruction_to_function(func, create_tac_instruction_negate(t1, t0, &arena), &arena);
    // return t1
    add_instruction_to_function(func, create_tac_instruction_return(t1, &arena), &arena);

    StringBuffer sb;
    string_buffer_init(&sb, &arena, 256);
    bool success = codegen_generate_program(prog, &sb);
    TEST_ASSERT_TRUE(success);

    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $5, -8(%rbp)\n" // t0 = 5
            "    movl -8(%rbp), %eax\n" // Load t0 into %eax
            "    negl %eax\n" // Negate %eax
            "    movl %eax, -16(%rbp)\n" // Store result in t1
            "    movl -16(%rbp), %eax\n" // Load t1 into %eax for return
            "    leave\n"
            "    retq\n";
    TEST_ASSERT_EQUAL_STRING(expected_asm, string_buffer_content_str(&sb));
    arena_destroy(&arena);
}

// Test for TAC_INS_COMPLEMENT: t0 = 10; t0 = ~t0; return t0;
static void test_codegen_complement_temp_in_place(void) {
    Arena arena = arena_create(2048);
    TEST_ASSERT_NOT_NULL(arena.start);

    TacProgram *prog = create_tac_program(&arena);
    TacFunction *func = create_tac_function("main", &arena);
    add_function_to_program(prog, func, &arena);

    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const10 = create_tac_operand_const(10);

    // t0 = 10
    add_instruction_to_function(func, create_tac_instruction_copy(t0, const10, &arena), &arena);
    // t0 = ~t0
    add_instruction_to_function(func, create_tac_instruction_complement(t0, t0, &arena), &arena);
    // return t0
    add_instruction_to_function(func, create_tac_instruction_return(t0, &arena), &arena);

    StringBuffer sb;
    string_buffer_init(&sb, &arena, 256);
    bool success = codegen_generate_program(prog, &sb);
    TEST_ASSERT_TRUE(success);

    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $10, -8(%rbp)\n" // t0 = 10
            "    notl -8(%rbp)\n" // t0 = ~t0 (in-place)
            "    movl -8(%rbp), %eax\n" // return t0
            "    leave\n"
            "    retq\n";
    TEST_ASSERT_EQUAL_STRING(expected_asm, string_buffer_content_str(&sb));
    arena_destroy(&arena);
}

// Test for return ~(-2);  (Should be -3)
static void test_codegen_complement_of_negated_constant(void) {
    Arena arena = arena_create(2048);
    TEST_ASSERT_NOT_NULL(arena.start);

    TacProgram *prog = create_tac_program(&arena);
    TacFunction *func = create_tac_function("main", &arena);
    add_function_to_program(prog, func, &arena);

    TacOperand t0 = create_tac_operand_temp(0); // Will hold -2
    TacOperand t1 = create_tac_operand_temp(1); // Will hold -t0 = 2
    TacOperand t2 = create_tac_operand_temp(2); // Will hold ~t1 = ~2 = -3
    TacOperand const_neg_2 = create_tac_operand_const(-2);

    // 1. t0 = -2
    add_instruction_to_function(func, create_tac_instruction_copy(t0, const_neg_2, &arena), &arena);
    // 2. t1 = -t0  (t1 = -(-2) = 2)
    add_instruction_to_function(func, create_tac_instruction_negate(t1, t0, &arena), &arena);
    // 3. t2 = ~t1  (t2 = ~(2) = -3)
    add_instruction_to_function(func, create_tac_instruction_complement(t2, t1, &arena), &arena);
    // 4. return t2
    add_instruction_to_function(func, create_tac_instruction_return(t2, &arena), &arena);

    StringBuffer sb;
    string_buffer_init(&sb, &arena, 512); // Increased buffer size a bit
    bool success = codegen_generate_program(prog, &sb);
    TEST_ASSERT_TRUE(success);

    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n" // Stack frame for t0, t1, t2
            "    movl $-2, -8(%rbp)\n" // t0 = -2
            "    movl -8(%rbp), %eax\n" // eax = t0
            "    negl %eax\n" // eax = -eax (eax = 2)
            "    movl %eax, -16(%rbp)\n" // t1 = eax (t1 = 2)
            "    movl -16(%rbp), %eax\n" // eax = t1
            "    notl %eax\n" // eax = ~eax (eax = ~2)
            "    movl %eax, -24(%rbp)\n" // t2 = eax (t2 = ~2)
            "    movl -24(%rbp), %eax\n" // eax = t2 (for return)
            "    leave\n"
            "    retq\n";
    TEST_ASSERT_EQUAL_STRING(expected_asm, string_buffer_content_str(&sb));
    arena_destroy(&arena);
}

// Test for stack allocation with multiple temporaries (t0-t4, expecting 48 bytes)
static void test_codegen_stack_allocation_for_many_temps(void) {
    Arena arena = arena_create(1024 * 4);
    TEST_ASSERT_NOT_NULL(arena.start);

    TacProgram *prog = create_tac_program(&arena);
    TacFunction *func = create_tac_function("main", &arena);
    add_function_to_program(prog, func, &arena);

    // Create 5 temporaries t0, t1, t2, t3, t4
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand t2 = create_tac_operand_temp(2);
    TacOperand t3 = create_tac_operand_temp(3);
    TacOperand t4 = create_tac_operand_temp(4);

    // Create constants
    TacOperand const0 = create_tac_operand_const(0);
    TacOperand const1 = create_tac_operand_const(1);
    TacOperand const2 = create_tac_operand_const(2);
    TacOperand const3 = create_tac_operand_const(3);
    TacOperand const4 = create_tac_operand_const(4);

    // t0 = 0
    add_instruction_to_function(func, create_tac_instruction_copy(t0, const0, &arena), &arena);
    // t1 = 1
    add_instruction_to_function(func, create_tac_instruction_copy(t1, const1, &arena), &arena);
    // t2 = 2
    add_instruction_to_function(func, create_tac_instruction_copy(t2, const2, &arena), &arena);
    // t3 = 3
    add_instruction_to_function(func, create_tac_instruction_copy(t3, const3, &arena), &arena);
    // t4 = 4
    add_instruction_to_function(func, create_tac_instruction_copy(t4, const4, &arena), &arena);
    // return t4
    add_instruction_to_function(func, create_tac_instruction_return(t4, &arena), &arena);

    StringBuffer sb;
    string_buffer_init(&sb, &arena, 512); // Increased buffer size for more instructions
    bool success = codegen_generate_program(prog, &sb);
    TEST_ASSERT_TRUE(success);

    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $48, %rsp\n" // 5 temps (t0-t4) -> max_id=4 -> (4+1)*8=40 bytes -> aligned to 48
            "    movl $0, -8(%rbp)\n" // t0 = 0
            "    movl $1, -16(%rbp)\n" // t1 = 1
            "    movl $2, -24(%rbp)\n" // t2 = 2
            "    movl $3, -32(%rbp)\n" // t3 = 3
            "    movl $4, -40(%rbp)\n" // t4 = 4
            "    movl -40(%rbp), %eax\n" // return t4
            "    leave\n"
            "    retq\n";

    const char *actual_asm = string_buffer_content_str(&sb);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm,
                                     "Generated assembly mismatch for many temps stack allocation");

    arena_destroy(&arena);
}

// --- New Tests for Relational and Conditional Jump Codegen ---

static void test_codegen_relational_equal_consts_true(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_equal_consts_true");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5_1 = create_tac_operand_const(5);
    TacOperand const5_2 = create_tac_operand_const(5);

    // Instructions
    // Note: create_tac_instruction_* returns a pointer, but add_instruction_to_function might copy it
    // or the TacFunction might own them. Let's stick to the pattern of creating them directly for the function.
    TacInstruction* instr1 = create_tac_instruction_equal(t0, const5_1, const5_2, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    // Create TacFunction
    TacFunction *func = create_tac_function("test_eq_true", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    // Expected Assembly (max_temp_id=0 -> (0+1)*8=8 bytes, aligned to 16 is 16, min 32 bytes)
    const char *expected_asm =
        ".globl _test_eq_true\n"
        "_test_eq_true:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $5, %eax\n"
        "    cmpl $5, %eax\n"
        "    sete %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_equal_consts_true: t0 = (5 == 5); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_equal_consts_false(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_equal_consts_false");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const6 = create_tac_operand_const(6);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_equal(t0, const5, const6, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_eq_false", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_eq_false\n"
        "_test_eq_false:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $5, %eax\n"
        "    cmpl $6, %eax\n"
        "    sete %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_equal_consts_false: t0 = (5 == 6); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

// Test for return -(5 + (-2));  (Should be -3)
static void test_codegen_return_negated_parenthesized_constant(void) {
    Arena arena = arena_create(1024 * 4);
    TEST_ASSERT_NOT_NULL(arena.start);

    TacProgram *prog = create_tac_program(&arena);
    TacFunction *func = create_tac_function("main", &arena);
    add_function_to_program(prog, func, &arena);

    // Operands
    TacOperand t0 = create_tac_operand_temp(0); // To hold 5 + (-2)
    TacOperand t1 = create_tac_operand_temp(1); // To hold -(t0)
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const_neg_2 = create_tac_operand_const(-2);

    // 1. t0 = 5 + (-2)  => t0 = 3
    add_instruction_to_function(func, create_tac_instruction_add(t0, const5, const_neg_2, &arena), &arena);
    // 2. t1 = -t0      => t1 = -3
    add_instruction_to_function(func, create_tac_instruction_negate(t1, t0, &arena), &arena);
    // 3. return t1
    add_instruction_to_function(func, create_tac_instruction_return(t1, &arena), &arena);

    StringBuffer sb;
    string_buffer_init(&sb, &arena, 512);
    bool success = codegen_generate_program(prog, &sb);
    TEST_ASSERT_TRUE(success);

    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n" // For t0, t1 (max_id=1 -> 2 temps * 8 = 16, aligned to 16, min 32)
            "    movl $5, %eax\n"
            "    addl $-2, %eax\n"
            "    movl %eax, -8(%rbp)\n"  // t0 = 3
            "    movl -8(%rbp), %eax\n"
            "    negl %eax\n"
            "    movl %eax, -16(%rbp)\n" // t1 = -3
            "    movl -16(%rbp), %eax\n" // return t1
            "    leave\n"
            "    retq\n";
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, string_buffer_content_str(&sb), "Test for -(5 + (-2)) failed.");
    arena_destroy(&arena);
}

// --- New Tests for Relational and Conditional Jump Codegen ---

static void test_codegen_relational_not_equal_consts_true(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_not_equal_consts_true");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const6 = create_tac_operand_const(6);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_not_equal(t0, const5, const6, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_ne_true", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_ne_true\n"
        "_test_ne_true:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $5, %eax\n"
        "    cmpl $6, %eax\n"
        "    setne %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_not_equal_consts_true: t0 = (5 != 6); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_not_equal_consts_false(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_not_equal_consts_false");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5_1 = create_tac_operand_const(5);
    TacOperand const5_2 = create_tac_operand_const(5);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_not_equal(t0, const5_1, const5_2, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_ne_false", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_ne_false\n"
        "_test_ne_false:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $5, %eax\n"
        "    cmpl $5, %eax\n"
        "    setne %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_not_equal_consts_false: t0 = (5 != 5); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_less_consts_true_less(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_less_consts_true_less");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const6 = create_tac_operand_const(6);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_less(t0, const5, const6, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_lt_true", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_lt_true\n"
        "_test_lt_true:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $5, %eax\n"
        "    cmpl $6, %eax\n"
        "    setl %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_less_consts_true_less: t0 = (5 < 6); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_less_consts_false_equal(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_less_consts_false_equal");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5_1 = create_tac_operand_const(5);
    TacOperand const5_2 = create_tac_operand_const(5);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_less(t0, const5_1, const5_2, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_lt_false_eq", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_lt_false_eq\n"
        "_test_lt_false_eq:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $5, %eax\n"
        "    cmpl $5, %eax\n"
        "    setl %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_less_consts_false_equal: t0 = (5 < 5); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_less_equal_consts_true_less(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_less_equal_consts_true_less");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const6 = create_tac_operand_const(6);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_less_equal(t0, const5, const6, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_le_true_lt", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_le_true_lt\n"
        "_test_le_true_lt:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $5, %eax\n"
        "    cmpl $6, %eax\n"
        "    setle %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_less_equal_consts_true_less: t0 = (5 <= 6); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_less_consts_false_greater(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_less_consts_false_greater");

    // Operands: t0 = (10 < 5) -> t0 = 0
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const10 = create_tac_operand_const(10); // src1
    TacOperand const5 = create_tac_operand_const(5);   // src2

    // Instructions
    // t0 = const10 < const5
    TacInstruction* instr1 = create_tac_instruction_less_equal(t0, const10, const5, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_less_false_greater", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
       ".globl _test_less_false_greater\n"
       "_test_less_false_greater:\n"
       "    pushq %rbp\n"
       "    movq %rsp, %rbp\n"
       "    subq $32, %rsp\n"
       "    movl $10, %eax\n"        // src1 (10) into eax
       "    cmpl $5, %eax\n"         // compare eax with src2 (5) - This was the pattern in your actual output
       "    setl %al\n"              // Should be setl for TAC_INS_LESS
       "    movzbl %al, %eax\n"
       "    movl %eax, -8(%rbp)\n"   // Store result (0) into t0
       "    movl -8(%rbp), %eax\n"   // Load t0 for return
       "    leave\n"
       "    retq\n";

    verify_asm_for_function("test_codegen_relational_less_consts_false_greater: t0 = (10 < 5)",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_less_equal_consts_true_equal(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_less_equal_consts_true_equal");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5_1 = create_tac_operand_const(5);
    TacOperand const5_2 = create_tac_operand_const(5);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_less_equal(t0, const5_1, const5_2, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_le_true_eq", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_le_true_eq\n"
        "_test_le_true_eq:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $5, %eax\n"
        "    cmpl $5, %eax\n"
        "    setle %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_less_equal_consts_true_equal: t0 = (5 <= 5); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_less_equal_consts_false_greater(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_less_equal_consts_false_greater");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const6 = create_tac_operand_const(6);
    TacOperand const5 = create_tac_operand_const(5);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_less_equal(t0, const6, const5, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_le_false_gt", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_le_false_gt\n"
        "_test_le_false_gt:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $6, %eax\n"
        "    cmpl $5, %eax\n"
        "    setle %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_less_equal_consts_false_greater: t0 = (6 <= 5); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_greater_consts_true_greater(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_greater_consts_true_greater");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const6 = create_tac_operand_const(6);
    TacOperand const5 = create_tac_operand_const(5);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_greater(t0, const6, const5, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_gt_true_gt", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_gt_true_gt\n"
        "_test_gt_true_gt:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $6, %eax\n"
        "    cmpl $5, %eax\n"
        "    setg %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_greater_consts_true_greater: t0 = (6 > 5); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_greater_consts_false_equal(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_greater_consts_false_equal");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5_1 = create_tac_operand_const(5);
    TacOperand const5_2 = create_tac_operand_const(5);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_greater(t0, const5_1, const5_2, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_gt_false_eq", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_gt_false_eq\n"
        "_test_gt_false_eq:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $5, %eax\n"
        "    cmpl $5, %eax\n"
        "    setg %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_greater_consts_false_equal: t0 = (5 > 5); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_greater_consts_false_less(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_greater_consts_false_less");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const6 = create_tac_operand_const(6);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_greater(t0, const5, const6, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_gt_false_lt", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_gt_false_lt\n"
        "_test_gt_false_lt:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $5, %eax\n"
        "    cmpl $6, %eax\n"
        "    setg %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_greater_consts_false_less: t0 = (5 > 6); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_greater_equal_consts_true_greater(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_greater_equal_consts_true_greater");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const6 = create_tac_operand_const(6);
    TacOperand const5 = create_tac_operand_const(5);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_greater_equal(t0, const6, const5, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_ge_true_gt", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_ge_true_gt\n"
        "_test_ge_true_gt:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $6, %eax\n"
        "    cmpl $5, %eax\n"
        "    setge %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_greater_equal_consts_true_greater: t0 = (6 >= 5); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_greater_equal_consts_true_equal(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_greater_equal_consts_true_equal");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5_1 = create_tac_operand_const(5);
    TacOperand const5_2 = create_tac_operand_const(5);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_greater_equal(t0, const5_1, const5_2, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_ge_true_eq", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_ge_true_eq\n"
        "_test_ge_true_eq:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $5, %eax\n"
        "    cmpl $5, %eax\n"
        "    setge %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_greater_equal_consts_true_equal: t0 = (5 >= 5); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_relational_greater_equal_consts_false_less(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_relational_greater_equal_consts_false_less");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const5 = create_tac_operand_const(5);
    TacOperand const6 = create_tac_operand_const(6);

    // Instructions
    TacInstruction* instr1 = create_tac_instruction_greater_equal(t0, const5, const6, &test_arena);
    TacInstruction* instr2 = create_tac_instruction_return(t0, &test_arena);

    TacFunction *func = create_tac_function("test_ge_false_lt", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);

    const char *expected_asm =
        ".globl _test_ge_false_lt\n"
        "_test_ge_false_lt:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $5, %eax\n"
        "    cmpl $6, %eax\n"
        "    setge %al\n"
        "    movzbl %al, %eax\n"
        "    movl %eax, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_relational_greater_equal_consts_false_less: t0 = (5 >= 6); return t0;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

// --- Test for TAC_INS_IF_FALSE_GOTO ---

static void test_codegen_if_false_goto_jumps(void) {
    Arena test_arena = arena_create(1024 * 2);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_if_false_goto_jumps");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand const0 = create_tac_operand_const(0);
    TacOperand const10 = create_tac_operand_const(10);
    TacOperand const20 = create_tac_operand_const(20);
    TacOperand label0 = create_tac_operand_label("_L0");

    // Instructions
    // t0 = 0
    TacInstruction* instr1 = create_tac_instruction_copy(t0, const0, &test_arena);
    // IF_FALSE t0 GOTO L0
    TacInstruction* instr2 = create_tac_instruction_if_false_goto(t0, label0, &test_arena);
    // t1 = 10 (skipped)
    TacInstruction* instr3 = create_tac_instruction_copy(t1, const10, &test_arena);
    // L0:
    TacInstruction* instr4 = create_tac_instruction_label(label0, &test_arena);
    // t1 = 20
    TacInstruction* instr5 = create_tac_instruction_copy(t1, const20, &test_arena);
    // RETURN t1
    TacInstruction* instr6 = create_tac_instruction_return(t1, &test_arena);

    TacFunction *func = create_tac_function("test_if_false_jumps", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);
    add_instruction_to_function(func, instr3, &test_arena);
    add_instruction_to_function(func, instr4, &test_arena);
    add_instruction_to_function(func, instr5, &test_arena);
    add_instruction_to_function(func, instr6, &test_arena);

    const char *expected_asm =
        ".globl _test_if_false_jumps\n"
        "_test_if_false_jumps:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $0, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    testl %eax, %eax\n"
        "    jz _L0\n"
        "    movl $10, -16(%rbp)\n"
        "_L0:\n"
        "    movl $20, -16(%rbp)\n"
        "    movl -16(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_if_false_goto_jumps: t0=0; IF_FALSE t0 GOTO L0; t1=10; L0: t1=20; RETURN t1;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_if_false_goto_no_jump(void) {
    Arena test_arena = arena_create(1024 * 4);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_if_false_goto_no_jump");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand const1 = create_tac_operand_const(1);
    TacOperand const10 = create_tac_operand_const(10);
    TacOperand const20 = create_tac_operand_const(20);
    TacOperand label0 = create_tac_operand_label("_L0");
    TacOperand label1 = create_tac_operand_label("_L1");

    // Instructions
    // t0 = 1 (true)
    TacInstruction* instr1 = create_tac_instruction_copy(t0, const1, &test_arena);
    // IF_FALSE t0 GOTO _L0 (no jump)
    TacInstruction* instr2 = create_tac_instruction_if_false_goto(t0, label0, &test_arena);
    // t1 = 10 (executed)
    TacInstruction* instr3 = create_tac_instruction_copy(t1, const10, &test_arena);
    // GOTO _L1
    TacInstruction* instr4 = create_tac_instruction_goto(label1, &test_arena);
    // _L0: (skipped section)
    TacInstruction* instr5 = create_tac_instruction_label(label0, &test_arena);
    // t1 = 20 (skipped)
    TacInstruction* instr6 = create_tac_instruction_copy(t1, const20, &test_arena);
    // _L1: (continue execution)
    TacInstruction* instr7 = create_tac_instruction_label(label1, &test_arena);
    // RETURN t1 (should be 10)
    TacInstruction* instr8 = create_tac_instruction_return(t1, &test_arena);

    TacFunction *func = create_tac_function("test_if_false_no_jump", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);
    add_instruction_to_function(func, instr3, &test_arena);
    add_instruction_to_function(func, instr4, &test_arena);
    add_instruction_to_function(func, instr5, &test_arena);
    add_instruction_to_function(func, instr6, &test_arena);
    add_instruction_to_function(func, instr7, &test_arena);
    add_instruction_to_function(func, instr8, &test_arena);

    const char *expected_asm =
        ".globl _test_if_false_no_jump\n"
        "_test_if_false_no_jump:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $1, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    testl %eax, %eax\n"
        "    jz _L0\n"
        "    movl $10, -16(%rbp)\n"
        "    jmp _L1\n"
        "_L0:\n"
        "    movl $20, -16(%rbp)\n"
        "_L1:\n"
        "    movl -16(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_if_false_goto_no_jump: t0=1; IF_FALSE t0 GOTO L0; t1=10; GOTO L1; L0: t1=20; L1: RETURN t1;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_if_true_goto_jumps(void) {
    Arena test_arena = arena_create(1024 * 2); // Start with 2KB, can increase if needed
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_if_true_goto_jumps");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand const1 = create_tac_operand_const(1);   // For t0 = 1 (true condition)
    TacOperand const10 = create_tac_operand_const(10);
    TacOperand const20 = create_tac_operand_const(20);
    TacOperand label0 = create_tac_operand_label("_L0");

    // Instructions
    // t0 = 1
    TacInstruction* instr1 = create_tac_instruction_copy(t0, const1, &test_arena);
    // IF_TRUE t0 GOTO _L0 (jump occurs because t0 is true)
    TacInstruction* instr2 = create_tac_instruction_if_true_goto(t0, label0, &test_arena);
    // t1 = 10 (skipped)
    TacInstruction* instr3 = create_tac_instruction_copy(t1, const10, &test_arena);
    // _L0:
    TacInstruction* instr4 = create_tac_instruction_label(label0, &test_arena);
    // t1 = 20 (executed)
    TacInstruction* instr5 = create_tac_instruction_copy(t1, const20, &test_arena);
    // RETURN t1 (should be 20)
    TacInstruction* instr6 = create_tac_instruction_return(t1, &test_arena);

    TacFunction *func = create_tac_function("test_if_true_jumps", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);
    add_instruction_to_function(func, instr3, &test_arena);
    add_instruction_to_function(func, instr4, &test_arena);
    add_instruction_to_function(func, instr5, &test_arena);
    add_instruction_to_function(func, instr6, &test_arena);

    const char *expected_asm =
        ".globl _test_if_true_jumps\n"
        "_test_if_true_jumps:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $1, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    testl %eax, %eax\n"
        "    jnz _L0\n"
        "    movl $10, -16(%rbp)\n"
        "_L0:\n"
        "    movl $20, -16(%rbp)\n"
        "    movl -16(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_if_true_goto_jumps: t0=1; IF_TRUE t0 GOTO L0; t1=10; L0: t1=20; RETURN t1;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

static void test_codegen_if_true_goto_no_jump(void) {
    Arena test_arena = arena_create(1024 * 4); // Matching increased size from similar test
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for test_codegen_if_true_goto_no_jump");

    // Operands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand const0 = create_tac_operand_const(0);   // For t0 = 0 (false condition)
    TacOperand const10 = create_tac_operand_const(10);
    TacOperand const20 = create_tac_operand_const(20);
    TacOperand label0 = create_tac_operand_label("_L0");
    TacOperand label1 = create_tac_operand_label("_L1");

    // Instructions
    // t0 = 0
    TacInstruction* instr1 = create_tac_instruction_copy(t0, const0, &test_arena);
    // IF_TRUE t0 GOTO _L0 (no jump because t0 is false)
    TacInstruction* instr2 = create_tac_instruction_if_true_goto(t0, label0, &test_arena);
    // t1 = 10 (executed)
    TacInstruction* instr3 = create_tac_instruction_copy(t1, const10, &test_arena);
    // GOTO _L1
    TacInstruction* instr4 = create_tac_instruction_goto(label1, &test_arena);
    // _L0: (skipped section)
    TacInstruction* instr5 = create_tac_instruction_label(label0, &test_arena);
    // t1 = 20 (skipped)
    TacInstruction* instr6 = create_tac_instruction_copy(t1, const20, &test_arena);
    // _L1: (continue execution)
    TacInstruction* instr7 = create_tac_instruction_label(label1, &test_arena);
    // RETURN t1 (should be 10)
    TacInstruction* instr8 = create_tac_instruction_return(t1, &test_arena);

    TacFunction *func = create_tac_function("test_if_true_no_jump", &test_arena);
    add_instruction_to_function(func, instr1, &test_arena);
    add_instruction_to_function(func, instr2, &test_arena);
    add_instruction_to_function(func, instr3, &test_arena);
    add_instruction_to_function(func, instr4, &test_arena);
    add_instruction_to_function(func, instr5, &test_arena);
    add_instruction_to_function(func, instr6, &test_arena);
    add_instruction_to_function(func, instr7, &test_arena);
    add_instruction_to_function(func, instr8, &test_arena);

    const char *expected_asm =
        ".globl _test_if_true_no_jump\n"
        "_test_if_true_no_jump:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"
        "    movl $0, -8(%rbp)\n"
        "    movl -8(%rbp), %eax\n"
        "    testl %eax, %eax\n"
        "    jnz _L0\n"
        "    movl $10, -16(%rbp)\n"
        "    jmp _L1\n"
        "_L0:\n"
        "    movl $20, -16(%rbp)\n"
        "_L1:\n"
        "    movl -16(%rbp), %eax\n"
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_if_true_goto_no_jump: t0=0; IF_TRUE t0 GOTO L0; t1=10; GOTO L1; L0: t1=20; L1: RETURN t1;",
                              func, &test_arena, expected_asm);

    arena_destroy(&test_arena);
}

// --- Tests for TAC_INS_LOGICAL_NOT ---

static void test_codegen_logical_not_const_false_input(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_not_const_false");

    // TAC: t0 = !0; RETURN t0
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const0 = create_tac_operand_const(0);

    TacInstruction *instr1 = create_tac_instruction_logical_not(t0, const0, &arena); // t0 = !0
    TacInstruction *instr2 = create_tac_instruction_return(t0, &arena);

    TacFunction *func = create_tac_function("test_logical_not_false", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);

    const char *expected_asm =
        ".globl _test_logical_not_false\n"
        "_test_logical_not_false:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"       // Default stack allocation
        "    movl $0, %eax\n"        // Load const 0 into eax (src operand)
        "    cmpl $0, %eax\n"        // Compare eax with 0
        "    sete %al\n"             // Set al if eax is 0
        "    movzbl %al, %eax\n"     // Zero-extend al to eax
        "    movl %eax, -8(%rbp)\n"  // t0 = result (1)
        "    movl -8(%rbp), %eax\n"  // return t0
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_logical_not_const_false_input: t0 = !0; RETURN t0", func, &arena, expected_asm);
    arena_destroy(&arena);
}

static void test_codegen_logical_not_const_true_input(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_not_const_true");

    // TAC: t0 = !1; RETURN t0
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const1 = create_tac_operand_const(1); // Non-zero constant

    TacInstruction *instr1 = create_tac_instruction_logical_not(t0, const1, &arena); // t0 = !1
    TacInstruction *instr2 = create_tac_instruction_return(t0, &arena);

    TacFunction *func = create_tac_function("test_logical_not_true", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);

    const char *expected_asm =
        ".globl _test_logical_not_true\n"
        "_test_logical_not_true:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"       // Default stack allocation
        "    movl $1, %eax\n"        // Load const 1 into eax (src operand)
        "    cmpl $0, %eax\n"        // Compare eax with 0
        "    sete %al\n"             // Set al if eax is 0
        "    movzbl %al, %eax\n"     // Zero-extend al to eax
        "    movl %eax, -8(%rbp)\n"  // t0 = result (0)
        "    movl -8(%rbp), %eax\n"  // return t0
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_logical_not_const_true_input: t0 = !1; RETURN t0", func, &arena, expected_asm);
    arena_destroy(&arena);
}

static void test_codegen_logical_not_temp_input(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_not_temp_input");

    // TAC: t0 = 5; t1 = !t0; RETURN t1
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand const5 = create_tac_operand_const(5);

    TacInstruction *instr1 = create_tac_instruction_copy(t0, const5, &arena); // t0 = 5
    TacInstruction *instr2 = create_tac_instruction_logical_not(t1, t0, &arena); // t1 = !t0
    TacInstruction *instr3 = create_tac_instruction_return(t1, &arena);

    TacFunction *func = create_tac_function("test_logical_not_temp", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);
    add_instruction_to_function(func, instr3, &arena);

    const char *expected_asm =
        ".globl _test_logical_not_temp\n"
        "_test_logical_not_temp:\n"
        "    pushq %rbp\n"
        "    movq %rsp, %rbp\n"
        "    subq $32, %rsp\n"       // Default stack, t0 is -8, t1 is -16
        "    movl $5, -8(%rbp)\n"    // t0 = 5
        "    movl -8(%rbp), %eax\n"  // load t0 (src operand)
        "    cmpl $0, %eax\n"        // Compare eax with 0
        "    sete %al\n"             // Set al if eax is 0
        "    movzbl %al, %eax\n"     // Zero-extend al to eax
        "    movl %eax, -16(%rbp)\n" // t1 = result (0)
        "    movl -16(%rbp), %eax\n" // return t1
        "    leave\n"
        "    retq\n";

    verify_asm_for_function("test_codegen_logical_not_temp_input: t0=5; t1=!t0; RETURN t1", func, &arena, expected_asm);
    arena_destroy(&arena);
}

// --- END OF Tests for TAC_INS_LOGICAL_NOT ---

// --- Test Runner ---

void run_codegen_tests(void)
{
    RUN_TEST(test_codegen_simple_return);
    RUN_TEST(test_operand_to_assembly_string_const_ok);
    RUN_TEST(test_operand_to_assembly_string_temp_ok);
    RUN_TEST(test_operand_to_assembly_string_null_operand);
    RUN_TEST(test_operand_to_assembly_string_small_buffer);
    RUN_TEST(test_operand_to_assembly_string_zero_buffer);
    RUN_TEST(test_operand_to_assembly_string_unhandled_type);

    RUN_TEST(test_calculate_max_temp_id_null_function);
    RUN_TEST(test_calculate_max_temp_id_no_instructions);
    RUN_TEST(test_calculate_max_temp_id_no_temporaries);
    RUN_TEST(test_calculate_max_temp_id_one_temporary_dst);
    RUN_TEST(test_calculate_max_temp_id_one_temporary_src);
    RUN_TEST(test_calculate_max_temp_id_mixed_operands);
    RUN_TEST(test_calculate_max_temp_id_temp_ids_not_sequential);

    RUN_TEST(test_codegen_copy_const_to_temp_and_return);
    RUN_TEST(test_codegen_negate_temp_from_temp);
    RUN_TEST(test_codegen_complement_temp_in_place);
    RUN_TEST(test_codegen_complement_of_negated_constant);
    RUN_TEST(test_codegen_stack_allocation_for_many_temps);
    RUN_TEST(test_codegen_return_negated_parenthesized_constant);

    // Add these lines for LOGICAL_NOT tests
    RUN_TEST(test_codegen_logical_not_const_false_input);
    RUN_TEST(test_codegen_logical_not_const_true_input);
    RUN_TEST(test_codegen_logical_not_temp_input);

    // New tests for relational and conditional jumps
    RUN_TEST(test_codegen_relational_equal_consts_true);
    RUN_TEST(test_codegen_relational_equal_consts_false);
    RUN_TEST(test_codegen_relational_not_equal_consts_true);
    RUN_TEST(test_codegen_relational_not_equal_consts_false);
    RUN_TEST(test_codegen_relational_less_consts_true_less);
    RUN_TEST(test_codegen_relational_less_consts_false_equal);
    RUN_TEST(test_codegen_relational_less_equal_consts_true_less);
    RUN_TEST(test_codegen_relational_less_equal_consts_true_equal);
    RUN_TEST(test_codegen_relational_greater_consts_true_greater);
    RUN_TEST(test_codegen_relational_greater_consts_false_equal);
    RUN_TEST(test_codegen_relational_greater_consts_false_less);
    RUN_TEST(test_codegen_relational_greater_equal_consts_true_greater);
    RUN_TEST(test_codegen_relational_greater_equal_consts_true_equal);
    RUN_TEST(test_codegen_relational_greater_equal_consts_false_less);
    RUN_TEST(test_codegen_if_false_goto_jumps);
    RUN_TEST(test_codegen_if_false_goto_no_jump);
    RUN_TEST(test_codegen_if_true_goto_jumps);
    RUN_TEST(test_codegen_if_true_goto_no_jump);

}
