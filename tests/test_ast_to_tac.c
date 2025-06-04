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

// Test returning a complemented negated integer literal
static void test_return_unary_complement_negate(void);

// Test returning the result of a binary addition of two literals
static void test_return_binary_add_literals(void);

// Test returning the result of a logical NOT operation
static void test_return_logical_not(void);

// Test returning the result of a relational LESS operation
static void test_return_relational_less(void);

// Test returning the result of a logical AND operation (RHS evaluates)
static void test_return_logical_and_rhs_evaluates(void);

// Test returning the result of a logical AND operation (short-circuit)
static void test_return_logical_and_short_circuit(void);

// Test returning the result of a logical OR operation (RHS evaluates)
static void test_return_logical_or_rhs_evaluates(void);

// Test returning the result of a logical OR operation (short-circuit)
static void test_return_logical_or_short_circuit(void);

// New test function declarations for variable declarations
static void test_var_decl_no_initializer(void);
static void test_var_decl_with_literal_initializer(void);
static void test_var_decl_with_expression_initializer(void);

// Add more test function declarations here...


// --- Test Case Implementations ---

static void test_return_int_literal(void) {
    // Create arena for this test
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    // 1. Construct AST for: int main() { return 5; }
    IntLiteralNode *int_node = create_int_literal_node(5, &test_arena);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *) int_node, &test_arena);
    BlockNode *body_block = create_block_node(&test_arena);
    block_node_add_item(body_block, (AstNode *) return_node, &test_arena);
    FuncDefNode *func_node = create_func_def_node("main", body_block, &test_arena);
    ProgramNode *ast = create_program_node(func_node, &test_arena);

    // 2. Translate AST to TAC
    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name); // Assuming "main" for now
    TEST_ASSERT_EQUAL_INT(1, func->instruction_count); // Expect 1 instruction: RETURN

    // Check the RETURN instruction
    const TacInstruction *instr = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(5, instr->operands.ret.src.value.constant_value);

    // Clean up arena
    arena_destroy(&test_arena);
}

static void test_return_unary_negate(void) {
    // Create arena for this test
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    // 1. Construct AST for: int main() { return -10; }
    IntLiteralNode *int_node_neg = create_int_literal_node(10, &test_arena);
    UnaryOpNode *unary_node_neg = create_unary_op_node(OPERATOR_NEGATE, (AstNode *) int_node_neg, &test_arena);
    ReturnStmtNode *return_node_neg = create_return_stmt_node((AstNode *) unary_node_neg, &test_arena);
    BlockNode *body_block_neg = create_block_node(&test_arena);
    block_node_add_item(body_block_neg, (AstNode *) return_node_neg, &test_arena);
    FuncDefNode *func_node_neg = create_func_def_node("main", body_block_neg, &test_arena);
    ProgramNode *ast = create_program_node(func_node_neg, &test_arena);

    // 2. Translate AST to TAC
    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name);
    TEST_ASSERT_EQUAL_INT(2, func->instruction_count); // Expect 2 instructions: NEGATE, RETURN

    // Check the NEGATE instruction (t0 = - 10)
    const TacInstruction *instr0 = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_NEGATE, instr0->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr0->operands.unary_op.dst.type);
    TEST_ASSERT_EQUAL_INT(0, instr0->operands.unary_op.dst.value.temp.id); // First temporary t0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0->operands.unary_op.src.type);
    TEST_ASSERT_EQUAL_INT(10, instr0->operands.unary_op.src.value.constant_value);

    // Check the RETURN instruction (RETURN t0)
    const TacInstruction *instr1 = &func->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr1->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr1->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(0, instr1->operands.ret.src.value.temp.id); // Should return t0

    // Clean up arena
    arena_destroy(&test_arena);
}

static void test_return_unary_complement(void) {
    // Create arena for this test
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    // 1. Construct AST for: int main() { return ~20; }
    IntLiteralNode *int_node_comp = create_int_literal_node(20, &test_arena);
    UnaryOpNode *unary_node_comp = create_unary_op_node(OPERATOR_COMPLEMENT, (AstNode *) int_node_comp, &test_arena);
    ReturnStmtNode *return_node_comp = create_return_stmt_node((AstNode *) unary_node_comp, &test_arena);
    BlockNode *body_block_comp = create_block_node(&test_arena);
    block_node_add_item(body_block_comp, (AstNode *) return_node_comp, &test_arena);
    FuncDefNode *func_node_comp = create_func_def_node("main", body_block_comp, &test_arena);
    ProgramNode *ast = create_program_node(func_node_comp, &test_arena);

    // 2. Translate AST to TAC
    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func_comp = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func_comp);
    TEST_ASSERT_EQUAL_STRING("main", func_comp->name);
    TEST_ASSERT_EQUAL_INT(2, func_comp->instruction_count);

    // Check the COMPLEMENT instruction (t0 = ~ 20)
    const TacInstruction *instr0_comp = &func_comp->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_COMPLEMENT, instr0_comp->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr0_comp->operands.unary_op.dst.type);
    TEST_ASSERT_EQUAL_INT(0, instr0_comp->operands.unary_op.dst.value.temp.id);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0_comp->operands.unary_op.src.type);
    TEST_ASSERT_EQUAL_INT(20, instr0_comp->operands.unary_op.src.value.constant_value);

    // Check the RETURN instruction (RETURN t0)
    const TacInstruction *instr1_comp = &func_comp->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr1_comp->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr1_comp->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(0, instr1_comp->operands.ret.src.value.temp.id);

    // Clean up arena
    arena_destroy(&test_arena);
}

