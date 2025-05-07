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
    Arena arena = arena_create(512); // Increased from 256
    TacFunction *func = create_tac_function("no_temps_func", &arena);

    TacOperand const_op = create_tac_operand_const(5);
    TacInstruction *ret_instr = create_tac_instruction_return(const_op, &arena);
    add_instruction_to_function(func, ret_instr, &arena);

    TEST_ASSERT_EQUAL_INT(-1, calculate_max_temp_id(func));
    arena_destroy(&arena);
}

static void test_calculate_max_temp_id_one_temporary_dst(void) {
    Arena arena = arena_create(512); // Increased from 256
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
    Arena arena = arena_create(512); // Increased from 256
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
    Arena arena = arena_create(512);
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
    Arena arena = arena_create(512); // Increased from 256
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
    Arena arena = arena_create(1024);
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
    Arena arena = arena_create(1024);
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
    Arena arena = arena_create(1024);
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
    Arena arena = arena_create(1024);
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

// Test for: int main(void) { return -((((10)))); }
// Expected TAC: t0 = 10; t1 = -t0; return t1;
// Expected stack: 2 temps (t0,t1) -> max_id=1 -> (1+1)*8=16 -> aligned to 16 -> min 32 bytes.
static void test_codegen_return_negated_parenthesized_constant(void) {
    Arena arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL(arena.start);

    // 1. Create AST: ProgramNode -> FuncDefNode("main") -> ReturnStmtNode -> UnaryOpNode(NEGATE_OP) -> IntLiteralNode(10)
    IntLiteralNode *int_node = create_int_literal_node(10, &arena);
    UnaryOpNode *neg_op_node = create_unary_op_node(OPERATOR_NEGATE, (AstNode *) int_node, &arena);
    ReturnStmtNode *ret_stmt_node = create_return_stmt_node((AstNode *) neg_op_node, &arena);
    FuncDefNode *func_def_node = create_func_def_node("main", (AstNode *) ret_stmt_node, &arena);
    ProgramNode *program_node = create_program_node(func_def_node, &arena);

    // 2. Convert AST to TAC
    TacProgram *tac_program = ast_to_tac(program_node, &arena);
    TEST_ASSERT_NOT_NULL_MESSAGE(tac_program, "AST to TAC conversion failed for negated parenthesized constant");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, tac_program->function_count, "Expected 1 function in TAC program");
    // Optional: Print TAC for debugging if needed
    // StringBuffer tac_sb;
    // string_buffer_init(&tac_sb, &arena, 256);
    // tac_print_program(&tac_sb, tac_program);
    // printf("TAC for test_codegen_return_negated_parenthesized_constant:\n%s\n", string_buffer_content_str(&tac_sb));


    // 3. Generate Assembly
    StringBuffer asm_sb;
    string_buffer_init(&asm_sb, &arena, 512);
    bool success = codegen_generate_program(tac_program, &asm_sb);
    TEST_ASSERT_TRUE_MESSAGE(success, "codegen_generate_program failed for negated parenthesized constant");

    // 4. Assertions
    const char *expected_asm =
            ".globl _main\n"
            "_main:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n" // Stack frame for t0
            "    movl $10, %eax\n" // eax = 10 (from const operand of negate)
            "    negl %eax\n" // eax = -eax
            "    movl %eax, -8(%rbp)\n" // t0 = eax (result of negation)
            "    movl -8(%rbp), %eax\n" // return t0
            "    leave\n"
            "    retq\n";

    const char *actual_asm = string_buffer_content_str(&asm_sb);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_asm, actual_asm,
                                     "Generated assembly mismatch for negated parenthesized constant");

    arena_destroy(&arena);
}

// --- Test Runner ---

void run_codegen_tests(void) {
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
    RUN_TEST(test_codegen_stack_allocation_for_many_temps); // Added new test
    RUN_TEST(test_codegen_return_negated_parenthesized_constant); // Added new test
}
