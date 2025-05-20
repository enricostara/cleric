#include "../_unity/unity.h"
#include "../../src/ir/tac.h"
#include "../../src/memory/arena.h"

void verify_asm_for_function(const char *test_description, // For TEST_ASSERT_EQUAL_STRING_MESSAGE
                             TacFunction *func, // Caller creates and populates this with an arena
                             Arena *arena, // The arena used by the caller for func and its instructions
                             const char *expected_assembly
);

// --- Test Cases ---

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
            "    subq $32, %rsp\n" // Default stack allocation
            "    movl $0, %eax\n" // Load const 0 into eax (src operand)
            "    cmpl $0, %eax\n" // Compare eax with 0
            "    sete %al\n" // Set al if eax is 0
            "    movzbl %al, %eax\n" // Zero-extend al to eax
            "    movl %eax, -8(%rbp)\n" // t0 = result (1)
            "    movl -8(%rbp), %eax\n" // return t0
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_logical_not_const_false_input: t0 = !0; RETURN t0", func, &arena,
                            expected_asm);
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
            "    subq $32, %rsp\n" // Default stack allocation
            "    movl $1, %eax\n" // Load const 1 into eax (src operand)
            "    cmpl $0, %eax\n" // Compare eax with 0
            "    sete %al\n" // Set al if eax is 0
            "    movzbl %al, %eax\n" // Zero-extend al to eax
            "    movl %eax, -8(%rbp)\n" // t0 = result (0)
            "    movl -8(%rbp), %eax\n" // return t0
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_logical_not_const_true_input: t0 = !1; RETURN t0", func, &arena,
                            expected_asm);
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
            "    subq $32, %rsp\n" // Default stack, t0 is -8, t1 is -16
            "    movl $5, -8(%rbp)\n" // t0 = 5
            "    movl -8(%rbp), %eax\n" // load t0 (src operand)
            "    cmpl $0, %eax\n" // Compare eax with 0
            "    sete %al\n" // Set al if eax is 0
            "    movzbl %al, %eax\n" // Zero-extend al to eax
            "    movl %eax, -16(%rbp)\n" // t1 = result (0)
            "    movl -16(%rbp), %eax\n" // return t1
            "    leave\n"
            "    retq\n";

    verify_asm_for_function("test_codegen_logical_not_temp_input: t0=5; t1=!t0; RETURN t1", func, &arena, expected_asm);
    arena_destroy(&arena);
}

// --- Test Cases for LOGICAL_AND ---

static void test_codegen_logical_and_consts_true_true(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_and_consts_true_true");

    // TAC: t0 = 1 && 1; RETURN t0
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const1_a = create_tac_operand_const(1);
    TacOperand const1_b = create_tac_operand_const(1);

    TacInstruction *instr1 = create_tac_instruction_logical_and(t0, const1_a, const1_b, &arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &arena);

    TacFunction *func = create_tac_function("test_logical_and_tt", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);

    const char *expected_asm =
            ".globl _test_logical_and_tt\n"
            "_test_logical_and_tt:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $1, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %dl\n"
            "    movl $1, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %al\n"
            "    andb %dl, %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n" // t0 = 1
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";
    verify_asm_for_function("test_codegen_logical_and_consts_true_true: t0 = 1 && 1; RETURN t0", func, &arena,
                            expected_asm);
    arena_destroy(&arena);
}

static void test_codegen_logical_and_consts_true_false(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_and_consts_true_false");

    // TAC: t0 = 1 && 0; RETURN t0
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const1 = create_tac_operand_const(1);
    TacOperand const0 = create_tac_operand_const(0);

    TacInstruction *instr1 = create_tac_instruction_logical_and(t0, const1, const0, &arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &arena);

    TacFunction *func = create_tac_function("test_logical_and_tf", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);

    const char *expected_asm =
            ".globl _test_logical_and_tf\n"
            "_test_logical_and_tf:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $1, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %dl\n"
            "    movl $0, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %al\n"
            "    andb %dl, %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n" // t0 = 0
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";
    verify_asm_for_function("test_codegen_logical_and_consts_true_false: t0 = 1 && 0; RETURN t0", func, &arena,
                            expected_asm);
    arena_destroy(&arena);
}

static void test_codegen_logical_and_consts_false_true(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_and_consts_false_true");

    // TAC: t0 = 0 && 1; RETURN t0
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const0 = create_tac_operand_const(0);
    TacOperand const1 = create_tac_operand_const(1);

    TacInstruction *instr1 = create_tac_instruction_logical_and(t0, const0, const1, &arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &arena);

    TacFunction *func = create_tac_function("test_logical_and_ft", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);

    const char *expected_asm =
            ".globl _test_logical_and_ft\n"
            "_test_logical_and_ft:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $0, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %dl\n"
            "    movl $1, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %al\n"
            "    andb %dl, %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n" // t0 = 0
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";
    verify_asm_for_function("test_codegen_logical_and_consts_false_true: t0 = 0 && 1; RETURN t0", func, &arena,
                            expected_asm);
    arena_destroy(&arena);
}