// Test for: int main() { return ~(-2); }
static void test_return_unary_complement_negate(void) {
    // Create arena for this test
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for ~(-2) test");

    // 1. Construct AST for: int main() { return ~(-2); }
    IntLiteralNode *int_node = create_int_literal_node(2, &test_arena);
    UnaryOpNode *negate_node = create_unary_op_node(OPERATOR_NEGATE, (AstNode *) int_node, &test_arena);
    UnaryOpNode *complement_node = create_unary_op_node(OPERATOR_COMPLEMENT, (AstNode *) negate_node, &test_arena);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *) complement_node, &test_arena);
    BlockNode *body_block = create_block_node(&test_arena);
    block_node_add_item(body_block, (AstNode *) return_node, &test_arena);
    FuncDefNode *func_node = create_func_def_node("main", body_block, &test_arena);
    ProgramNode *ast = create_program_node(func_node, &test_arena);

    // 2. Translate AST to TAC
    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, func->instruction_count, "Expected 3 TAC instructions for ~(-2)");

    // Check the NEGATE instruction (t0 = -2)
    const TacInstruction *instr0 = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_NEGATE, instr0->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr0->operands.unary_op.dst.type);
    TEST_ASSERT_EQUAL_INT(0, instr0->operands.unary_op.dst.value.temp.id); // t0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0->operands.unary_op.src.type);
    TEST_ASSERT_EQUAL_INT(2, instr0->operands.unary_op.src.value.constant_value);

    // Check the COMPLEMENT instruction (t1 = ~t0)
    const TacInstruction *instr1 = &func->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_COMPLEMENT, instr1->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr1->operands.unary_op.dst.type);
    TEST_ASSERT_EQUAL_INT(1, instr1->operands.unary_op.dst.value.temp.id); // t1
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr1->operands.unary_op.src.type);
    TEST_ASSERT_EQUAL_INT(0, instr1->operands.unary_op.src.value.temp.id); // t0

    // Check the RETURN instruction (RETURN t1)
    const TacInstruction *instr2 = &func->instructions[2];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr2->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr2->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(1, instr2->operands.ret.src.value.temp.id); // t1

    // Clean up arena
    arena_destroy(&test_arena);
}

// Test for: int main() { return 5 + 3; }
static void test_return_binary_add_literals(void) {
    // Create arena for this test
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for binary add test");

    // 1. Construct AST for: int main() { return 5 + 3; }
    IntLiteralNode *lhs_literal = create_int_literal_node(5, &test_arena);
    IntLiteralNode *rhs_literal = create_int_literal_node(3, &test_arena);
    BinaryOpNode *binary_add_node = create_binary_op_node(OPERATOR_ADD, (AstNode *) lhs_literal,
                                                          (AstNode *) rhs_literal, &test_arena);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *) binary_add_node, &test_arena);
    BlockNode *body_block = create_block_node(&test_arena);
    block_node_add_item(body_block, (AstNode *) return_node, &test_arena);
    FuncDefNode *func_node = create_func_def_node("main", body_block, &test_arena);
    ProgramNode *ast = create_program_node(func_node, &test_arena);

    // 2. Translate AST to TAC
    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, func->instruction_count, "Expected 2 TAC instructions for 5 + 3");

    // Check the ADD instruction (t0 = 5 + 3)
    const TacInstruction *instr0 = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_ADD, instr0->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr0->operands.binary_op.dst.type);
    TEST_ASSERT_EQUAL_INT(0, instr0->operands.binary_op.dst.value.temp.id); // t0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0->operands.binary_op.src1.type);
    TEST_ASSERT_EQUAL_INT(5, instr0->operands.binary_op.src1.value.constant_value);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0->operands.binary_op.src2.type);
    TEST_ASSERT_EQUAL_INT(3, instr0->operands.binary_op.src2.value.constant_value);

    // Check the RETURN instruction (RETURN t0)
    const TacInstruction *instr1 = &func->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr1->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr1->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(0, instr1->operands.ret.src.value.temp.id); // t0

    // Clean up arena
    arena_destroy(&test_arena);
}

