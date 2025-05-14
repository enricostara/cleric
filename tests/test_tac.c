#include "unity.h"
#include "../src/memory/arena.h" // For Arena allocator
#include "../src/ir/tac.h"      // Header for the TAC module we are testing
#include "../src/strings/strings.h" // For StringBuffer

#include <stddef.h> // For size_t
#include <string.h> // For strcmp
#include <stdlib.h> // For general utilities (not strictly needed for free anymore with arena)

// --- Test Cases ---

// Test creating constant and temporary operands
void test_create_operands(void) {
    // Test Constant Operand
    TacOperand const_op = create_tac_operand_const(123);
    TEST_ASSERT_EQUAL(TAC_OPERAND_CONST, const_op.type);
    TEST_ASSERT_EQUAL(123, const_op.value.constant_value);

    // Test Temporary Operand
    TacOperand temp_op = create_tac_operand_temp(5);
    TEST_ASSERT_EQUAL(TAC_OPERAND_TEMP, temp_op.type);
    TEST_ASSERT_EQUAL(5, temp_op.value.temp_id);

    // Test Label Operand
    Arena label_arena = arena_create(256);
    const char *label_name_str = "L1";
    TacOperand label_op = create_tac_operand_label(label_name_str, &label_arena);
    TEST_ASSERT_EQUAL(TAC_OPERAND_LABEL, label_op.type);
    TEST_ASSERT_EQUAL_STRING(label_name_str, label_op.value.label_name);
    arena_destroy(&label_arena);
}

// Test creating basic instructions
void test_create_instructions(void) {
    // Create a local arena for this test
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for test_create_instructions");

    TacOperand dst = create_tac_operand_temp(1);
    TacOperand src_const = create_tac_operand_const(42);
    TacOperand src_temp = create_tac_operand_temp(0);

    // Test COPY
    TacInstruction *copy_instr = create_tac_instruction_copy(dst, src_const, &test_arena);
    TEST_ASSERT_NOT_NULL(copy_instr);
    TEST_ASSERT_EQUAL(TAC_INS_COPY, copy_instr->type);
    TEST_ASSERT_EQUAL(dst.type, copy_instr->operands.copy.dst.type);
    TEST_ASSERT_EQUAL(dst.value.temp_id, copy_instr->operands.copy.dst.value.temp_id);
    TEST_ASSERT_EQUAL(src_const.type, copy_instr->operands.copy.src.type);
    TEST_ASSERT_EQUAL(src_const.value.constant_value, copy_instr->operands.copy.src.value.constant_value);

    // Test NEGATE
    TacInstruction *negate_instr = create_tac_instruction_negate(dst, src_temp, &test_arena);
    TEST_ASSERT_NOT_NULL(negate_instr);
    TEST_ASSERT_EQUAL(TAC_INS_NEGATE, negate_instr->type);
    TEST_ASSERT_EQUAL(dst.value.temp_id, negate_instr->operands.unary_op.dst.value.temp_id);
    TEST_ASSERT_EQUAL(src_temp.value.temp_id, negate_instr->operands.unary_op.src.value.temp_id);

    // Test COMPLEMENT
    TacInstruction *comp_instr = create_tac_instruction_complement(dst, src_temp, &test_arena);
    TEST_ASSERT_NOT_NULL(comp_instr);
    TEST_ASSERT_EQUAL(TAC_INS_COMPLEMENT, comp_instr->type);
    TEST_ASSERT_EQUAL(dst.value.temp_id, comp_instr->operands.unary_op.dst.value.temp_id);
    TEST_ASSERT_EQUAL(src_temp.value.temp_id, comp_instr->operands.unary_op.src.value.temp_id);

    // Test RETURN
    TacInstruction *ret_instr = create_tac_instruction_return(src_const, &test_arena);
    TEST_ASSERT_NOT_NULL(ret_instr);
    TEST_ASSERT_EQUAL(TAC_INS_RETURN, ret_instr->type);
    TEST_ASSERT_EQUAL(src_const.value.constant_value, ret_instr->operands.ret.src.value.constant_value);

    // Destroy the local arena
    arena_destroy(&test_arena);
}