static void test_codegen_logical_and_consts_false_false(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_and_consts_false_false");

    // TAC: t0 = 0 && 0; RETURN t0
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const0_a = create_tac_operand_const(0);
    TacOperand const0_b = create_tac_operand_const(0);

    TacInstruction *instr1 = create_tac_instruction_logical_and(t0, const0_a, const0_b, &arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &arena);

    TacFunction *func = create_tac_function("test_logical_and_ff", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);

    const char *expected_asm =
            ".globl _test_logical_and_ff\n"
            "_test_logical_and_ff:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $0, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %dl\n"
            "    movl $0, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %al\n"
            "    andb %dl, %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n" // t0 = 0
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";
    verify_asm_for_function("test_codegen_logical_and_consts_false_false: t0 = 0 && 0; RETURN t0", func, &arena,
                            expected_asm);
    arena_destroy(&arena);
}

static void test_codegen_logical_and_temps(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_and_temps");

    // TAC: t0 = 1; t1 = 0; t2 = t0 && t1; RETURN t2
    TacOperand t0_op = create_tac_operand_temp(0);
    TacOperand t1_op = create_tac_operand_temp(1);
    TacOperand t2_op = create_tac_operand_temp(2);
    TacOperand const1 = create_tac_operand_const(1);
    TacOperand const0 = create_tac_operand_const(0);

    TacInstruction *instr1 = create_tac_instruction_copy(t0_op, const1, &arena);
    TacInstruction *instr2 = create_tac_instruction_copy(t1_op, const0, &arena);
    TacInstruction *instr3 = create_tac_instruction_logical_and(t2_op, t0_op, t1_op, &arena);
    TacInstruction *instr4 = create_tac_instruction_return(t2_op, &arena);

    TacFunction *func = create_tac_function("test_logical_and_temps", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);
    add_instruction_to_function(func, instr3, &arena);
    add_instruction_to_function(func, instr4, &arena);

    const char *expected_asm =
            ".globl _test_logical_and_temps\n"
            "_test_logical_and_temps:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $1, -8(%rbp)\n" // t0 = 1
            "    movl $0, -16(%rbp)\n" // t1 = 0
            "    movl -8(%rbp), %eax\n" // load t0
            "    testl %eax, %eax\n"
            "    setne %dl\n" // dl = (t0 != 0) -> 1
            "    movl -16(%rbp), %eax\n" // load t1
            "    testl %eax, %eax\n"
            "    setne %al\n" // al = (t1 != 0) -> 0
            "    andb %dl, %al\n" // al = 1 && 0 -> 0
            "    movzbl %al, %eax\n"
            "    movl %eax, -24(%rbp)\n" // t2 = 0
            "    movl -24(%rbp), %eax\n" // return t2
            "    leave\n"
            "    retq\n";
    verify_asm_for_function("test_codegen_logical_and_temps: t0=1; t1=0; t2=t0&&t1; RETURN t2", func, &arena,
                            expected_asm);
    arena_destroy(&arena);
}

// --- Test Cases for LOGICAL_OR ---

static void test_codegen_logical_or_consts_true_true(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_or_consts_true_true");

    // TAC: t0 = 1 || 1; RETURN t0
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const1_a = create_tac_operand_const(1);
    TacOperand const1_b = create_tac_operand_const(1);

    TacInstruction *instr1 = create_tac_instruction_logical_or(t0, const1_a, const1_b, &arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &arena);

    TacFunction *func = create_tac_function("test_logical_or_tt", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);

    const char *expected_asm =
            ".globl _test_logical_or_tt\n"
            "_test_logical_or_tt:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $1, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %dl\n"
            "    movl $1, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %al\n"
            "    orb %dl, %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n" // t0 = 1
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";
    verify_asm_for_function("test_codegen_logical_or_consts_true_true: t0 = 1 || 1; RETURN t0", func, &arena,
                            expected_asm);
    arena_destroy(&arena);
}

static void test_codegen_logical_or_consts_true_false(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_or_consts_true_false");

    // TAC: t0 = 1 || 0; RETURN t0
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const1 = create_tac_operand_const(1);
    TacOperand const0 = create_tac_operand_const(0);

    TacInstruction *instr1 = create_tac_instruction_logical_or(t0, const1, const0, &arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &arena);

    TacFunction *func = create_tac_function("test_logical_or_tf", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);

    const char *expected_asm =
            ".globl _test_logical_or_tf\n"
            "_test_logical_or_tf:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $1, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %dl\n"
            "    movl $0, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %al\n"
            "    orb %dl, %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n" // t0 = 1
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";
    verify_asm_for_function("test_codegen_logical_or_consts_true_false: t0 = 1 || 0; RETURN t0", func, &arena,
                            expected_asm);
    arena_destroy(&arena);
}

