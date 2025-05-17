#include "ast_to_tac.h"

#include <stdio.h>  // For error reporting (potentially)
#include <stdlib.h> // For NULL
#include <stdbool.h>// For bool type
#include <string.h> // For strcpy and strlen for label generation

// --- Static Helper Function Declarations ---

// Forward declaration for visiting statements (like ReturnStmt)
static void visit_statement(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr);

// Forward declaration for visiting expressions (which produce a value)
static TacOperand visit_expression(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr);

// Helper to generate unique label names
static const char *generate_label_name(Arena *arena, int *label_counter_ptr) {
    char buffer[32]; // Sufficient for "L" + number
    sprintf(buffer, "L%d", (*label_counter_ptr)++);
    char *label_str = arena_alloc(arena, strlen(buffer) + 1);
    if (label_str) {
        strcpy(label_str, buffer);
    }
    return label_str;
}

// Helper to create an invalid operand (useful for nodes that don't return a value)
static TacOperand create_invalid_operand() {
    return (TacOperand){.type = -1}; // Use an invalid type marker
}

// Helper to check if an operand is valid
static bool is_valid_operand(TacOperand op) {
    return op.type != -1;
}

// --- Main Translation Function ---

TacProgram *ast_to_tac(ProgramNode *ast_root, Arena *arena) {
    if (!ast_root || ast_root->base.type != NODE_PROGRAM) {
        fprintf(stderr, "Error: Invalid AST root node for TAC generation.\n");
        return NULL;
    }

    TacProgram *tac_program = create_tac_program(arena);
    if (!tac_program) {
        fprintf(stderr, "Error: Failed to create TAC program.\n");
        return NULL;
    }

    // Assume cleric only supports one function ('main') for now
    if (!ast_root->function || ast_root->function->base.type != NODE_FUNC_DEF) {
        fprintf(stderr, "Error: AST ProgramNode does not contain a valid FuncDefNode.\n");
        // Consider cleaning up tac_program? Maybe not needed with arena.
        return NULL;
    }

    const FuncDefNode *func_def = ast_root->function;

    // Create the TacFunction corresponding to the FuncDefNode
    TacFunction *tac_function = create_tac_function(func_def->name, arena);
    if (!tac_function) {
        fprintf(stderr, "Error: Failed to create TAC function for '%s'.\n", func_def->name);
        return NULL;
    }

    // Add the function to the program
    add_function_to_program(tac_program, tac_function, arena);

    // Visit the function body (statements)
    // Initialize the temporary ID counter and label counter for this function
    int next_temp_id = 0;
    int label_counter = 0;
    // For now, assume the body is a single statement (like the ReturnStmt)
    // A real implementation would handle blocks/sequences of statements.
    if (func_def->body) {
        visit_statement(func_def->body, tac_function, arena, &next_temp_id, &label_counter);
    } else {
        // Handle functions with empty bodies if necessary
        fprintf(stderr, "Warning: Function '%s' has an empty body.\n", func_def->name);
    }

    return tac_program;
}

// --- Static Helper Function Implementations ---

// Visitor for statement nodes
static void visit_statement(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr) {
    // ReSharper disable once CppDFAConstantConditions
    // ReSharper disable once CppDFAUnreachableCode
    if (!node) return;

    switch (node->type) {
        case NODE_RETURN_STMT: {
            const ReturnStmtNode *ret_node = (ReturnStmtNode *) node;
            // Visit the expression to generate its TAC and get the result operand
            const TacOperand result_operand = visit_expression(ret_node->expression, current_function, arena,
                                                               next_temp_id_ptr, label_counter_ptr);

            // Check if the expression produced a valid result
            if (!is_valid_operand(result_operand)) {
                fprintf(stderr, "Error: Return statement's expression did not yield a valid result operand.\n");
                // Potentially set an error flag in the context?
                return;
            }

            // Create and add the RETURN instruction
            const TacInstruction *ret_instr = create_tac_instruction_return(result_operand, arena);
            if (ret_instr) {
                add_instruction_to_function(current_function, ret_instr, arena);
            } else {
                fprintf(stderr, "Error: Failed to create TAC RETURN instruction.\n");
            }
            break;
        }
        // Add cases for other statements here
        default:
            fprintf(stderr, "Error: Unexpected AST node type %d in visit_statement.\n", node->type);
            break;
    }
}