// Test for: int main() { return !0; }
static void test_return_logical_not(void) {
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for logical NOT test");

    // 1. Construct AST for: int main() { return !0; }
    IntLiteralNode *zero_literal = create_int_literal_node(0, &test_arena);
    UnaryOpNode *logical_not_node = create_unary_op_node(OPERATOR_LOGICAL_NOT, (AstNode *) zero_literal, &test_arena);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *) logical_not_node, &test_arena);
    BlockNode *body_block = create_block_node(&test_arena);
    block_node_add_item(body_block, (AstNode *) return_node, &test_arena);
    FuncDefNode *func_node = create_func_def_node("main", body_block, &test_arena);
    ProgramNode *ast = create_program_node(func_node, &test_arena);

    // 2. Translate AST to TAC
    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, func->instruction_count, "Expected 2 TAC instructions for !0");

    // Check the LOGICAL_NOT instruction (t0 = ! 0)
    const TacInstruction *instr0 = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_LOGICAL_NOT, instr0->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr0->operands.unary_op.dst.type);
    TEST_ASSERT_EQUAL_INT(0, instr0->operands.unary_op.dst.value.temp.id); // t0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0->operands.unary_op.src.type);
    TEST_ASSERT_EQUAL_INT(0, instr0->operands.unary_op.src.value.constant_value);

    // Check the RETURN instruction (RETURN t0)
    const TacInstruction *instr1 = &func->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr1->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr1->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(0, instr1->operands.ret.src.value.temp.id); // t0

    arena_destroy(&test_arena);
}

// Test for: int main() { return 3 < 5; }
static void test_return_relational_less(void) {
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for relational LESS test");

    // 1. Construct AST for: int main() { return 3 < 5; }
    IntLiteralNode *lhs_literal = create_int_literal_node(3, &test_arena);
    IntLiteralNode *rhs_literal = create_int_literal_node(5, &test_arena);
    BinaryOpNode *less_node = create_binary_op_node(OPERATOR_LESS, (AstNode *) lhs_literal, (AstNode *) rhs_literal,
                                                    &test_arena);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *) less_node, &test_arena);
    BlockNode *body_block = create_block_node(&test_arena);
    block_node_add_item(body_block, (AstNode *) return_node, &test_arena);
    FuncDefNode *func_node = create_func_def_node("main", body_block, &test_arena);
    ProgramNode *ast = create_program_node(func_node, &test_arena);

    // 2. Translate AST to TAC
    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, func->instruction_count, "Expected 2 TAC instructions for 3 < 5");

    // Check the LESS instruction (t0 = 3 < 5)
    const TacInstruction *instr0 = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_LESS, instr0->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr0->operands.relational_op.dst.type);
    TEST_ASSERT_EQUAL_INT(0, instr0->operands.relational_op.dst.value.temp.id); // t0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0->operands.relational_op.src1.type);
    TEST_ASSERT_EQUAL_INT(3, instr0->operands.relational_op.src1.value.constant_value);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0->operands.relational_op.src2.type);
    TEST_ASSERT_EQUAL_INT(5, instr0->operands.relational_op.src2.value.constant_value);

    // Check the RETURN instruction (RETURN t0)
    const TacInstruction *instr1 = &func->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr1->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr1->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(0, instr1->operands.ret.src.value.temp.id); // t0

    arena_destroy(&test_arena);
}