static void test_codegen_logical_or_consts_false_true(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_or_consts_false_true");

    // TAC: t0 = 0 || 1; RETURN t0
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const0 = create_tac_operand_const(0);
    TacOperand const1 = create_tac_operand_const(1);

    TacInstruction *instr1 = create_tac_instruction_logical_or(t0, const0, const1, &arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &arena);

    TacFunction *func = create_tac_function("test_logical_or_ft", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);

    const char *expected_asm =
            ".globl _test_logical_or_ft\n"
            "_test_logical_or_ft:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $0, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %dl\n"
            "    movl $1, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %al\n"
            "    orb %dl, %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n" // t0 = 1
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";
    verify_asm_for_function("test_codegen_logical_or_consts_false_true: t0 = 0 || 1; RETURN t0", func, &arena,
                            expected_asm);
    arena_destroy(&arena);
}

static void test_codegen_logical_or_consts_false_false(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_or_consts_false_false");

    // TAC: t0 = 0 || 0; RETURN t0
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand const0_a = create_tac_operand_const(0);
    TacOperand const0_b = create_tac_operand_const(0);

    TacInstruction *instr1 = create_tac_instruction_logical_or(t0, const0_a, const0_b, &arena);
    TacInstruction *instr2 = create_tac_instruction_return(t0, &arena);

    TacFunction *func = create_tac_function("test_logical_or_ff", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);

    const char *expected_asm =
            ".globl _test_logical_or_ff\n"
            "_test_logical_or_ff:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $0, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %dl\n"
            "    movl $0, %eax\n"
            "    testl %eax, %eax\n"
            "    setne %al\n"
            "    orb %dl, %al\n"
            "    movzbl %al, %eax\n"
            "    movl %eax, -8(%rbp)\n" // t0 = 0
            "    movl -8(%rbp), %eax\n"
            "    leave\n"
            "    retq\n";
    verify_asm_for_function("test_codegen_logical_or_consts_false_false: t0 = 0 || 0; RETURN t0", func, &arena,
                            expected_asm);
    arena_destroy(&arena);
}

static void test_codegen_logical_or_temps(void) {
    Arena arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(arena.start, "Failed to create arena for logical_or_temps");

    // TAC: t0 = 0; t1 = 1; t2 = t0 || t1; RETURN t2
    TacOperand t0_op = create_tac_operand_temp(0);
    TacOperand t1_op = create_tac_operand_temp(1);
    TacOperand t2_op = create_tac_operand_temp(2);
    TacOperand const0 = create_tac_operand_const(0);
    TacOperand const1 = create_tac_operand_const(1);

    TacInstruction *instr1 = create_tac_instruction_copy(t0_op, const0, &arena);
    TacInstruction *instr2 = create_tac_instruction_copy(t1_op, const1, &arena);
    TacInstruction *instr3 = create_tac_instruction_logical_or(t2_op, t0_op, t1_op, &arena);
    TacInstruction *instr4 = create_tac_instruction_return(t2_op, &arena);

    TacFunction *func = create_tac_function("test_logical_or_temps", &arena);
    add_instruction_to_function(func, instr1, &arena);
    add_instruction_to_function(func, instr2, &arena);
    add_instruction_to_function(func, instr3, &arena);
    add_instruction_to_function(func, instr4, &arena);

    const char *expected_asm =
            ".globl _test_logical_or_temps\n"
            "_test_logical_or_temps:\n"
            "    pushq %rbp\n"
            "    movq %rsp, %rbp\n"
            "    subq $32, %rsp\n"
            "    movl $0, -8(%rbp)\n" // t0 = 0
            "    movl $1, -16(%rbp)\n" // t1 = 1
            "    movl -8(%rbp), %eax\n" // load t0
            "    testl %eax, %eax\n"
            "    setne %dl\n" // dl = (t0 != 0) -> 0
            "    movl -16(%rbp), %eax\n" // load t1
            "    testl %eax, %eax\n"
            "    setne %al\n" // al = (t1 != 0) -> 1
            "    orb %dl, %al\n" // al = 0 || 1 -> 1
            "    movzbl %al, %eax\n"
            "    movl %eax, -24(%rbp)\n" // t2 = 1
            "    movl -24(%rbp), %eax\n" // return t2
            "    leave\n"
            "    retq\n";
    verify_asm_for_function("test_codegen_logical_or_temps: t0=0; t1=1; t2=t0||t1; RETURN t2", func, &arena,
                            expected_asm);
    arena_destroy(&arena);
}

// --- Test Runner ---

void run_codegen_logical_tests(void) {
    RUN_TEST(test_codegen_logical_not_const_false_input);
    RUN_TEST(test_codegen_logical_not_const_true_input);
    RUN_TEST(test_codegen_logical_not_temp_input);

    // LOGICAL_AND tests
    RUN_TEST(test_codegen_logical_and_consts_true_true);
    RUN_TEST(test_codegen_logical_and_consts_true_false);
    RUN_TEST(test_codegen_logical_and_consts_false_true);
    RUN_TEST(test_codegen_logical_and_consts_false_false);
    RUN_TEST(test_codegen_logical_and_temps);

    // LOGICAL_OR tests
    RUN_TEST(test_codegen_logical_or_consts_true_true);
    RUN_TEST(test_codegen_logical_or_consts_true_false);
    RUN_TEST(test_codegen_logical_or_consts_false_true);
    RUN_TEST(test_codegen_logical_or_consts_false_false);
    RUN_TEST(test_codegen_logical_or_temps);
}