// Helper function to handle unary operations
static TacOperand visit_unary_op_expression(const UnaryOpNode *unary_node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr) {
    // 1. Visit the operand expression first
    const TacOperand operand_result = visit_expression(unary_node->operand, current_function, arena,
                                                       next_temp_id_ptr, label_counter_ptr);
    if (!is_valid_operand(operand_result)) {
        fprintf(stderr, "Error: Unary operator's operand did not yield a valid result.\n");
        return create_invalid_operand();
    }

    // 2. Create a new temporary to store the result of the unary operation
    //    Use the counter passed via pointer.
    const TacOperand dest_temp = create_tac_operand_temp((*next_temp_id_ptr)++);

    // 3. Create the specific TAC instruction based on the operator
    const TacInstruction *unary_instr = NULL;
    switch (unary_node->op) {
        case OPERATOR_NEGATE:
            unary_instr = create_tac_instruction_negate(dest_temp, operand_result, arena);
            break;
        case OPERATOR_COMPLEMENT:
            unary_instr = create_tac_instruction_complement(dest_temp, operand_result, arena);
            break;
        case OPERATOR_LOGICAL_NOT:
            unary_instr = create_tac_instruction_logical_not(dest_temp, operand_result, arena);
            break;
        default:
            fprintf(stderr, "Error: Unsupported unary operator type %d.\n", unary_node->op);
            return create_invalid_operand();
    }

    // 4. Add the instruction to the function
    if (unary_instr) {
        add_instruction_to_function(current_function, unary_instr, arena);
    } else {
        fprintf(stderr, "Error: Failed to create TAC instruction for unary operator %d.\n", unary_node->op);
        return create_invalid_operand(); // Failed to create instruction
    }

    // 5. The result of this expression is the destination temporary
    return dest_temp;
}