// Test creating binary instructions
void test_create_binary_instructions(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for test_create_binary_instructions");

    TacOperand dst = create_tac_operand_temp(3);
    TacOperand src1_temp = create_tac_operand_temp(1);
    TacOperand src2_temp = create_tac_operand_temp(2);
    TacOperand src1_const = create_tac_operand_const(100);
    TacOperand src2_const = create_tac_operand_const(50);

    // Test ADD (temp + temp)
    TacInstruction *add_instr = create_tac_instruction_add(dst, src1_temp, src2_temp, &test_arena);
    TEST_ASSERT_NOT_NULL(add_instr);
    TEST_ASSERT_EQUAL(TAC_INS_ADD, add_instr->type);
    TEST_ASSERT_EQUAL(dst.value.temp_id, add_instr->operands.binary_op.dst.value.temp_id);
    TEST_ASSERT_EQUAL(src1_temp.value.temp_id, add_instr->operands.binary_op.src1.value.temp_id);
    TEST_ASSERT_EQUAL(src2_temp.value.temp_id, add_instr->operands.binary_op.src2.value.temp_id);

    // Test SUB (const - temp)
    TacInstruction *sub_instr = create_tac_instruction_sub(dst, src1_const, src2_temp, &test_arena);
    TEST_ASSERT_NOT_NULL(sub_instr);
    TEST_ASSERT_EQUAL(TAC_INS_SUB, sub_instr->type);
    TEST_ASSERT_EQUAL(dst.value.temp_id, sub_instr->operands.binary_op.dst.value.temp_id);
    TEST_ASSERT_EQUAL(src1_const.value.constant_value, sub_instr->operands.binary_op.src1.value.constant_value);
    TEST_ASSERT_EQUAL(src2_temp.value.temp_id, sub_instr->operands.binary_op.src2.value.temp_id);

    // Test MUL (temp * const)
    TacInstruction *mul_instr = create_tac_instruction_mul(dst, src1_temp, src2_const, &test_arena);
    TEST_ASSERT_NOT_NULL(mul_instr);
    TEST_ASSERT_EQUAL(TAC_INS_MUL, mul_instr->type);
    TEST_ASSERT_EQUAL(dst.value.temp_id, mul_instr->operands.binary_op.dst.value.temp_id);
    TEST_ASSERT_EQUAL(src1_temp.value.temp_id, mul_instr->operands.binary_op.src1.value.temp_id);
    TEST_ASSERT_EQUAL(src2_const.value.constant_value, mul_instr->operands.binary_op.src2.value.constant_value);

    // Test DIV (const / const)
    TacInstruction *div_instr = create_tac_instruction_div(dst, src1_const, src2_const, &test_arena);
    TEST_ASSERT_NOT_NULL(div_instr);
    TEST_ASSERT_EQUAL(TAC_INS_DIV, div_instr->type);
    TEST_ASSERT_EQUAL(dst.value.temp_id, div_instr->operands.binary_op.dst.value.temp_id);
    TEST_ASSERT_EQUAL(src1_const.value.constant_value, div_instr->operands.binary_op.src1.value.constant_value);
    TEST_ASSERT_EQUAL(src2_const.value.constant_value, div_instr->operands.binary_op.src2.value.constant_value);

    // Test MOD (temp % temp)
    TacInstruction *mod_instr = create_tac_instruction_mod(dst, src1_temp, src2_temp, &test_arena);
    TEST_ASSERT_NOT_NULL(mod_instr);
    TEST_ASSERT_EQUAL(TAC_INS_MOD, mod_instr->type);
    TEST_ASSERT_EQUAL(dst.value.temp_id, mod_instr->operands.binary_op.dst.value.temp_id);
    TEST_ASSERT_EQUAL(src1_temp.value.temp_id, mod_instr->operands.binary_op.src1.value.temp_id);
    TEST_ASSERT_EQUAL(src2_temp.value.temp_id, mod_instr->operands.binary_op.src2.value.temp_id);

    arena_destroy(&test_arena);
}

