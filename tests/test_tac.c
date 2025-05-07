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

// Test creating a TAC function
void test_create_function(void) {
    // Create a local arena for this test
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for test_create_function");

    const char *func_name = "my_test_func";
    TacFunction *func = create_tac_function(func_name, &test_arena);

    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_NOT_NULL(func->name);
    TEST_ASSERT_EQUAL_STRING(func_name, func->name);
    // Check if the name was allocated in the arena (it should not be the same pointer)
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
    Arena test_arena = arena_create(4096);
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
        TEST_ASSERT_EQUAL(initial_capacity, func->instruction_capacity); // Capacity shouldn't change yet
        // Verify the added instruction (basic check)
        TEST_ASSERT_EQUAL(TAC_INS_COPY, func->instructions[i].type);
        TEST_ASSERT_EQUAL(t0.value.temp_id, func->instructions[i].operands.copy.dst.value.temp_id);
        TEST_ASSERT_EQUAL(c10.value.constant_value, func->instructions[i].operands.copy.src.value.constant_value);
    }

    // Add one more instruction to trigger reallocation
    TacInstruction *trigger_instr = create_tac_instruction_return(t0, &test_arena);
    add_instruction_to_function(func, trigger_instr, &test_arena);

    // Verify counts and capacity after reallocation
    TEST_ASSERT_EQUAL(initial_capacity + 1, func->instruction_count);
    TEST_ASSERT_GREATER_THAN(initial_capacity, func->instruction_capacity); // Capacity must have increased

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
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for test_add_functions");

    TacProgram *prog = create_tac_program(&test_arena);
    TEST_ASSERT_NOT_NULL(prog);

    size_t initial_capacity = prog->function_capacity;
    TEST_ASSERT_GREATER_THAN(0, initial_capacity);

    // Add functions up to initial capacity
    for (size_t i = 0; i < initial_capacity; ++i) {
        char func_name[20];
        sprintf(func_name, "func_%zu", i);
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
    TEST_ASSERT_GREATER_THAN(initial_capacity, prog->function_capacity); // Capacity must have increased
    TEST_ASSERT_EQUAL_STRING("trigger_func", prog->functions[initial_capacity]->name);

    // Destroy the local arena
    arena_destroy(&test_arena);
}

// Test the TAC pretty printing functionality
static void test_print_tac_program(void) {
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create arena for print test");

    // 1. Initialize StringBuffer
    StringBuffer sb;
    string_buffer_init(&sb, &test_arena, 256); // Initial capacity 256, will grow if needed

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

    // 5. Create and add TacInstructions to the function
    TacInstruction *instr1 = create_tac_instruction_copy(t0, const_val2, &test_arena);
    TacInstruction *instr2 = create_tac_instruction_negate(t1, t0, &test_arena);
    TacInstruction *instr3 = create_tac_instruction_return(t1, &test_arena);
    TacInstruction *instr4 = create_tac_instruction_return(const_val, &test_arena); // Another return for variety

    add_instruction_to_function(func1, instr1, &test_arena);
    add_instruction_to_function(func1, instr2, &test_arena);
    add_instruction_to_function(func1, instr3, &test_arena);
    add_instruction_to_function(func1, instr4, &test_arena);

    // 6. Add the function to the program
    add_function_to_program(program, func1, &test_arena);

    // 7. Print to StringBuffer
    tac_print_program(&sb, program);

    // 8. Define expected output
    const char *expected_output =
            "Program\n"
            "Function main:\n"
            "  t0 = 10\n"
            "  t1 = - t0\n"
            "  return t1\n"
            "  return 42\n";

    // 9. Compare
    const char *actual_output = string_buffer_content_str(&sb);
    // printf("\nExpected:\n%s\nActual:\n%s\nLength Expected: %zu, Actual: %zu\n", expected_output, actual_output, strlen(expected_output), sb.length);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_output, actual_output,
                                     "TAC program print output mismatch using StringBuffer.");

    // 10. Cleanup - Arena destruction handles StringBuffer memory
    arena_destroy(&test_arena);
}

// --- Test Runner ---

void run_tac_tests(void) {
    RUN_TEST(test_create_operands);
    RUN_TEST(test_create_instructions);
    RUN_TEST(test_create_function);
    RUN_TEST(test_add_instructions);
    RUN_TEST(test_create_program);
    RUN_TEST(test_add_functions);
    RUN_TEST(test_print_tac_program); // Add the new test to the runner
}
