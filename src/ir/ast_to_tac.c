#include "ast_to_tac.h"

#include <stdio.h>  // For error reporting (potentially)
#include <stdlib.h> // For NULL
#include <stdbool.h>// For bool type
#include <string.h> // For strcpy and strlen for label generation

// --- Static Helper Function Declarations ---

// Forward declaration for visiting statements (like ReturnStmt)
static void visit_statement(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr,
                            int *label_counter_ptr);

// Forward declaration for visiting expressions (which produce a value)
static TacOperand visit_expression(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr,
                                   int *label_counter_ptr);

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

// Placeholder: Represents looking up a variable in a symbol table
// In a real compiler, this would involve a symbol table lookup
// and return the TacOperand (likely a temp) associated with the variable.
static TacOperand lookup_variable_operand(const char* var_name, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr) {
    // For now, let's assume every variable read creates a new temporary and
    // would require a load from memory. This is a simplification.
    // A proper implementation would use a symbol table to find the existing operand for 'var_name'.
    fprintf(stdout, "lookup_variable_operand: Placeholder for '%s'. Creating a new temp.\n", var_name);
    // This is not correct for a real compiler but serves as a placeholder
    // to ensure a valid TacOperand is returned.
    return create_tac_operand_temp((*next_temp_id_ptr)++, var_name);
}

// Helper to get the TacOperand for an l-value, typically an identifier.
// This is simplified; a real version would handle memory addresses, struct fields, etc.
static TacOperand visit_lvalue_expression(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr) {
    if (!node) return create_invalid_operand();

    switch (node->type) {
        case NODE_IDENTIFIER: {
            const IdentifierNode *id_node = (IdentifierNode *) node;
            // For an l-value, we'd typically get its address or the temp assigned to it.
            // This is simplified: assumes we use the same operand as r-value lookup.
            return lookup_variable_operand(id_node->name, current_function, arena, next_temp_id_ptr);
        }
        // Add cases for other l-value expressions like array indexing, dereferences, etc.
        default:
            fprintf(stderr, "Error: Unexpected AST node type %d for l-value in visit_lvalue_expression.\n", node->type);
            return create_invalid_operand();
    }
}