// Test creating new TAC instructions (Logical NOT, Relational, Control Flow)
void test_create_new_tac_instructions(void) {
    Arena test_arena = arena_create(2048);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for test_create_new_tac_instructions");

    TacOperand dst = create_tac_operand_temp(1);
    TacOperand src1_temp = create_tac_operand_temp(0);
    TacOperand src2_temp = create_tac_operand_temp(2);
    TacOperand const_op = create_tac_operand_const(1);

    // Test LOGICAL_NOT (dst = !src1_temp)
    TacInstruction *logical_not_instr = create_tac_instruction_logical_not(dst, src1_temp, &test_arena);
    TEST_ASSERT_NOT_NULL(logical_not_instr);
    TEST_ASSERT_EQUAL(TAC_INS_LOGICAL_NOT, logical_not_instr->type);
    TEST_ASSERT_EQUAL(dst.value.temp_id, logical_not_instr->operands.unary_op.dst.value.temp_id);
    TEST_ASSERT_EQUAL(src1_temp.value.temp_id, logical_not_instr->operands.unary_op.src.value.temp_id);

    // Test LESS (dst = src1_temp < src2_temp)
    TacInstruction *less_instr = create_tac_instruction_less(dst, src1_temp, src2_temp, &test_arena);
    TEST_ASSERT_NOT_NULL(less_instr);
    TEST_ASSERT_EQUAL(TAC_INS_LESS, less_instr->type);
    TEST_ASSERT_EQUAL(dst.value.temp_id, less_instr->operands.relational_op.dst.value.temp_id);
    TEST_ASSERT_EQUAL(src1_temp.value.temp_id, less_instr->operands.relational_op.src1.value.temp_id);
    TEST_ASSERT_EQUAL(src2_temp.value.temp_id, less_instr->operands.relational_op.src2.value.temp_id);

    // Test GREATER_EQUAL (dst = src1_temp >= const_op)
    TacInstruction *ge_instr = create_tac_instruction_greater_equal(dst, src1_temp, const_op, &test_arena);
    TEST_ASSERT_NOT_NULL(ge_instr);
    TEST_ASSERT_EQUAL(TAC_INS_GREATER_EQUAL, ge_instr->type);
    TEST_ASSERT_EQUAL(dst.value.temp_id, ge_instr->operands.relational_op.dst.value.temp_id);
    TEST_ASSERT_EQUAL(src1_temp.value.temp_id, ge_instr->operands.relational_op.src1.value.temp_id);
    TEST_ASSERT_EQUAL(const_op.value.constant_value, ge_instr->operands.relational_op.src2.value.constant_value);

    // Test LABEL (LBL1:)
    const char *lbl_name = "LBL1";
    TacOperand label_op_def = create_tac_operand_label(lbl_name, &test_arena);
    TacInstruction *label_instr = create_tac_instruction_label(label_op_def, &test_arena);
    TEST_ASSERT_NOT_NULL(label_instr);
    TEST_ASSERT_EQUAL(TAC_INS_LABEL, label_instr->type);
    TEST_ASSERT_EQUAL(TAC_OPERAND_LABEL, label_instr->operands.label_def.label.type);
    TEST_ASSERT_EQUAL_STRING(lbl_name, label_instr->operands.label_def.label.value.label_name);

    // Test GOTO (GOTO LBL1)
    TacOperand target_label_op = create_tac_operand_label(lbl_name, &test_arena);
    TacInstruction *goto_instr = create_tac_instruction_goto(target_label_op, &test_arena);
    TEST_ASSERT_NOT_NULL(goto_instr);
    TEST_ASSERT_EQUAL(TAC_INS_GOTO, goto_instr->type);
    TEST_ASSERT_EQUAL(TAC_OPERAND_LABEL, goto_instr->operands.go_to.target_label.type);
    TEST_ASSERT_EQUAL_STRING(lbl_name, goto_instr->operands.go_to.target_label.value.label_name);

    // Test IF_FALSE_GOTO (IF_FALSE src1_temp GOTO LBL1)
    TacInstruction *if_false_goto_instr = create_tac_instruction_if_false_goto(src1_temp, target_label_op, &test_arena);
    TEST_ASSERT_NOT_NULL(if_false_goto_instr);
    TEST_ASSERT_EQUAL(TAC_INS_IF_FALSE_GOTO, if_false_goto_instr->type);
    TEST_ASSERT_EQUAL(src1_temp.type, if_false_goto_instr->operands.conditional_goto.condition_src.type);
    TEST_ASSERT_EQUAL(src1_temp.value.temp_id,
                      if_false_goto_instr->operands.conditional_goto.condition_src.value.temp_id);
    TEST_ASSERT_EQUAL(TAC_OPERAND_LABEL, if_false_goto_instr->operands.conditional_goto.target_label.type);
    TEST_ASSERT_EQUAL_STRING(lbl_name, if_false_goto_instr->operands.conditional_goto.target_label.value.label_name);

    arena_destroy(&test_arena);
}