// Helper function to handle LOGICAL_AND operations (short-circuiting)
static TacOperand visit_logical_and_expression(const BinaryOpNode *binary_node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr) {
    // LHS && RHS
    // 1. Evaluate LHS
    TacOperand lhs_result = visit_expression(binary_node->left, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    if (!is_valid_operand(lhs_result)) {
        fprintf(stderr, "Error: LOGICAL_AND's left operand did not yield a valid result.\n");
        return create_invalid_operand();
    }

    // 2. Create a temporary for the final result of the AND expression
    TacOperand dest_temp = create_tac_operand_temp((*next_temp_id_ptr)++);

    // 3. Create labels for short-circuiting
    const char *false_exit_label_name = generate_label_name(arena, label_counter_ptr);
    TacOperand false_exit_label = create_tac_operand_label(false_exit_label_name, arena);
    const char *end_label_name = generate_label_name(arena, label_counter_ptr);
    TacOperand end_label = create_tac_operand_label(end_label_name, arena);

    // 4. if_false lhs_result goto L_false_exit
    TacInstruction *if_false_instr = create_tac_instruction_if_false_goto(lhs_result, false_exit_label, arena);
    add_instruction_to_function(current_function, if_false_instr, arena);

    // 5. Evaluate RHS (only if LHS was true)
    TacOperand rhs_result = visit_expression(binary_node->right, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    if (!is_valid_operand(rhs_result)) {
        fprintf(stderr, "Error: LOGICAL_AND's right operand did not yield a valid result.\n");
        // Still need to emit the labels for correct control flow even on error
        TacInstruction *define_false_label_instr = create_tac_instruction_label(false_exit_label, arena);
        add_instruction_to_function(current_function, define_false_label_instr, arena);
        TacInstruction *assign_false_instr = create_tac_instruction_copy(dest_temp, create_tac_operand_const(0), arena);
        add_instruction_to_function(current_function, assign_false_instr, arena);
        TacInstruction *define_end_label_instr = create_tac_instruction_label(end_label, arena);
        add_instruction_to_function(current_function, define_end_label_instr, arena);
        return create_invalid_operand();
    }
    // 6. dest_temp = rhs_result (if RHS is evaluated, its result is the result of the AND)
    //    However, logical AND should result in 0 or 1. So if rhs_result is non-zero, dest_temp is 1, else 0.
    TacInstruction *assign_rhs_bool_instr = create_tac_instruction_not_equal(dest_temp, rhs_result, create_tac_operand_const(0), arena);
    add_instruction_to_function(current_function, assign_rhs_bool_instr, arena);

    // 7. goto L_end
    TacInstruction *goto_end_instr = create_tac_instruction_goto(end_label, arena);
    add_instruction_to_function(current_function, goto_end_instr, arena);

    // 8. L_false_exit:
    TacInstruction *false_label_def_instr = create_tac_instruction_label(false_exit_label, arena);
    add_instruction_to_function(current_function, false_label_def_instr, arena);

    // 9. dest_temp = 0 (false)
    TacInstruction *assign_false_val_instr = create_tac_instruction_copy(dest_temp, create_tac_operand_const(0), arena);
    add_instruction_to_function(current_function, assign_false_val_instr, arena);

    // 10. L_end:
    TacInstruction *end_label_def_instr = create_tac_instruction_label(end_label, arena);
    add_instruction_to_function(current_function, end_label_def_instr, arena);

    // 11. The result of this expression is dest_temp
    return dest_temp;
}

// Helper function to handle LOGICAL_OR operations (short-circuiting)
static TacOperand visit_logical_or_expression(const BinaryOpNode *binary_node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr) {
    // LHS || RHS
    // 1. Evaluate LHS
    TacOperand lhs_result = visit_expression(binary_node->left, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    if (!is_valid_operand(lhs_result)) {
        fprintf(stderr, "Error: LOGICAL_OR's left operand did not yield a valid result.\n");
        return create_invalid_operand();
    }

    // 2. Create a temporary for the final result
    TacOperand dest_temp = create_tac_operand_temp((*next_temp_id_ptr)++);

    // 3. Create labels
    const char *eval_rhs_label_name = generate_label_name(arena, label_counter_ptr);
    TacOperand eval_rhs_label = create_tac_operand_label(eval_rhs_label_name, arena);
    const char *true_exit_label_name = generate_label_name(arena, label_counter_ptr);
    TacOperand true_exit_label = create_tac_operand_label(true_exit_label_name, arena);
    const char *end_label_name = generate_label_name(arena, label_counter_ptr);
    TacOperand end_label = create_tac_operand_label(end_label_name, arena);

    // 4. if_true lhs_result goto L_true_exit (If LHS is true, jump to assign true and exit)
    TacInstruction *if_true_instr = create_tac_instruction_if_true_goto(lhs_result, true_exit_label, arena);
    add_instruction_to_function(current_function, if_true_instr, arena);

    // 5. LHS was false. Evaluate RHS. (This is the path if not short-circuited by LHS being true)
    TacOperand rhs_result = visit_expression(binary_node->right, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    if (!is_valid_operand(rhs_result)) {
        fprintf(stderr, "Error: LOGICAL_OR's right operand did not yield a valid result.\n");
        // Still need to emit labels for correct control flow even on error
        TacInstruction *define_true_label_instr = create_tac_instruction_label(true_exit_label, arena);
        add_instruction_to_function(current_function, define_true_label_instr, arena);
        TacInstruction *assign_true_instr_on_err = create_tac_instruction_copy(dest_temp, create_tac_operand_const(1), arena);
        add_instruction_to_function(current_function, assign_true_instr_on_err, arena);
        // We also need the eval_rhs label and end_label for consistent paths.
        TacInstruction *define_eval_rhs_label_instr = create_tac_instruction_label(eval_rhs_label, arena);
        add_instruction_to_function(current_function, define_eval_rhs_label_instr, arena);
        TacInstruction *define_end_label_instr = create_tac_instruction_label(end_label, arena);
        add_instruction_to_function(current_function, define_end_label_instr, arena);
        return create_invalid_operand();
    }
    // 6. dest_temp = (rhs_result != 0) (Result is true if RHS is non-zero, false otherwise)
    TacInstruction *assign_rhs_bool_instr = create_tac_instruction_not_equal(dest_temp, rhs_result, create_tac_operand_const(0), arena);
    add_instruction_to_function(current_function, assign_rhs_bool_instr, arena);
    //    goto L_end
    TacInstruction *goto_end_instr_from_rhs = create_tac_instruction_goto(end_label, arena);
    add_instruction_to_function(current_function, goto_end_instr_from_rhs, arena);

    // 7. L_true_exit: (LHS was true, or preparing path for it)
    TacInstruction *true_label_def_instr = create_tac_instruction_label(true_exit_label, arena);
    add_instruction_to_function(current_function, true_label_def_instr, arena);

    // 8. dest_temp = 1 (true)
    TacInstruction *assign_true_val_instr = create_tac_instruction_copy(dest_temp, create_tac_operand_const(1), arena);
    add_instruction_to_function(current_function, assign_true_val_instr, arena);
    // No need for goto L_end here, as L_end will follow or this path is taken via goto.

    // 9. L_end: (Path from RHS evaluation also jumps here)
    TacInstruction *end_label_def_instr = create_tac_instruction_label(end_label, arena);
    add_instruction_to_function(current_function, end_label_def_instr, arena);

    // Note: The eval_rhs_label is implicitly handled by the control flow where RHS is evaluated if LHS is false.
    // The explicit definition and jump to eval_rhs_label_name from original code might be slightly different. Reviewing that. 
    // The original OR was: if_false LSH goto eval_RHS; LHS_true_path: assign 1, goto end; eval_RHS_label: evaluate RHS, assign bool(RHS), end_label.
    // Current is: if_true LHS goto true_exit; evaluate RHS, assign bool(RHS), goto end; true_exit_label: assign 1; end_label.
    // These are logically equivalent for correct short-circuiting. The key is that RHS is skipped if LHS meets OR's condition.

    // 10. The result of this expression is dest_temp
    return dest_temp;
}

// Helper function to handle binary operations
static TacOperand visit_binary_op_expression(const BinaryOpNode *binary_node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr) {
    // Handle LOGICAL_AND and LOGICAL_OR separately due to short-circuiting
    if (binary_node->op == OPERATOR_LOGICAL_AND) {
        return visit_logical_and_expression(binary_node, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    }
    if (binary_node->op == OPERATOR_LOGICAL_OR) {
        return visit_logical_or_expression(binary_node, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    }
    // Standard binary operations (non-short-circuiting)
    // 1. Visit left and right operands
    const TacOperand left_operand = visit_expression(binary_node->left, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    if (!is_valid_operand(left_operand)) {
        fprintf(stderr, "Error: Binary operator's left operand did not yield a valid result.\n");
        return create_invalid_operand();
    }

    const TacOperand right_operand = visit_expression(binary_node->right, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    if (!is_valid_operand(right_operand)) {
        fprintf(stderr, "Error: Binary operator's right operand did not yield a valid result.\n");
        return create_invalid_operand();
    }

    // 2. Create a new temporary to store the result
    const TacOperand dest_temp = create_tac_operand_temp((*next_temp_id_ptr)++);

    // 3. Create the specific TAC instruction based on the operator
    const TacInstruction *binary_instr = NULL;
    switch (binary_node->op) {
        case OPERATOR_ADD:
            binary_instr = create_tac_instruction_add(dest_temp, left_operand, right_operand, arena);
            break;
        case OPERATOR_SUBTRACT:
            binary_instr = create_tac_instruction_sub(dest_temp, left_operand, right_operand, arena);
            break;
        case OPERATOR_MULTIPLY:
            binary_instr = create_tac_instruction_mul(dest_temp, left_operand, right_operand, arena);
            break;
        case OPERATOR_DIVIDE:
            binary_instr = create_tac_instruction_div(dest_temp, left_operand, right_operand, arena);
            break;
        case OPERATOR_MODULO:
            binary_instr = create_tac_instruction_mod(dest_temp, left_operand, right_operand, arena);
            break;
        case OPERATOR_LESS:
            binary_instr = create_tac_instruction_less(dest_temp, left_operand, right_operand, arena);
            break;
        case OPERATOR_GREATER:
            binary_instr = create_tac_instruction_greater(dest_temp, left_operand, right_operand, arena);
            break;
        case OPERATOR_LESS_EQUAL:
            binary_instr = create_tac_instruction_less_equal(dest_temp, left_operand, right_operand, arena);
            break;
        case OPERATOR_GREATER_EQUAL:
            binary_instr = create_tac_instruction_greater_equal(dest_temp, left_operand, right_operand, arena);
            break;
        case OPERATOR_EQUAL_EQUAL:
            binary_instr = create_tac_instruction_equal(dest_temp, left_operand, right_operand, arena);
            break;
        case OPERATOR_NOT_EQUAL:
            binary_instr = create_tac_instruction_not_equal(dest_temp, left_operand, right_operand, arena);
            break;
        // Logical Operators (to be implemented with short-circuiting)
        // case OPERATOR_LOGICAL_AND: // Handled above
        // case OPERATOR_LOGICAL_OR: // Handled above
        default:
            fprintf(stderr, "Error: Unsupported binary operator type %d (or not yet handled).\n", binary_node->op);
            return create_invalid_operand();
    }

    // 4. Add the instruction to the function
    if (binary_instr) {
        add_instruction_to_function(current_function, binary_instr, arena);
    } else {
        // This case might not be reached if create_tac_instruction_* functions exit on failure
        fprintf(stderr, "Error: Failed to create TAC instruction for binary operator %d.\n", binary_node->op);
        return create_invalid_operand();
    }

    // 5. The result of this expression is the destination temporary
    return dest_temp;
}

// Visitor for expression nodes (returns the operand holding the result)
static TacOperand visit_expression(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr) {
    if (!node) return create_invalid_operand();

    switch (node->type) {
        case NODE_INT_LITERAL: {
            const IntLiteralNode *int_node = (IntLiteralNode *) node;
            // Integer literals directly translate to constant operands
            return create_tac_operand_const(int_node->value);
        }
        case NODE_UNARY_OP: {
            const UnaryOpNode *unary_node = (UnaryOpNode *) node;
            return visit_unary_op_expression(unary_node, current_function, arena, next_temp_id_ptr, label_counter_ptr);
        }
        case NODE_BINARY_OP: {
            const BinaryOpNode *binary_node = (BinaryOpNode *) node;
            return visit_binary_op_expression(binary_node, current_function, arena, next_temp_id_ptr, label_counter_ptr);
        }
        // Add cases for other expressions (FunctionCall, etc.)
        default:
            fprintf(stderr, "Error: Unexpected AST node type %d in visit_expression.\n", node->type);
            return create_invalid_operand();
    }
}
