#include "unity.h"
#include "ir/ast_to_tac.h"

#include "ir/tac.h"
#include "memory/arena.h"
#include "parser/ast.h"
#include <string.h> // For strcmp

// Forward declarations for test functions
static void test_return_int_literal(void);
static void test_return_unary_negate(void);
static void test_return_unary_complement(void);
// Add more test function declarations here...



// --- Test Case Implementations ---

static void test_return_int_literal(void) {
    // Create arena for this test
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    // 1. Construct AST for: int main() { return 5; }
    IntLiteralNode* int_node = create_int_literal_node(5, &test_arena);
    ReturnStmtNode* return_node = create_return_stmt_node((AstNode*)int_node, &test_arena);
    FuncDefNode* func_node = create_func_def_node("main", (AstNode*)return_node, &test_arena);
    ProgramNode* ast = create_program_node(func_node, &test_arena);

    // 2. Translate AST to TAC
    TacProgram* tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction* func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name); // Assuming "main" for now
    TEST_ASSERT_EQUAL_INT(1, func->instruction_count); // Expect 1 instruction: RETURN

    // Check the RETURN instruction
    const TacInstruction* instr = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(5, instr->operands.ret.src.value.constant_value);

    // Clean up arena
    arena_destroy(&test_arena);
}

static void test_return_unary_negate(void) {
    // Create arena for this test
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    // 1. Construct AST for: int main() { return -10; }
    IntLiteralNode* int_node_neg = create_int_literal_node(10, &test_arena);
    UnaryOpNode* unary_node_neg = create_unary_op_node(OPERATOR_NEGATE, (AstNode*)int_node_neg, &test_arena);
    ReturnStmtNode* return_node_neg = create_return_stmt_node((AstNode*)unary_node_neg, &test_arena);
    FuncDefNode* func_node_neg = create_func_def_node("main", (AstNode*)return_node_neg, &test_arena);
    ProgramNode* ast = create_program_node(func_node_neg, &test_arena);

    // 2. Translate AST to TAC
    TacProgram* tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction* func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name);
    TEST_ASSERT_EQUAL_INT(2, func->instruction_count); // Expect 2 instructions: NEGATE, RETURN

    // Check the NEGATE instruction (t0 = - 10)
    const TacInstruction* instr0 = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_NEGATE, instr0->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr0->operands.unary_op.dst.type);
    TEST_ASSERT_EQUAL_INT(0, instr0->operands.unary_op.dst.value.temp_id); // First temporary t0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0->operands.unary_op.src.type);
    TEST_ASSERT_EQUAL_INT(10, instr0->operands.unary_op.src.value.constant_value);

    // Check the RETURN instruction (RETURN t0)
    const TacInstruction* instr1 = &func->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr1->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr1->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(0, instr1->operands.ret.src.value.temp_id); // Should return t0

    // Clean up arena
    arena_destroy(&test_arena);
}

static void test_return_unary_complement(void) {
    // Create arena for this test
    Arena test_arena = arena_create(1024);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    // 1. Construct AST for: int main() { return ~20; }
    IntLiteralNode* int_node_comp = create_int_literal_node(20, &test_arena);
    UnaryOpNode* unary_node_comp = create_unary_op_node(OPERATOR_COMPLEMENT, (AstNode*)int_node_comp, &test_arena);
    ReturnStmtNode* return_node_comp = create_return_stmt_node((AstNode*)unary_node_comp, &test_arena);
    FuncDefNode* func_node_comp = create_func_def_node("main", (AstNode*)return_node_comp, &test_arena);
    ProgramNode* ast = create_program_node(func_node_comp, &test_arena);

    // 2. Translate AST to TAC
    TacProgram* tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction* func_comp = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func_comp);
    TEST_ASSERT_EQUAL_STRING("main", func_comp->name);
    TEST_ASSERT_EQUAL_INT(2, func_comp->instruction_count);

    // Check the COMPLEMENT instruction (t0 = ~ 20)
    const TacInstruction* instr0_comp = &func_comp->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_COMPLEMENT, instr0_comp->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr0_comp->operands.unary_op.dst.type);
    TEST_ASSERT_EQUAL_INT(0, instr0_comp->operands.unary_op.dst.value.temp_id);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0_comp->operands.unary_op.src.type);
    TEST_ASSERT_EQUAL_INT(20, instr0_comp->operands.unary_op.src.value.constant_value);

    // Check the RETURN instruction (RETURN t0)
    const TacInstruction* instr1_comp = &func_comp->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr1_comp->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr1_comp->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(0, instr1_comp->operands.ret.src.value.temp_id);

    // Clean up arena
    arena_destroy(&test_arena);
}


// --- Test Runner ---

void run_ast_to_tac_tests(void) {
    RUN_TEST(test_return_int_literal);
    RUN_TEST(test_return_unary_negate);
    RUN_TEST(test_return_unary_complement);
}