// Test creating a TAC function
void test_create_function(void) {
    // Create a local arena for this test
    Arena test_arena = arena_create(2048); // Increased from 1024
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for test_create_function");

    const char *func_name = "my_test_func";
    TacFunction *func = create_tac_function(func_name, &test_arena);

    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_NOT_NULL(func->name);
    TEST_ASSERT_EQUAL_STRING(func_name, func->name);
    TEST_ASSERT_NOT_EQUAL(func_name, func->name);

    TEST_ASSERT_EQUAL(0, func->instruction_count);
    TEST_ASSERT_GREATER_THAN(0, func->instruction_capacity);
    TEST_ASSERT_NOT_NULL(func->instructions);

    // Destroy the local arena
    arena_destroy(&test_arena);
}

// Test adding instructions and reallocation
void test_add_instructions(void) {
    // Create a local arena for this test
    Arena test_arena = arena_create(8192); // Increased from 1024
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for test_add_instructions");

    TacFunction *func = create_tac_function("grow_func", &test_arena);
    TEST_ASSERT_NOT_NULL(func);

    size_t initial_capacity = func->instruction_capacity;
    TEST_ASSERT_GREATER_THAN(0, initial_capacity);

    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand c10 = create_tac_operand_const(10);

    // Add instructions up to initial capacity
    for (size_t i = 0; i < initial_capacity; ++i) {
        TacInstruction *instr = create_tac_instruction_copy(t0, c10, &test_arena);
        add_instruction_to_function(func, instr, &test_arena);
        TEST_ASSERT_EQUAL(i + 1, func->instruction_count);
        TEST_ASSERT_EQUAL(initial_capacity, func->instruction_capacity);
        TEST_ASSERT_EQUAL(TAC_INS_COPY, func->instructions[i].type);
        TEST_ASSERT_EQUAL(t0.value.temp_id, func->instructions[i].operands.copy.dst.value.temp_id);
        TEST_ASSERT_EQUAL(c10.value.constant_value, func->instructions[i].operands.copy.src.value.constant_value);
    }

    // Add one more instruction to trigger reallocation
    TacInstruction *trigger_instr = create_tac_instruction_return(t0, &test_arena);
    add_instruction_to_function(func, trigger_instr, &test_arena);

    // Verify counts and capacity after reallocation
    TEST_ASSERT_EQUAL(initial_capacity + 1, func->instruction_count);
    TEST_ASSERT_GREATER_THAN(initial_capacity, func->instruction_capacity);

    // Verify the instruction added after reallocation is correct
    TEST_ASSERT_EQUAL(TAC_INS_RETURN, func->instructions[initial_capacity].type);
    TEST_ASSERT_EQUAL(t0.value.temp_id, func->instructions[initial_capacity].operands.ret.src.value.temp_id);

    // Add more to potentially trigger another realloc (depending on growth factor)
    size_t final_capacity = func->instruction_capacity;
    for (size_t i = func->instruction_count; i < final_capacity + 5; ++i) {
        TacInstruction *instr = create_tac_instruction_copy(t0, c10, &test_arena);
        add_instruction_to_function(func, instr, &test_arena);
        TEST_ASSERT_EQUAL(i + 1, func->instruction_count);
    }
    TEST_ASSERT_GREATER_THAN(final_capacity, func->instruction_capacity);
    TEST_ASSERT_EQUAL(final_capacity + 5, func->instruction_count);

    // Destroy the local arena
    arena_destroy(&test_arena);
}