// Visitor for assignment expressions
static TacOperand visit_assignment_expression(AssignmentExpNode *assign_node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr) {
    // 1. Evaluate the right-hand side (the value to be assigned)
    TacOperand rhs_operand = visit_expression(assign_node->value, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    if (!is_valid_operand(rhs_operand)) {
        fprintf(stderr, "Error: Assignment expression's right-hand side did not yield a valid result.\n");
        return create_invalid_operand();
    }

    // 2. Determine the target (l-value) operand
    // The target of an assignment must be an l-value (e.g., variable, array element).
    // For simplicity, we assume it's an identifier for now.
    TacOperand lhs_operand = visit_lvalue_expression(assign_node->target, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    if (!is_valid_operand(lhs_operand)) {
        fprintf(stderr, "Error: Assignment expression's left-hand side is not a valid l-value.\n");
        return create_invalid_operand();
    }

    // 3. Create the TAC_INS_COPY instruction (target = value)
    const TacInstruction *copy_instr = create_tac_instruction_copy(lhs_operand, rhs_operand, arena);
    if (!copy_instr) {
        fprintf(stderr, "Error: Failed to create TAC instruction for assignment.\n");
        return create_invalid_operand();
    }
    add_instruction_to_function(current_function, copy_instr, arena);

    // 4. The result of an assignment expression is the value assigned (rhs_operand)
    return rhs_operand;
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
        visit_statement((AstNode*)func_def->body, tac_function, arena, &next_temp_id, &label_counter);
    } else {
        // Handle functions with empty bodies if necessary
        fprintf(stderr, "Warning: Function '%s' has an empty body.\n", func_def->name);
    }

    return tac_program;
}

// --- Static Helper Function Implementations ---

// Visitor for statement nodes
static void visit_statement(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr,
                            int *label_counter_ptr) {
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
        case NODE_BLOCK: {
            const BlockNode *block_node = (BlockNode *)node;
            for (size_t i = 0; i < block_node->num_items; ++i) {
                visit_statement(block_node->items[i], current_function, arena, next_temp_id_ptr, label_counter_ptr);
                // If an error occurs in a sub-statement, we might need a way to propagate it.
                // For now, assuming sub-calls will print errors and we continue or bail if they return a specific error code/flag.
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
static TacOperand visit_unary_op_expression(const UnaryOpNode *unary_node, TacFunction *current_function, Arena *arena,
                                            int *next_temp_id_ptr, int *label_counter_ptr) {
    // 1. Visit the operand expression first
    const TacOperand operand_result = visit_expression(unary_node->operand, current_function, arena, next_temp_id_ptr,
                                                       label_counter_ptr);
    if (!is_valid_operand(operand_result)) {
        fprintf(stderr, "Error: Unary operator's operand did not yield a valid result.\n");
        return create_invalid_operand();
    }

    // 2. Create a new temporary to store the result of the unary operation
    //    Use the counter passed via pointer.
    const TacOperand dest_temp = create_tac_operand_temp((*next_temp_id_ptr)++, "t");

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
// Generates TAC for an expression like `t_dest = t_lhs && t_rhs`.
// The exact temporary variable names (e.g., t0, t1) and label names (e.g., L0, L1)
// will be determined by the current state of `next_temp_id_ptr` and `label_counter_ptr`.
//
// Example TAC sequence:
//   ; ... Instructions to evaluate LHS into t_lhs ...
//   IF_FALSE t_lhs, L0
//   ; ... Instructions to evaluate RHS into t_rhs (only if LHS was true) ...
//   t_dest = t_rhs != const 0
//   GOTO L1
// L0:
//   t_dest = const 0
// L1:
//   ; t_dest now holds the boolean result (0 or 1) of the AND operation
static TacOperand visit_logical_and_expression(const BinaryOpNode *binary_node, TacFunction *current_function,
                                               Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr) {
    // LHS && RHS
    // 1. Evaluate LHS
    const TacOperand lhs_result = visit_expression(binary_node->left, current_function, arena, next_temp_id_ptr,
                                                   label_counter_ptr);
    if (!is_valid_operand(lhs_result)) {
        fprintf(stderr, "Error: LOGICAL_AND's left operand did not yield a valid result.\n");
        return create_invalid_operand();
    }

    // 2. Create a temporary for the final result of the AND expression
    const TacOperand dest_temp = create_tac_operand_temp((*next_temp_id_ptr)++, "t");

    // 3. Create labels for short-circuiting
    const char *false_exit_label_name = generate_label_name(arena, label_counter_ptr);
    const TacOperand false_exit_label = create_tac_operand_label(false_exit_label_name);
    const char *end_label_name = generate_label_name(arena, label_counter_ptr);
    const TacOperand end_label = create_tac_operand_label(end_label_name);

    // 4. if_false lhs_result goto L_false_exit
    const TacInstruction *if_false_instr = create_tac_instruction_if_false_goto(lhs_result, false_exit_label, arena);
    add_instruction_to_function(current_function, if_false_instr, arena);

    // 5. Evaluate RHS (only if LHS was true)
    const TacOperand rhs_result = visit_expression(binary_node->right, current_function, arena, next_temp_id_ptr,
                                                   label_counter_ptr);
    if (!is_valid_operand(rhs_result)) {
        fprintf(stderr, "Error: LOGICAL_AND's right operand did not yield a valid result.\n");
        // Still need to emit the labels for correct control flow even on error
        const TacInstruction *define_false_label_instr = create_tac_instruction_label(false_exit_label, arena);
        add_instruction_to_function(current_function, define_false_label_instr, arena);
        const TacInstruction *assign_false_instr = create_tac_instruction_copy(
            dest_temp, create_tac_operand_const(0), arena);
        add_instruction_to_function(current_function, assign_false_instr, arena);
        const TacInstruction *define_end_label_instr = create_tac_instruction_label(end_label, arena);
        add_instruction_to_function(current_function, define_end_label_instr, arena);
        return create_invalid_operand();
    }
    // 6. dest_temp = rhs_result (if RHS is evaluated, its result is the result of the AND)
    //    However, logical AND should result in 0 or 1. So if rhs_result is non-zero, dest_temp is 1, else 0.
    const TacInstruction *assign_rhs_bool_instr = create_tac_instruction_not_equal(
        dest_temp, rhs_result, create_tac_operand_const(0), arena);
    add_instruction_to_function(current_function, assign_rhs_bool_instr, arena);

    // 7. goto L_end
    const TacInstruction *goto_end_instr = create_tac_instruction_goto(end_label, arena);
    add_instruction_to_function(current_function, goto_end_instr, arena);

    // 8. L_false_exit:
    const TacInstruction *false_label_def_instr = create_tac_instruction_label(false_exit_label, arena);
    add_instruction_to_function(current_function, false_label_def_instr, arena);

    // 9. dest_temp = 0 (false)
    const TacInstruction *assign_false_val_instr = create_tac_instruction_copy(
        dest_temp, create_tac_operand_const(0), arena);
    add_instruction_to_function(current_function, assign_false_val_instr, arena);

    // 10. L_end:
    const TacInstruction *end_label_def_instr = create_tac_instruction_label(end_label, arena);
    add_instruction_to_function(current_function, end_label_def_instr, arena);

    // 11. The result of this expression is dest_temp
    return dest_temp;
}

// Helper function to handle LOGICAL_OR operations (short-circuiting)
// Generates TAC for an expression like `t_dest = t_lhs || t_rhs`.
// The exact temporary variable names (e.g., t0, t1) and label names (e.g., L0, L1, L2)
// will be determined by the current state of `next_temp_id_ptr` and `label_counter_ptr`.
// Assumes `true_exit_label` is L_i+1 and `end_label` is L_i+2 (if an implicit `eval_rhs_label` was L_i).
//
// Example TAC sequence:
//   ; ... Instructions to evaluate LHS into t_lhs ...
//   IF_TRUE t_lhs, L_i+1         ; if LHS is true, jump to set result to 1
//   ; ... Instructions to evaluate RHS into t_rhs (only if LHS was false) ...
//   t_dest = t_rhs != const 0  ; dest_temp = (rhs_result != 0)
//   GOTO L_i+2                   ; jump to the end
// L_i+1:                          ; true_exit_label: LHS was true
//   t_dest = const 1             ; result is 1
// L_i+2:                          ; end_label: common exit point
//   ; t_dest now holds the boolean result (0 or 1) of the OR operation
static TacOperand visit_logical_or_expression(const BinaryOpNode *binary_node, TacFunction *current_function,
                                              Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr) {
    // LHS || RHS
    // 1. Allocate a temporary for the result of the OR expression.
    const TacOperand dest_temp = create_tac_operand_temp((*next_temp_id_ptr)++, "t");

    // 2. Create labels
    const char *eval_rhs_label_name = generate_label_name(arena, label_counter_ptr);
    const TacOperand eval_rhs_label = create_tac_operand_label(eval_rhs_label_name);
    const char *true_exit_label_name = generate_label_name(arena, label_counter_ptr);
    const TacOperand true_exit_label = create_tac_operand_label(true_exit_label_name);
    const char *end_label_name = generate_label_name(arena, label_counter_ptr);
    const TacOperand end_label = create_tac_operand_label(end_label_name);

    // 3. if_true lhs_result goto L_true_exit (If LHS is true, jump to assign true and exit)
    const TacOperand lhs_result = visit_expression(binary_node->left, current_function, arena, next_temp_id_ptr,
                                                   label_counter_ptr);
    if (!is_valid_operand(lhs_result)) {
        fprintf(stderr, "Error: LOGICAL_OR's left operand did not yield a valid result.\n");
        return create_invalid_operand();
    }
    const TacInstruction *if_true_instr = create_tac_instruction_if_true_goto(lhs_result, true_exit_label, arena);
    add_instruction_to_function(current_function, if_true_instr, arena);

    // 4. LHS was false. Evaluate RHS. (This is the path if not short-circuited by LHS being true)
    const TacOperand rhs_result = visit_expression(binary_node->right, current_function, arena, next_temp_id_ptr,
                                                   label_counter_ptr);
    if (!is_valid_operand(rhs_result)) {
        fprintf(stderr, "Error: LOGICAL_OR's right operand did not yield a valid result.\n");
        // Still need to emit labels for correct control flow even on error
        const TacInstruction *define_true_label_instr = create_tac_instruction_label(true_exit_label, arena);
        add_instruction_to_function(current_function, define_true_label_instr, arena);
        const TacInstruction *assign_true_instr_on_err = create_tac_instruction_copy(
            dest_temp, create_tac_operand_const(1), arena);
        add_instruction_to_function(current_function, assign_true_instr_on_err, arena);
        // We also need the eval_rhs label and end_label for consistent paths.
        const TacInstruction *define_eval_rhs_label_instr = create_tac_instruction_label(eval_rhs_label, arena);
        add_instruction_to_function(current_function, define_eval_rhs_label_instr, arena);
        const TacInstruction *define_end_label_instr = create_tac_instruction_label(end_label, arena);
        add_instruction_to_function(current_function, define_end_label_instr, arena);
        return create_invalid_operand();
    }
    // 5. dest_temp = (rhs_result != 0) (Result is true if RHS is non-zero, false otherwise)
    const TacInstruction *assign_rhs_bool_instr = create_tac_instruction_not_equal(
        dest_temp, rhs_result, create_tac_operand_const(0), arena);
    add_instruction_to_function(current_function, assign_rhs_bool_instr, arena);
    //    goto L_end
    TacInstruction *goto_end_instr_from_rhs = create_tac_instruction_goto(end_label, arena);
    add_instruction_to_function(current_function, goto_end_instr_from_rhs, arena);

    // 6. L_true_exit: (LHS was true, or preparing path for it)
    const TacInstruction *true_label_def_instr = create_tac_instruction_label(true_exit_label, arena);
    add_instruction_to_function(current_function, true_label_def_instr, arena);

    // 7. dest_temp = 1 (true)
    const TacInstruction *assign_true_val_instr = create_tac_instruction_copy(
        dest_temp, create_tac_operand_const(1), arena);
    add_instruction_to_function(current_function, assign_true_val_instr, arena);
    // No need for goto L_end here, as L_end will follow or this path is taken via goto.

    // 8. L_end: (Path from RHS evaluation also jumps here)
    const TacInstruction *end_label_def_instr = create_tac_instruction_label(end_label, arena);
    add_instruction_to_function(current_function, end_label_def_instr, arena);

    // Note: The eval_rhs_label is implicitly handled by the control flow where RHS is evaluated if LHS is false.
    // The explicit definition and jump to eval_rhs_label_name from original code might be slightly different. Reviewing that. 
    // The original OR was: if_false LSH goto eval_RHS; LHS_true_path: assign 1, goto end; eval_RHS_label: evaluate RHS, assign bool(RHS), end_label.
    // Current is: if_true LHS goto true_exit; evaluate RHS, assign bool(RHS), goto end; true_exit_label: assign 1; end_label.
    // These are logically equivalent for correct short-circuiting. The key is that RHS is skipped if LHS meets OR's condition.

    // 9. The result of this expression is dest_temp
    return dest_temp;
}

// Helper function to handle binary operations
static TacOperand visit_binary_op_expression(const BinaryOpNode *binary_node, TacFunction *current_function,
                                             Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr) {
    // Handle LOGICAL_AND and LOGICAL_OR separately due to short-circuiting
    if (binary_node->op == OPERATOR_LOGICAL_AND) {
        return visit_logical_and_expression(binary_node, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    }
    if (binary_node->op == OPERATOR_LOGICAL_OR) {
        return visit_logical_or_expression(binary_node, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    }
    // Standard binary operations (non-short-circuiting)
    // 1. Visit left and right operands
    const TacOperand left_operand = visit_expression(binary_node->left, current_function, arena, next_temp_id_ptr,
                                                     label_counter_ptr);
    if (!is_valid_operand(left_operand)) {
        fprintf(stderr, "Error: Binary operator's left operand did not yield a valid result.\n");
        return create_invalid_operand();
    }

    const TacOperand right_operand = visit_expression(binary_node->right, current_function, arena, next_temp_id_ptr,
                                                      label_counter_ptr);
    if (!is_valid_operand(right_operand)) {
        fprintf(stderr, "Error: Binary operator's right operand did not yield a valid result.\n");
        return create_invalid_operand();
    }

    // 2. Create a new temporary to store the result
    const TacOperand dest_temp = create_tac_operand_temp((*next_temp_id_ptr)++, "t");

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
static TacOperand visit_expression(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr,
                                   int *label_counter_ptr) {
    if (!node) return create_invalid_operand();

    switch (node->type) {
        case NODE_INT_LITERAL: {
            const IntLiteralNode *int_node = (IntLiteralNode *) node;
            // Integer literals directly translate to constant operands
            return create_tac_operand_const(int_node->value);
        }
        case NODE_IDENTIFIER: { 
            const IdentifierNode *id_node = (IdentifierNode *) node;
            // This is for r-value usage of an identifier.
            // A proper implementation would look up id_node->name in a symbol table.
            return lookup_variable_operand(id_node->name, current_function, arena, next_temp_id_ptr);
        }
        case NODE_ASSIGNMENT_EXP: { 
            AssignmentExpNode *assign_node = (AssignmentExpNode *) node;
            return visit_assignment_expression(assign_node, current_function, arena, next_temp_id_ptr, label_counter_ptr);
        }
        case NODE_UNARY_OP: {
            const UnaryOpNode *unary_node = (UnaryOpNode *) node;
            return visit_unary_op_expression(unary_node, current_function, arena, next_temp_id_ptr, label_counter_ptr);
        }
        case NODE_BINARY_OP: {
            const BinaryOpNode *binary_node = (BinaryOpNode *) node;
            return visit_binary_op_expression(binary_node, current_function, arena, next_temp_id_ptr,
                                              label_counter_ptr);
        }
        // Add cases for other expressions (FunctionCall, etc.)
        default:
            fprintf(stderr, "Error: Unexpected AST node type %d in visit_expression. File: %s, Line: %d\n", node->type, __FILE__, __LINE__);
            return create_invalid_operand();
    }
}