// Test for: int main() { return 1 && 0; } (RHS evaluates, result is 0)
static void test_return_logical_and_rhs_evaluates(void) {
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for logical AND (RHS eval) test");

    // 1. Construct AST for: int main() { return 1 && 0; }
    IntLiteralNode *lhs_literal = create_int_literal_node(1, &test_arena); // LHS is true
    IntLiteralNode *rhs_literal = create_int_literal_node(0, &test_arena); // RHS is false
    BinaryOpNode *and_node = create_binary_op_node(OPERATOR_LOGICAL_AND, (AstNode *) lhs_literal,
                                                   (AstNode *) rhs_literal, &test_arena);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *) and_node, &test_arena);
    BlockNode *body_block = create_block_node(&test_arena);
    block_node_add_item(body_block, (AstNode *) return_node, &test_arena);
    FuncDefNode *func_node = create_func_def_node("main", body_block, &test_arena);
    ProgramNode *ast = create_program_node(func_node, &test_arena);

    // 2. Translate AST to TAC
    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name);
    // Expected: if_false const 1 goto L0, t0 = (rhs_result != 0), goto L1, L0:, t0 = 0, L1:, return t0
    TEST_ASSERT_EQUAL_INT_MESSAGE(7, func->instruction_count, "Expected 7 TAC items for 1 && 0");

    const TacInstruction *instr;
    int temp_id_dest = 0; // Assuming the first temp created is t0 for the result of &&

    // instr[0]: if_false const 1 goto L0
    instr = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_IF_FALSE_GOTO, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.conditional_goto.condition_src.type); // LHS is const 1
    TEST_ASSERT_EQUAL_INT(1, instr->operands.conditional_goto.condition_src.value.constant_value);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.conditional_goto.target_label.type);
    TEST_ASSERT_EQUAL_STRING("L0", instr->operands.conditional_goto.target_label.value.label_name);

    // instr[1]: t0 = (rhs_result != 0)  (e.g. t0 = (const 0 != const 0) -> t0 = 0)
    // Original AST: 1 && 0. So rhs_result is const 0.
    instr = &func->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_NOT_EQUAL, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr->operands.relational_op.dst.type);
    TEST_ASSERT_EQUAL_INT(temp_id_dest, instr->operands.relational_op.dst.value.temp.id); // t0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.relational_op.src1.type); // rhs_result (const 0)
    TEST_ASSERT_EQUAL_INT(0, instr->operands.relational_op.src1.value.constant_value);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.relational_op.src2.type); // const 0 to compare against
    TEST_ASSERT_EQUAL_INT(0, instr->operands.relational_op.src2.value.constant_value);

    // instr[2]: goto L1
    instr = &func->instructions[2];
    TEST_ASSERT_EQUAL_INT(TAC_INS_GOTO, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.go_to.target_label.type);
    TEST_ASSERT_EQUAL_STRING("L1", instr->operands.go_to.target_label.value.label_name);

    // instr[3]: L0:
    instr = &func->instructions[3];
    TEST_ASSERT_EQUAL_INT(TAC_INS_LABEL, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.label_def.label.type);
    TEST_ASSERT_EQUAL_STRING("L0", instr->operands.label_def.label.value.label_name);

    // instr[4]: t0 = 0 (dest_temp = false)
    instr = &func->instructions[4];
    TEST_ASSERT_EQUAL_INT(TAC_INS_COPY, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr->operands.copy.dst.type);
    TEST_ASSERT_EQUAL_INT(temp_id_dest, instr->operands.copy.dst.value.temp.id); // t0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.copy.src.type); // Assigning constant 0
    TEST_ASSERT_EQUAL_INT(0, instr->operands.copy.src.value.constant_value);

    // instr[5]: L1:
    instr = &func->instructions[5];
    TEST_ASSERT_EQUAL_INT(TAC_INS_LABEL, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.label_def.label.type);
    TEST_ASSERT_EQUAL_STRING("L1", instr->operands.label_def.label.value.label_name);

    // instr[6]: return t0
    instr = &func->instructions[6];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(temp_id_dest, instr->operands.ret.src.value.temp.id); // t0

    arena_destroy(&test_arena);
}

// Test for: int main() { return 0 && 1; } (Short-circuit, result is 0)
static void test_return_logical_and_short_circuit(void) {
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for logical AND (short-circuit) test");

    // 1. Construct AST for: int main() { return 0 && 1; }
    IntLiteralNode *lhs_literal = create_int_literal_node(0, &test_arena); // LHS is false
    IntLiteralNode *rhs_literal = create_int_literal_node(1, &test_arena); // RHS (should not be 'evaluated' logically)
    BinaryOpNode *and_node = create_binary_op_node(OPERATOR_LOGICAL_AND, (AstNode *) lhs_literal,
                                                   (AstNode *) rhs_literal, &test_arena);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *) and_node, &test_arena);
    BlockNode *body_block = create_block_node(&test_arena);
    block_node_add_item(body_block, (AstNode *) return_node, &test_arena);
    FuncDefNode *func_node = create_func_def_node("main", body_block, &test_arena);
    ProgramNode *ast = create_program_node(func_node, &test_arena);

    // 2. Translate AST to TAC
    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name);
    // Expected: if_false const 0 goto L0, t0 = (rhs_result != 0), goto L1, L0:, t0 = 0, L1:, return t0
    TEST_ASSERT_EQUAL_INT_MESSAGE(7, func->instruction_count, "Expected 7 TAC items for 0 && 1 (short-circuit)");

    const TacInstruction *instr;
    int temp_id_dest = 0; // Assuming the first temp created is t0 for the result of &&

    // instr[0]: if_false const 0 goto L0
    instr = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_IF_FALSE_GOTO, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.conditional_goto.condition_src.type); // LHS is const 0
    TEST_ASSERT_EQUAL_INT(0, instr->operands.conditional_goto.condition_src.value.constant_value);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.conditional_goto.target_label.type);
    TEST_ASSERT_EQUAL_STRING("L0", instr->operands.conditional_goto.target_label.value.label_name);

    // instr[1]: t0 = (rhs_result != 0) (e.g. t0 = (const 1 != const 0) -> t0 = 1)
    // This instruction is generated but skipped due to short-circuiting for 0 && 1.
    // Original AST: 0 && 1. So rhs_result is const 1.
    instr = &func->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_NOT_EQUAL, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr->operands.relational_op.dst.type);
    TEST_ASSERT_EQUAL_INT(temp_id_dest, instr->operands.relational_op.dst.value.temp.id); // t0 (dest_temp)
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.relational_op.src1.type); // rhs_result (const 1)
    TEST_ASSERT_EQUAL_INT(1, instr->operands.relational_op.src1.value.constant_value);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.relational_op.src2.type); // const 0 to compare against
    TEST_ASSERT_EQUAL_INT(0, instr->operands.relational_op.src2.value.constant_value);

    // instr[2]: goto L1
    instr = &func->instructions[2];
    TEST_ASSERT_EQUAL_INT(TAC_INS_GOTO, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.go_to.target_label.type);
    TEST_ASSERT_EQUAL_STRING("L1", instr->operands.go_to.target_label.value.label_name);

    // instr[3]: L0:
    instr = &func->instructions[3];
    TEST_ASSERT_EQUAL_INT(TAC_INS_LABEL, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.label_def.label.type);
    TEST_ASSERT_EQUAL_STRING("L0", instr->operands.label_def.label.value.label_name);

    // instr[4]: t0 = 0 (dest_temp = false - this path is taken)
    instr = &func->instructions[4];
    TEST_ASSERT_EQUAL_INT(TAC_INS_COPY, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr->operands.copy.dst.type);
    TEST_ASSERT_EQUAL_INT(temp_id_dest, instr->operands.copy.dst.value.temp.id); // t0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.copy.src.type);
    TEST_ASSERT_EQUAL_INT(0, instr->operands.copy.src.value.constant_value);

    // instr[5]: L1:
    instr = &func->instructions[5];
    TEST_ASSERT_EQUAL_INT(TAC_INS_LABEL, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.label_def.label.type);
    TEST_ASSERT_EQUAL_STRING("L1", instr->operands.label_def.label.value.label_name);

    // instr[6]: return t0
    instr = &func->instructions[6];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(temp_id_dest, instr->operands.ret.src.value.temp.id); // t0

    arena_destroy(&test_arena);
}