// Test creating a TAC program
void test_create_program(void) {
    // Create a local arena for this test
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for test_create_program");

    TacProgram *prog = create_tac_program(&test_arena);
    TEST_ASSERT_NOT_NULL(prog);
    TEST_ASSERT_EQUAL(0, prog->function_count);
    TEST_ASSERT_GREATER_THAN(0, prog->function_capacity);
    TEST_ASSERT_NOT_NULL(prog->functions);

    // Destroy the local arena
    arena_destroy(&test_arena);
}

// Test adding functions to a program and reallocation
void test_add_functions(void) {
    // Create a local arena for this test
    Arena test_arena = arena_create(8192);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for test_add_functions");

    TacProgram *prog = create_tac_program(&test_arena);
    TEST_ASSERT_NOT_NULL(prog);

    size_t initial_capacity = prog->function_capacity;
    TEST_ASSERT_GREATER_THAN(0, initial_capacity);

    // Add functions up to initial capacity
    for (size_t i = 0; i < initial_capacity; ++i) {
        char func_name[32];
        snprintf(func_name, sizeof(func_name), "func_%zu", i);
        TacFunction *func = create_tac_function(func_name, &test_arena);
        add_function_to_program(prog, func, &test_arena);
        TEST_ASSERT_EQUAL(i + 1, prog->function_count);
        TEST_ASSERT_EQUAL(initial_capacity, prog->function_capacity);
        TEST_ASSERT_EQUAL_STRING(func_name, prog->functions[i]->name);
    }

    // Add one more to trigger reallocation
    TacFunction *trigger_func = create_tac_function("trigger_func", &test_arena);
    add_function_to_program(prog, trigger_func, &test_arena);

    // Verify counts and capacity after reallocation
    TEST_ASSERT_EQUAL(initial_capacity + 1, prog->function_count);
    TEST_ASSERT_GREATER_THAN(initial_capacity, prog->function_capacity);
    TEST_ASSERT_EQUAL_STRING("trigger_func", prog->functions[initial_capacity]->name);

    // Destroy the local arena
    arena_destroy(&test_arena);
}