// Test for: int main() { return 0 || 1; } (LHS false, RHS evaluates, result is 1)
static void test_return_logical_or_rhs_evaluates(void) {
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for logical OR (RHS eval) test");

    // 1. Construct AST for: int main() { return 0 || 1; }
    IntLiteralNode *lhs_literal = create_int_literal_node(0, &test_arena); // LHS is false
    IntLiteralNode *rhs_literal = create_int_literal_node(1, &test_arena); // RHS is true
    BinaryOpNode *or_node = create_binary_op_node(OPERATOR_LOGICAL_OR, (AstNode *) lhs_literal, (AstNode *) rhs_literal,
                                                  &test_arena);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *) or_node, &test_arena);
    BlockNode *body_block = create_block_node(&test_arena);
    block_node_add_item(body_block, (AstNode *) return_node, &test_arena);
    FuncDefNode *func_node = create_func_def_node("main", body_block, &test_arena);
    ProgramNode *ast = create_program_node(func_node, &test_arena);

    // 2. Translate AST to TAC
    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name);
    TEST_ASSERT_EQUAL_INT_MESSAGE(7, func->instruction_count, "Expected 7 TAC items for 0 || 1");

    const TacInstruction *instr;
    int temp_id_dest = 0; // Assuming t0 is the destination temporary for ||

    // instr[0]: if_true const 0 goto L_true_exit (L1)
    instr = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_IF_TRUE_GOTO, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.conditional_goto.condition_src.type);
    TEST_ASSERT_EQUAL_INT(0, instr->operands.conditional_goto.condition_src.value.constant_value); // LHS is const 0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.conditional_goto.target_label.type);
    TEST_ASSERT_EQUAL_STRING("L1", instr->operands.conditional_goto.target_label.value.label_name);

    // instr[1]: t0 = (rhs_result != 0) (e.g. t0 = (const 1 != const 0) -> t0 = 1)
    instr = &func->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_NOT_EQUAL, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr->operands.relational_op.dst.type);
    TEST_ASSERT_EQUAL_INT(temp_id_dest, instr->operands.relational_op.dst.value.temp.id); // t0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.relational_op.src1.type); // rhs_result (const 1)
    TEST_ASSERT_EQUAL_INT(1, instr->operands.relational_op.src1.value.constant_value);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.relational_op.src2.type); // const 0 to compare against
    TEST_ASSERT_EQUAL_INT(0, instr->operands.relational_op.src2.value.constant_value);

    // instr[2]: goto L_end (L2)
    instr = &func->instructions[2];
    TEST_ASSERT_EQUAL_INT(TAC_INS_GOTO, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.go_to.target_label.type);
    TEST_ASSERT_EQUAL_STRING("L2", instr->operands.go_to.target_label.value.label_name);

    // instr[3]: L_true_exit: (L1)
    instr = &func->instructions[3];
    TEST_ASSERT_EQUAL_INT(TAC_INS_LABEL, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.label_def.label.type);
    TEST_ASSERT_EQUAL_STRING("L1", instr->operands.label_def.label.value.label_name);

    // instr[4]: t0 = 1 (if LHS was true path)
    instr = &func->instructions[4];
    TEST_ASSERT_EQUAL_INT(TAC_INS_COPY, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr->operands.copy.dst.type);
    TEST_ASSERT_EQUAL_INT(temp_id_dest, instr->operands.copy.dst.value.temp.id); // t0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.copy.src.type);
    TEST_ASSERT_EQUAL_INT(1, instr->operands.copy.src.value.constant_value);

    // instr[5]: L_end: (L2)
    instr = &func->instructions[5];
    TEST_ASSERT_EQUAL_INT(TAC_INS_LABEL, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.label_def.label.type);
    TEST_ASSERT_EQUAL_STRING("L2", instr->operands.label_def.label.value.label_name);

    // instr[6]: return t0
    instr = &func->instructions[6];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(temp_id_dest, instr->operands.ret.src.value.temp.id); // t0

    arena_destroy(&test_arena);
}

// Test for: int main() { return 1 || 0; } (LHS true, short-circuit, result is 1)
static void test_return_logical_or_short_circuit(void) {
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena for logical OR (short-circuit) test");

    // 1. Construct AST for: int main() { return 1 || 0; }
    IntLiteralNode *lhs_literal = create_int_literal_node(1, &test_arena); // LHS is true
    IntLiteralNode *rhs_literal = create_int_literal_node(0, &test_arena); // RHS is false
    BinaryOpNode *or_node = create_binary_op_node(OPERATOR_LOGICAL_OR, (AstNode *) lhs_literal, (AstNode *) rhs_literal,
                                                  &test_arena);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *) or_node, &test_arena);
    BlockNode *body_block = create_block_node(&test_arena);
    block_node_add_item(body_block, (AstNode *) return_node, &test_arena);
    FuncDefNode *func_node = create_func_def_node("main", body_block, &test_arena);
    ProgramNode *ast = create_program_node(func_node, &test_arena);

    // 2. Translate AST to TAC
    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    // 3. Assertions
    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name);
    TEST_ASSERT_EQUAL_INT_MESSAGE(7, func->instruction_count, "Expected 7 TAC items for 1 || 0 (short-circuit)");

    const TacInstruction *instr;
    int temp_id_dest = 0; // Assuming t0 is the destination temporary for ||

    // instr[0]: if_true const 1 goto L_true_exit (L1)
    instr = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_IF_TRUE_GOTO, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.conditional_goto.condition_src.type);
    TEST_ASSERT_EQUAL_INT(1, instr->operands.conditional_goto.condition_src.value.constant_value); // LHS is const 1
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.conditional_goto.target_label.type);
    TEST_ASSERT_EQUAL_STRING("L1", instr->operands.conditional_goto.target_label.value.label_name); // L_true_exit

    // Path for 1 || 0 (LHS is true, short-circuits):
    // instr[1]: t0 = (const 0 != const 0) -> t0 = 0 (Generated, but skipped. RHS is 0)
    instr = &func->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_NOT_EQUAL, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr->operands.relational_op.dst.type);
    TEST_ASSERT_EQUAL_INT(temp_id_dest, instr->operands.relational_op.dst.value.temp.id); // t0 (dest_temp)
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.relational_op.src1.type); // rhs_result (const 0)
    TEST_ASSERT_EQUAL_INT(0, instr->operands.relational_op.src1.value.constant_value);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.relational_op.src2.type); // const 0 to compare against
    TEST_ASSERT_EQUAL_INT(0, instr->operands.relational_op.src2.value.constant_value);

    // instr[2]: goto L_end (L2) (Generated, but skipped)
    instr = &func->instructions[2];
    TEST_ASSERT_EQUAL_INT(TAC_INS_GOTO, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.go_to.target_label.type);
    TEST_ASSERT_EQUAL_STRING("L2", instr->operands.go_to.target_label.value.label_name); // L_end_logical_or is L2

    // instr[3]: L_true_exit: (L1)
    instr = &func->instructions[3];
    TEST_ASSERT_EQUAL_INT(TAC_INS_LABEL, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.label_def.label.type);
    TEST_ASSERT_EQUAL_STRING("L1", instr->operands.label_def.label.value.label_name);

    // instr[4]: t0 = 1 (Executed, because LHS was true)
    instr = &func->instructions[4];
    TEST_ASSERT_EQUAL_INT(TAC_INS_COPY, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr->operands.copy.dst.type);
    TEST_ASSERT_EQUAL_INT(temp_id_dest, instr->operands.copy.dst.value.temp.id); // t0
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr->operands.copy.src.type);
    TEST_ASSERT_EQUAL_INT(1, instr->operands.copy.src.value.constant_value); // Assign 1 because LHS was true

    // instr[5]: L_end: (L2)
    instr = &func->instructions[5];
    TEST_ASSERT_EQUAL_INT(TAC_INS_LABEL, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_LABEL, instr->operands.label_def.label.type);
    TEST_ASSERT_EQUAL_STRING("L2", instr->operands.label_def.label.value.label_name);

    // instr[6]: return t0
    instr = &func->instructions[6];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(temp_id_dest, instr->operands.ret.src.value.temp.id); // t0

    arena_destroy(&test_arena);
}


// --- New Test Case Implementations for Variable Declarations ---