// Test the TAC pretty printing functionality
static void test_print_tac_program(void) {
    Arena test_arena = arena_create(8192); // Increased from 2048
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for print test");

    // 1. Initialize StringBuffer
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 256);

    // 2. Create a TacProgram
    TacProgram *program = create_tac_program(&test_arena);
    TEST_ASSERT_NOT_NULL(program);

    // 3. Create a TacFunction
    TacFunction *func1 = create_tac_function("main", &test_arena);
    TEST_ASSERT_NOT_NULL(func1);

    // 4. Create some TacOperands
    TacOperand t0 = create_tac_operand_temp(0);
    TacOperand t1 = create_tac_operand_temp(1);
    TacOperand const_val = create_tac_operand_const(42);
    TacOperand const_val2 = create_tac_operand_const(10);
    TacOperand t2 = create_tac_operand_temp(2);
    TacOperand t3 = create_tac_operand_temp(3);
    TacOperand t4 = create_tac_operand_temp(4);
    TacOperand t5 = create_tac_operand_temp(5);
    TacOperand t6 = create_tac_operand_temp(6);
    TacOperand t7 = create_tac_operand_temp(7);
    TacOperand const_val3 = create_tac_operand_const(7);
    TacOperand const_bool_true = create_tac_operand_const(1); // For boolean true

    // Operands for control flow
    const char *label_L0_name = "L0";
    const char *label_L1_name = "L1";
    TacOperand label_L0 = create_tac_operand_label(label_L0_name, &test_arena);
    TacOperand label_L1 = create_tac_operand_label(label_L1_name, &test_arena);

    // 5. Create and add TacInstructions to the function
    TacInstruction *instr1 = create_tac_instruction_copy(t0, const_val2, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_negate(t1, t0, &test_arena);
    TacInstruction *instr_copy_t2 = create_tac_instruction_copy(t2, const_val3, &test_arena);

    TacInstruction *instr_add = create_tac_instruction_add(t3, t0, t2, &test_arena);
    TacInstruction *instr_sub = create_tac_instruction_sub(t4, t0, t2, &test_arena);
    TacInstruction *instr_mul = create_tac_instruction_mul(t5, t0, t2, &test_arena);
    TacInstruction *instr_div = create_tac_instruction_div(t6, t0, t2, &test_arena);
    TacInstruction *instr_mod = create_tac_instruction_mod(t7, t0, t2, &test_arena);

    TacInstruction *instr3 = create_tac_instruction_return(t1, &test_arena);
    TacInstruction *instr4 = create_tac_instruction_return(const_val, &test_arena);

    // New instructions for printing test
    TacInstruction *instr_logical_not = create_tac_instruction_logical_not(t0, const_bool_true, &test_arena); // t0 = !1
    TacInstruction *instr_less = create_tac_instruction_less(t1, t0, t2, &test_arena); // t1 = t0 < t2
    TacInstruction *instr_equal = create_tac_instruction_equal(t3, t1, const_val, &test_arena); // t3 = t1 == 42

    TacInstruction *instr_label_L0 = create_tac_instruction_label(label_L0, &test_arena); // L0:
    TacInstruction *instr_goto_L1 = create_tac_instruction_goto(label_L1, &test_arena); // GOTO L1
    TacInstruction *instr_label_L1 = create_tac_instruction_label(label_L1, &test_arena); // L1:
    TacInstruction *instr_if_false_goto = create_tac_instruction_if_false_goto(t3, label_L0, &test_arena);
    // IF_FALSE t3 GOTO L0

    add_instruction_to_function(func1, instr1, &test_arena);
    add_instruction_to_function(func1, instr2, &test_arena);
    add_instruction_to_function(func1, instr_copy_t2, &test_arena);
    add_instruction_to_function(func1, instr_add, &test_arena);
    add_instruction_to_function(func1, instr_sub, &test_arena);
    add_instruction_to_function(func1, instr_mul, &test_arena);
    add_instruction_to_function(func1, instr_div, &test_arena);
    add_instruction_to_function(func1, instr_mod, &test_arena);
    add_instruction_to_function(func1, instr3, &test_arena);
    add_instruction_to_function(func1, instr4, &test_arena);

    // Add new instructions
    add_instruction_to_function(func1, instr_logical_not, &test_arena);
    add_instruction_to_function(func1, instr_less, &test_arena);
    add_instruction_to_function(func1, instr_equal, &test_arena);
    add_instruction_to_function(func1, instr_label_L0, &test_arena);
    add_instruction_to_function(func1, instr_goto_L1, &test_arena);
    add_instruction_to_function(func1, instr_label_L1, &test_arena);
    add_instruction_to_function(func1, instr_if_false_goto, &test_arena);

    // 6. Add function to program
    add_function_to_program(program, func1, &test_arena);

    // 7. Print to StringBuffer
    tac_print_program(&sb, program);

    // 8. Define expected output string
    // Note: Trailing newline for each instruction, and for function and program overall.
    const char *expected_output =
            "program:\n"
            "  function main:\n"
            "    t0 = 10\n"
            "    t1 = - t0\n"
            "    t2 = 7\n"
            "    t3 = t0 + t2\n"
            "    t4 = t0 - t2\n"
            "    t5 = t0 * t2\n"
            "    t6 = t0 / t2\n"
            "    t7 = t0 % t2\n"
            "    return t1\n"
            "    return 42\n"
            "    t0 = ! 1\n"
            "    t1 = t0 < t2\n"
            "    t3 = t1 == 42\n"
            "    L0:\n"
            "    goto L1\n"
            "    L1:\n"
            "    if_false t3 goto L0\n"
            "end program\n";

    // 9. Compare
    const char *actual_output = string_buffer_content_str(&sb);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_output, actual_output,
                                     "TAC program print output mismatch using StringBuffer.");

    // 10. Cleanup - Arena destruction handles StringBuffer memory
    arena_destroy(&test_arena);
}

// --- Test Runner ---

void run_tac_tests(void) {
    RUN_TEST(test_create_operands);
    RUN_TEST(test_create_instructions);
    RUN_TEST(test_create_binary_instructions);
    RUN_TEST(test_create_new_tac_instructions);
    RUN_TEST(test_create_function);
    RUN_TEST(test_add_instructions);
    RUN_TEST(test_create_program);
    RUN_TEST(test_add_functions);
    RUN_TEST(test_print_tac_program);
}