static void test_var_decl_no_initializer(void) {
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    // AST for: int main() { int x; return 0; } (return 0 to have at least one instruction)
    // VarDeclNode for 'int x;'
    VarDeclNode *var_x = create_var_decl_node("int", "x", NULL, &test_arena);
    var_x->tac_temp_id = 0; // Manually set for test purposes
    var_x->tac_name_hint = arena_strdup("x_0", &test_arena);

    IntLiteralNode *ret_val_node = create_int_literal_node(0, &test_arena);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *)ret_val_node, &test_arena);

    BlockNode *body_block = create_block_node(&test_arena);
    block_node_add_item(body_block, (AstNode *)var_x, &test_arena);
    block_node_add_item(body_block, (AstNode *)return_node, &test_arena);

    FuncDefNode *func_node = create_func_def_node("main", body_block, &test_arena);
    ProgramNode *ast = create_program_node(func_node, &test_arena);

    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    // Expect 1 instruction: RETURN 0. No TAC for 'int x;' itself.
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, func->instruction_count, "Expected 1 instruction for 'int x; return 0;'"); 

    const TacInstruction *instr0 = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr0->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(0, instr0->operands.ret.src.value.constant_value);

    arena_destroy(&test_arena);
}

static void test_var_decl_with_literal_initializer(void) {
    Arena test_arena = arena_create(4096);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    // AST for: int main() { int x = 10; return x; }
    IntLiteralNode *init_val_node = create_int_literal_node(10, &test_arena);
    VarDeclNode *var_x = create_var_decl_node("int", "x", (AstNode *)init_val_node, &test_arena);
    var_x->tac_temp_id = 0; // Manually set for test purposes
    var_x->tac_name_hint = arena_strdup("x_0", &test_arena);

    IdentifierNode *ret_id_node = create_identifier_node("x", &test_arena);
    ret_id_node->tac_temp_id = 0; // x uses t0
    ret_id_node->tac_name_hint = arena_strdup("x_0", &test_arena);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *)ret_id_node, &test_arena);

    BlockNode *body_block = create_block_node(&test_arena);
    block_node_add_item(body_block, (AstNode *)var_x, &test_arena);
    block_node_add_item(body_block, (AstNode *)return_node, &test_arena);

    FuncDefNode *func_node = create_func_def_node("main", body_block, &test_arena);
    ProgramNode *ast = create_program_node(func_node, &test_arena);

    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_STRING("main", func->name);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, func->instruction_count, "Expected 2 instructions for 'int x = 10; return x;'");

    // 1. t0 = 10 (TAC_INS_COPY for initialization)
    const TacInstruction *instr0 = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_COPY, instr0->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr0->operands.copy.dst.type);
    TEST_ASSERT_EQUAL_INT(0, instr0->operands.copy.dst.value.temp.id); // x (t0)
    TEST_ASSERT_EQUAL_STRING("x_0", instr0->operands.copy.dst.value.temp.name_hint);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0->operands.copy.src.type);
    TEST_ASSERT_EQUAL_INT(10, instr0->operands.copy.src.value.constant_value);

    // 2. RETURN t0
    const TacInstruction *instr1 = &func->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr1->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr1->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(0, instr1->operands.ret.src.value.temp.id); // return x (t0)
    TEST_ASSERT_EQUAL_STRING("x_0", instr1->operands.ret.src.value.temp.name_hint);

    arena_destroy(&test_arena);
}

static void test_var_decl_with_expression_initializer(void) {
    Arena test_arena = arena_create(8192); // Increased arena size for more complex AST
    TEST_ASSERT_NOT_NULL_MESSAGE(test_arena.start, "Failed to create test arena");

    // AST for: int main() { int y = 5; int x = y + 2; return x; }

    // int y = 5;
    IntLiteralNode *init_y_val_node = create_int_literal_node(5, &test_arena);
    VarDeclNode *var_y = create_var_decl_node("int", "y", (AstNode *)init_y_val_node, &test_arena);
    var_y->tac_temp_id = 0; // y is t0
    var_y->tac_name_hint = arena_strdup("y_0", &test_arena);

    // y + 2 (for x's initializer)
    IdentifierNode *id_y_for_expr = create_identifier_node("y", &test_arena);
    id_y_for_expr->tac_temp_id = 0; // y is t0
    id_y_for_expr->tac_name_hint = arena_strdup("y_0", &test_arena);
    IntLiteralNode *const_2_node = create_int_literal_node(2, &test_arena);
    BinaryOpNode *add_expr_node = create_binary_op_node(OPERATOR_ADD, (AstNode *)id_y_for_expr, (AstNode *)const_2_node, &test_arena);

    // int x = y + 2;
    VarDeclNode *var_x = create_var_decl_node("int", "x", (AstNode *)add_expr_node, &test_arena);
    var_x->tac_temp_id = 1; // x is t1 (assuming y's init uses t0, add_expr uses t1, then x gets t1)
                            // More precisely, x will be assigned the result of the expression, which will be in a temp.
                            // The validator should assign distinct temp_ids for x and y.
                            // Let's assume x is t2, and the expression y+2 results in t1.
    var_x->tac_temp_id = 2; // x is t2
    var_x->tac_name_hint = arena_strdup("x_2", &test_arena);

    // return x;
    IdentifierNode *ret_id_x_node = create_identifier_node("x", &test_arena);
    ret_id_x_node->tac_temp_id = 2; // x is t2
    ret_id_x_node->tac_name_hint = arena_strdup("x_2", &test_arena);
    ReturnStmtNode *return_node = create_return_stmt_node((AstNode *)ret_id_x_node, &test_arena);

    BlockNode *body_block = create_block_node(&test_arena);
    block_node_add_item(body_block, (AstNode *)var_y, &test_arena);
    block_node_add_item(body_block, (AstNode *)var_x, &test_arena);
    block_node_add_item(body_block, (AstNode *)return_node, &test_arena);

    FuncDefNode *func_node = create_func_def_node("main", body_block, &test_arena);
    ProgramNode *ast = create_program_node(func_node, &test_arena);

    TacProgram *tac_program = ast_to_tac(ast, &test_arena);

    TEST_ASSERT_NOT_NULL(tac_program);
    TEST_ASSERT_EQUAL_INT(1, tac_program->function_count);
    TacFunction *func = tac_program->functions[0];
    TEST_ASSERT_NOT_NULL(func);
    // Expected: 
    // 1. t0 = 5          (y = 5)
    // 2. t1 = t0 + 2     (y + 2)
    // 3. t2 = t1         (x = result of y+2)
    // 4. RETURN t2       (return x)
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, func->instruction_count, "Expected 4 instructions");

    // 1. t0 = 5 (y = 5)
    const TacInstruction *instr0 = &func->instructions[0];
    TEST_ASSERT_EQUAL_INT(TAC_INS_COPY, instr0->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr0->operands.copy.dst.type);
    TEST_ASSERT_EQUAL_INT(0, instr0->operands.copy.dst.value.temp.id); // y (t0)
    TEST_ASSERT_EQUAL_STRING("y_0", instr0->operands.copy.dst.value.temp.name_hint);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr0->operands.copy.src.type);
    TEST_ASSERT_EQUAL_INT(5, instr0->operands.copy.src.value.constant_value);

    // 2. t1 = t0 + 2 (y + 2)
    const TacInstruction *instr1 = &func->instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_INS_ADD, instr1->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr1->operands.binary_op.dst.type); // Result of y+2 goes into a new temp, t1
    TEST_ASSERT_EQUAL_INT(1, instr1->operands.binary_op.dst.value.temp.id); // t1
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr1->operands.binary_op.src1.type);
    TEST_ASSERT_EQUAL_INT(0, instr1->operands.binary_op.src1.value.temp.id); // y (t0)
    TEST_ASSERT_EQUAL_STRING("y_0", instr1->operands.binary_op.src1.value.temp.name_hint);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_CONST, instr1->operands.binary_op.src2.type);
    TEST_ASSERT_EQUAL_INT(2, instr1->operands.binary_op.src2.value.constant_value);

    // 3. t2 = t1 (x = result of y+2)
    const TacInstruction *instr2 = &func->instructions[2];
    TEST_ASSERT_EQUAL_INT(TAC_INS_COPY, instr2->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr2->operands.copy.dst.type);
    TEST_ASSERT_EQUAL_INT(2, instr2->operands.copy.dst.value.temp.id); // x (t2)
    TEST_ASSERT_EQUAL_STRING("x_2", instr2->operands.copy.dst.value.temp.name_hint);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr2->operands.copy.src.type);
    TEST_ASSERT_EQUAL_INT(1, instr2->operands.copy.src.value.temp.id); // result of y+2 (t1)

    // 4. RETURN t2 (return x)
    const TacInstruction *instr3 = &func->instructions[3];
    TEST_ASSERT_EQUAL_INT(TAC_INS_RETURN, instr3->type);
    TEST_ASSERT_EQUAL_INT(TAC_OPERAND_TEMP, instr3->operands.ret.src.type);
    TEST_ASSERT_EQUAL_INT(2, instr3->operands.ret.src.value.temp.id); // x (t2)
    TEST_ASSERT_EQUAL_STRING("x_2", instr3->operands.ret.src.value.temp.name_hint);

    arena_destroy(&test_arena);
}


// --- Test Runner ---

void run_ast_to_tac_tests(void) {
    RUN_TEST(test_return_int_literal);
    RUN_TEST(test_return_unary_negate);
    RUN_TEST(test_return_unary_complement);
    RUN_TEST(test_return_unary_complement_negate);
    RUN_TEST(test_return_binary_add_literals);
    RUN_TEST(test_return_logical_not);
    RUN_TEST(test_return_relational_less);
    RUN_TEST(test_return_logical_and_rhs_evaluates);
    RUN_TEST(test_return_logical_and_short_circuit);
    RUN_TEST(test_return_logical_or_rhs_evaluates);
    RUN_TEST(test_return_logical_or_short_circuit);
    // Add new tests here
    RUN_TEST(test_var_decl_no_initializer);
    RUN_TEST(test_var_decl_with_literal_initializer);
    RUN_TEST(test_var_decl_with_expression_initializer);
}
