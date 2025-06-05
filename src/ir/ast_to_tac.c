#include "ast_to_tac.h"

#include <stdio.h>  // For error reporting (potentially)
#include <stdlib.h> // For NULL
#include <stdbool.h>// For bool type
#include <string.h> // For strcmp in some TAC printing/debugging if added

// --- Static Helper Function Declarations ---

// Forward declaration for visiting statements (like ReturnStmt)
static void visit_statement(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr,
                            int *label_counter_ptr);

// Forward declaration for visiting expressions (which produce a value)
static TacOperand visit_expression(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr,
                                   int *label_counter_ptr, int target_temp_id_hint);

// Forward declaration for visiting lvalue expressions (like identifiers on the LHS of an assignment)
static TacOperand visit_lvalue_expression(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr,
                                          int *label_counter_ptr);

static TacOperand visit_unary_op_expression(UnaryOpNode *unary_node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr, int target_temp_id_hint);
static TacOperand visit_binary_op_expression(BinaryOpNode *binary_node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr, int target_temp_id_hint);
static TacOperand visit_logical_op_expression(BinaryOpNode *logical_node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr, int target_temp_id_hint);
static TacOperand visit_assignment_expression(AssignmentExpNode *assign_node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr, int *label_counter_ptr);

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
TacOperand create_invalid_operand() {
    TacOperand op;
    op.type = TAC_OPERAND_NONE;
    return op;
}

// Helper to check if an operand is valid
bool is_valid_operand(TacOperand op) {
    return op.type != TAC_OPERAND_NONE;
}

// Helper function to find the maximum pre-assigned tac_temp_id in VarDeclNodes
// within an AST node and its children.
static int find_max_preassigned_temp_id_recursive(AstNode *node, int current_max) {
    if (!node) {
        return current_max;
    }

    int max_here = current_max;

    switch (node->type) {
        case NODE_VAR_DECL: {
            VarDeclNode *vd_node = (VarDeclNode *)node;
            if (vd_node->tac_temp_id > max_here) {
                max_here = vd_node->tac_temp_id;
            }
            // Recursively check the initializer expression
            if (vd_node->initializer) {
                max_here = find_max_preassigned_temp_id_recursive(vd_node->initializer, max_here);
            }
            break;
        }
        case NODE_BLOCK: {
            BlockNode *block_node = (BlockNode *)node;
            for (size_t i = 0; i < block_node->num_items; ++i) { 
                max_here = find_max_preassigned_temp_id_recursive(block_node->items[i], max_here);
            }
            break;
        }
        case NODE_FUNC_DEF: { // Should be called on the body
            FuncDefNode *fn_node = (FuncDefNode *)node;
            max_here = find_max_preassigned_temp_id_recursive((AstNode*)fn_node->body, max_here); 
            break;
        }
        case NODE_RETURN_STMT: {
            ReturnStmtNode *ret_node = (ReturnStmtNode *)node;
            max_here = find_max_preassigned_temp_id_recursive(ret_node->expression, max_here);
            break;
        }
        case NODE_UNARY_OP: {
            UnaryOpNode *un_node = (UnaryOpNode *)node;
            max_here = find_max_preassigned_temp_id_recursive(un_node->operand, max_here);
            break;
        }
        case NODE_BINARY_OP: {
            BinaryOpNode *bin_node = (BinaryOpNode *)node;
            max_here = find_max_preassigned_temp_id_recursive(bin_node->left, max_here);
            max_here = find_max_preassigned_temp_id_recursive(bin_node->right, max_here);
            break;
        }
        case NODE_ASSIGNMENT_EXP: {
            AssignmentExpNode *assign_node = (AssignmentExpNode *)node;
            max_here = find_max_preassigned_temp_id_recursive(assign_node->target, max_here);
            max_here = find_max_preassigned_temp_id_recursive(assign_node->value, max_here);
            break;
        }
        default:
            break;
    }
    return max_here;
}

// --- Main Translation Function ---

TacProgram* ast_to_tac(ProgramNode *ast_root, Arena *arena) { 
    if (!ast_root) {
        fprintf(stderr, "Error: AST root is NULL in ast_to_tac.\n");
        return NULL;
    }

    // Allocate TacProgram from the arena
    TacProgram *tac_program = arena_alloc(arena, sizeof(TacProgram));
    if (!tac_program) {
        fprintf(stderr, "Error: Failed to allocate TacProgram from arena.\n");
        return NULL;
    }
    tac_program->functions = arena_alloc(arena, sizeof(TacFunction *) * 1); 
    tac_program->function_count = 0;

    // Current cleric supports only one function definition in ProgramNode
    if (!ast_root->function) {
        fprintf(stderr, "Warning: ProgramNode does not contain a function definition.\n");
        // Return the empty (but allocated) program
        return tac_program;
    }
    FuncDefNode *func_node = ast_root->function;

    TacFunction *current_function = create_tac_function(func_node->name, arena);
    if (!current_function) {
        fprintf(stderr, "Error: Failed to create TAC function for '%s'.\n", func_node->name);
        return tac_program; 
    }

    int max_preassigned_id = -1;
    if (func_node->body) {
        max_preassigned_id = find_max_preassigned_temp_id_recursive((AstNode*)func_node->body, -1); 
    }

    int next_temp_id = (max_preassigned_id == -1) ? 0 : max_preassigned_id + 1;
    int label_counter = 0;

    if (func_node->body) {
        visit_statement((AstNode*)func_node->body, current_function, arena, &next_temp_id, &label_counter); 
    }

    add_function_to_program(tac_program, current_function, arena);
    return tac_program;
}

// --- Static Helper Function Implementations ---

// Visitor for statement nodes
static void visit_statement(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr,
                            int *label_counter_ptr) {
    if (!node) return;

    switch (node->type) {
        case NODE_RETURN_STMT: {
            ReturnStmtNode *ret_node = (ReturnStmtNode *) node;
            TacOperand result_operand = visit_expression(ret_node->expression, current_function, arena, next_temp_id_ptr, label_counter_ptr, -1); // No hint for return value's computation

            if (!is_valid_operand(result_operand)) {
                fprintf(stderr, "Error: Return statement's expression did not yield a valid result operand.\n");
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
            BlockNode *block_node = (BlockNode *)node;
            for (size_t i = 0; i < block_node->num_items; ++i) { 
                visit_statement(block_node->items[i], current_function, arena, next_temp_id_ptr, label_counter_ptr);
            }
            break;
        }
        case NODE_VAR_DECL: {
            VarDeclNode *var_decl_node = (VarDeclNode *) node;
            // The variable is now "active" in TAC. Its temp_id should have been assigned by the validator.
            // If there's an initializer, generate TAC to assign its value to the variable's temporary.
            if (var_decl_node->tac_temp_id < 0) { 
                 fprintf(stderr, "Error: Variable Declaration '%s' (in visit_statement) does not have a valid TAC temporary ID (id: %d). Validator issue?\n", var_decl_node->var_name, var_decl_node->tac_temp_id);
                 break; 
            }

            if (var_decl_node->initializer) {
                // Pass the variable's own temp_id as a hint to the expression visitor
                TacOperand rhs_operand = visit_expression(var_decl_node->initializer, current_function, arena, next_temp_id_ptr, label_counter_ptr, var_decl_node->tac_temp_id);

                if (!is_valid_operand(rhs_operand)) {
                    fprintf(stderr, "Error: Variable declaration for '%s' has an initializer that did not yield a valid TAC operand.\n", var_decl_node->var_name);
                    break; 
                }

                // If the rhs_operand is NOT the same as the variable's temporary,
                // it means the expression visitor couldn't directly use the hint
                // (e.g., it was a literal or a simple identifier that doesn't generate an instruction itself).
                // In this case, we still need a COPY.
                if (!(rhs_operand.type == TAC_OPERAND_TEMP && rhs_operand.value.temp.id == var_decl_node->tac_temp_id)) {
                    TacOperand lhs_operand = create_tac_operand_temp(var_decl_node->tac_temp_id, var_decl_node->var_name);
                    TacInstruction *copy_instr = create_tac_instruction_copy(lhs_operand, rhs_operand, arena);
                    if (copy_instr) {
                        add_instruction_to_function(current_function, copy_instr, arena);
                    } else {
                        fprintf(stderr, "Error: Failed to create TAC_INS_COPY instruction for variable initialization of '%s'.\n", var_decl_node->var_name);
                    }
                }
            }
            break;
        }
        default:
            fprintf(stderr, "Error: Unexpected AST node type %d in visit_statement.\n", node->type);
            break;
    }
}

// Visitor for expression nodes
// Returns the operand where the result of the expression is stored.
// target_temp_id_hint: If not -1, the visitor will attempt to store the result
// directly into this temporary ID. Otherwise, a new temporary may be allocated.
static TacOperand visit_expression(AstNode *node, TacFunction *current_function, Arena *arena, int *next_temp_id_ptr,
                                   int *label_counter_ptr, int target_temp_id_hint) {
    if (!node) {
        fprintf(stderr, "Error: Encountered NULL AST node in visit_expression.\n");
        return create_invalid_operand();
    }

    switch (node->type) {
        case NODE_INT_LITERAL: {
            IntLiteralNode *lit_node = (IntLiteralNode *) node;
            return create_tac_operand_const(lit_node->value);
        }
        case NODE_IDENTIFIER: {
            IdentifierNode *id_node = (IdentifierNode *) node;
            // Assuming validator has populated tac_temp_id and it's an rvalue context. Hint not used here.
            if (id_node->tac_temp_id < 0) {
                fprintf(stderr, "Error: Identifier '%s' (in visit_expression) does not have a valid TAC temporary ID. Validator issue or undeclared var?\n", id_node->name);
                return create_invalid_operand();
            }
            return create_tac_operand_temp(id_node->tac_temp_id, id_node->name);
        }
        case NODE_UNARY_OP:
            return visit_unary_op_expression((UnaryOpNode *)node, current_function, arena, next_temp_id_ptr, label_counter_ptr, target_temp_id_hint);
        case NODE_BINARY_OP: {
            BinaryOpNode *bin_node = (BinaryOpNode *)node;
            // Check if it's a logical operator, which has special handling for short-circuiting
            if (bin_node->op == OPERATOR_LOGICAL_AND || bin_node->op == OPERATOR_LOGICAL_OR) {
                return visit_logical_op_expression(bin_node, current_function, arena, next_temp_id_ptr, label_counter_ptr, target_temp_id_hint);
            } else {
                return visit_binary_op_expression(bin_node, current_function, arena, next_temp_id_ptr, label_counter_ptr, target_temp_id_hint);
            }
        }
        case NODE_ASSIGNMENT_EXP: // Assignment expressions return the assigned value
            // Assignment has its own target; hint is not for the assignment op itself but for RHS if it were complex.
            // However, standard assignment `a = b` where `b` is an expression, `b` computes into its own temp first or uses hint if `a` was a var_decl.
            // For `a = b`, the result of assignment is `a`. We pass -1 as hint for RHS of assignment here.
            return visit_assignment_expression((AssignmentExpNode *)node, current_function, arena, next_temp_id_ptr, label_counter_ptr); 
        default:
            fprintf(stderr, "Error: Unexpected AST node type %d in visit_expression.\n", node->type);
            return create_invalid_operand();
    }
}

// Visitor for unary operator expressions
static TacOperand visit_unary_op_expression(UnaryOpNode *unary_node, TacFunction *current_function, Arena *arena,
                                            int *next_temp_id_ptr, int *label_counter_ptr, int target_temp_id_hint) {
    // 1. Visit the operand expression
    TacOperand operand = visit_expression(unary_node->operand, current_function, arena, next_temp_id_ptr, label_counter_ptr, -1); // Operand computes into its own temp
    if (!is_valid_operand(operand)) {
        fprintf(stderr, "Error: Operand for unary operator %d did not yield a valid TAC operand.\n", unary_node->op);
        return create_invalid_operand();
    }

    // 2. Determine the TAC instruction type from the unary operator
    TacInstructionType tac_op_type; 
    switch (unary_node->op) {
        case OPERATOR_NEGATE:       tac_op_type = TAC_INS_NEGATE; break;
        case OPERATOR_LOGICAL_NOT:  tac_op_type = TAC_INS_LOGICAL_NOT; break;
        // Add other unary operators if any, e.g., bitwise NOT
        default: fprintf(stderr, "Error: Unsupported unary operator %d.\n", unary_node->op); return create_invalid_operand();
    }

    // 3. Create destination operand: use hint or new temporary
    TacOperand dst_operand;
    if (target_temp_id_hint != -1) {
        dst_operand = create_tac_operand_temp(target_temp_id_hint, "un_op_hinted_dst");
    } else {
        dst_operand = create_tac_operand_temp((*next_temp_id_ptr)++, "un_op_res");
    }

    // 4. Create and add the TAC instruction
    TacInstruction *unary_instr = NULL;
    switch (tac_op_type) {
        case TAC_INS_NEGATE:
            unary_instr = create_tac_instruction_negate(dst_operand, operand, arena);
            break;
        case TAC_INS_LOGICAL_NOT:
            unary_instr = create_tac_instruction_logical_not(dst_operand, operand, arena);
            break;
        // Add other unary ops if they share the same operand structure
        default:
            fprintf(stderr, "Error: Unhandled TAC instruction type %d in visit_unary_op_expression's instruction creation.\n", tac_op_type);
            return create_invalid_operand();
    }

    if (unary_instr) {
        add_instruction_to_function(current_function, unary_instr, arena);
    } else {
        // This case might not be reached if create_tac_instruction_* functions exit on failure
        fprintf(stderr, "Error: Failed to create TAC instruction for unary operator %d.\n", unary_node->op);
        return create_invalid_operand();
    }

    // 5. The result of this expression is the destination temporary
    return dst_operand;
}

// Visitor for binary operator expressions (arithmetic, relational)
static TacOperand visit_binary_op_expression(BinaryOpNode *binary_node, TacFunction *current_function, Arena *arena,
                                            int *next_temp_id_ptr, int *label_counter_ptr, int target_temp_id_hint) {
    // 1. Visit the left and right operand expressions
    // Operands compute into their own temps first, hint is for the result of *this* binary op.
    TacOperand lhs_operand = visit_expression(binary_node->left, current_function, arena, next_temp_id_ptr, label_counter_ptr, -1);
    TacOperand rhs_operand = visit_expression(binary_node->right, current_function, arena, next_temp_id_ptr, label_counter_ptr, -1);

    if (!is_valid_operand(lhs_operand) || !is_valid_operand(rhs_operand)) {
        fprintf(stderr, "Error: One or both operands for binary operator %d did not yield valid TAC operands.\n", binary_node->op);
        return create_invalid_operand();
    }

    // 2. Determine the TAC instruction type
    TacInstructionType tac_op_type;
    switch (binary_node->op) {
        case OPERATOR_ADD: tac_op_type = TAC_INS_ADD; break;
        case OPERATOR_SUBTRACT: tac_op_type = TAC_INS_SUB; break;
        case OPERATOR_MULTIPLY: tac_op_type = TAC_INS_MUL; break;
        case OPERATOR_DIVIDE: tac_op_type = TAC_INS_DIV; break;
        case OPERATOR_MODULO: tac_op_type = TAC_INS_MOD; break;
        case OPERATOR_LESS: tac_op_type = TAC_INS_LESS; break;
        case OPERATOR_GREATER: tac_op_type = TAC_INS_GREATER; break;
        case OPERATOR_LESS_EQUAL: tac_op_type = TAC_INS_LESS_EQUAL; break;
        case OPERATOR_GREATER_EQUAL: tac_op_type = TAC_INS_GREATER_EQUAL; break;
        case OPERATOR_EQUAL_EQUAL: tac_op_type = TAC_INS_EQUAL; break;
        case OPERATOR_NOT_EQUAL: tac_op_type = TAC_INS_NOT_EQUAL; break;
        // Logical AND/OR are handled by visit_logical_op_expression
        default: fprintf(stderr, "Error: Unsupported binary operator type %d (or not yet handled).\n", binary_node->op);
            return create_invalid_operand();
    }

    // 3. Create destination operand: use hint or new temporary
    TacOperand dst_operand;
    if (target_temp_id_hint != -1) {
        dst_operand = create_tac_operand_temp(target_temp_id_hint, "bin_op_hinted_dst"); // TODO: Better name hint using var_name if available
    } else {
        dst_operand = create_tac_operand_temp((*next_temp_id_ptr)++, "bin_op_res");
    }

    // 4. Create and add the TAC instruction
    TacInstruction *binary_instr = NULL;
    switch (tac_op_type) {
        case TAC_INS_ADD:
            binary_instr = create_tac_instruction_add(dst_operand, lhs_operand, rhs_operand, arena);
            break;
        case TAC_INS_SUB:
            binary_instr = create_tac_instruction_sub(dst_operand, lhs_operand, rhs_operand, arena);
            break;
        case TAC_INS_MUL:
            binary_instr = create_tac_instruction_mul(dst_operand, lhs_operand, rhs_operand, arena);
            break;
        case TAC_INS_DIV:
            binary_instr = create_tac_instruction_div(dst_operand, lhs_operand, rhs_operand, arena);
            break;
        case TAC_INS_MOD:
            binary_instr = create_tac_instruction_mod(dst_operand, lhs_operand, rhs_operand, arena);
            break;
        case TAC_INS_LESS:
            binary_instr = create_tac_instruction_less(dst_operand, lhs_operand, rhs_operand, arena);
            break;
        case TAC_INS_GREATER:
            binary_instr = create_tac_instruction_greater(dst_operand, lhs_operand, rhs_operand, arena);
            break;
        case TAC_INS_LESS_EQUAL:
            binary_instr = create_tac_instruction_less_equal(dst_operand, lhs_operand, rhs_operand, arena);
            break;
        case TAC_INS_GREATER_EQUAL:
            binary_instr = create_tac_instruction_greater_equal(dst_operand, lhs_operand, rhs_operand, arena);
            break;
        case TAC_INS_EQUAL:
            binary_instr = create_tac_instruction_equal(dst_operand, lhs_operand, rhs_operand, arena);
            break;
        case TAC_INS_NOT_EQUAL:
            binary_instr = create_tac_instruction_not_equal(dst_operand, lhs_operand, rhs_operand, arena);
            break;
        // Logical AND/OR are handled by visit_logical_op_expression
        default:
            fprintf(stderr, "Error: Unhandled TAC instruction type %d in visit_binary_op_expression's instruction creation.\n", tac_op_type);
            return create_invalid_operand();
    }

    if (binary_instr) {
        add_instruction_to_function(current_function, binary_instr, arena);
    } else {
        // This case might not be reached if create_tac_instruction_* functions exit on failure
        fprintf(stderr, "Error: Failed to create TAC instruction for binary operator %d.\n", binary_node->op);
        return create_invalid_operand();
    }

    // 5. The result of this expression is the destination temporary
    return dst_operand;
}

// Visitor for logical AND/OR expressions (handles short-circuiting)
static TacOperand visit_logical_op_expression(BinaryOpNode *logical_node, TacFunction *current_function, Arena *arena,
                                            int *next_temp_id_ptr, int *label_counter_ptr, int target_temp_id_hint) {
    TacOperand dst_operand;
    if (target_temp_id_hint != -1) {
        dst_operand = create_tac_operand_temp(target_temp_id_hint, "log_op_hinted_dst");
    } else {
        dst_operand = create_tac_operand_temp((*next_temp_id_ptr)++, "logical_res");
    }

    if (logical_node->op == OPERATOR_LOGICAL_OR) {
        int true_label_id = (*label_counter_ptr)++;
        int end_label_id = (*label_counter_ptr)++;
        const char* true_label_name = generate_label_name(arena, &true_label_id); // Use ID directly for name generation
        const char* end_label_name = generate_label_name(arena, &end_label_id);
        TacOperand true_label_op = create_tac_operand_label(true_label_name);
        TacOperand end_label_op = create_tac_operand_label(end_label_name);

        // IF_TRUE lhs_operand GOTO true_label
        TacOperand lhs_operand = visit_expression(logical_node->left, current_function, arena, next_temp_id_ptr, label_counter_ptr, -1);
        if (!is_valid_operand(lhs_operand)) {
            fprintf(stderr, "Error: LHS of logical operator did not yield a valid TAC operand.\n");
            return create_invalid_operand();
        }
        add_instruction_to_function(current_function, create_tac_instruction_if_true_goto(lhs_operand, true_label_op, arena), arena);

        // LHS is false, evaluate RHS
        TacOperand rhs_operand = visit_expression(logical_node->right, current_function, arena, next_temp_id_ptr, label_counter_ptr, -1);
        if (!is_valid_operand(rhs_operand)) { /* Error handled by visit_expression */ return create_invalid_operand(); }
        // dst_operand = (rhs_operand != 0)
        add_instruction_to_function(current_function, create_tac_instruction_not_equal(dst_operand, rhs_operand, create_tac_operand_const(0), arena), arena);
        // GOTO end_label
        add_instruction_to_function(current_function, create_tac_instruction_goto(end_label_op, arena), arena);

        // true_label: (LHS was true)
        add_instruction_to_function(current_function, create_tac_instruction_label(true_label_op, arena), arena);
        // dst_operand = 1
        add_instruction_to_function(current_function, create_tac_instruction_copy(dst_operand, create_tac_operand_const(1), arena), arena);

        // end_label:
        add_instruction_to_function(current_function, create_tac_instruction_label(end_label_op, arena), arena);

    } else if (logical_node->op == OPERATOR_LOGICAL_AND) {
        int false_label_id = (*label_counter_ptr)++;
        int end_label_id = (*label_counter_ptr)++;
        const char* false_label_name = generate_label_name(arena, &false_label_id);
        const char* end_label_name = generate_label_name(arena, &end_label_id);
        TacOperand false_label_op = create_tac_operand_label(false_label_name);
        TacOperand end_label_op = create_tac_operand_label(end_label_name);

        // IF_FALSE lhs_operand GOTO false_label
        TacOperand lhs_operand = visit_expression(logical_node->left, current_function, arena, next_temp_id_ptr, label_counter_ptr, -1);
        if (!is_valid_operand(lhs_operand)) {
            fprintf(stderr, "Error: LHS of logical operator did not yield a valid TAC operand.\n");
            return create_invalid_operand();
        }
        add_instruction_to_function(current_function, create_tac_instruction_if_false_goto(lhs_operand, false_label_op, arena), arena);

        // LHS is true, evaluate RHS
        TacOperand rhs_operand = visit_expression(logical_node->right, current_function, arena, next_temp_id_ptr, label_counter_ptr, -1);
        if (!is_valid_operand(rhs_operand)) { /* Error handled by visit_expression */ return create_invalid_operand(); }
        // dst_operand = (rhs_operand != 0)
        add_instruction_to_function(current_function, create_tac_instruction_not_equal(dst_operand, rhs_operand, create_tac_operand_const(0), arena), arena);
        // GOTO end_label
        add_instruction_to_function(current_function, create_tac_instruction_goto(end_label_op, arena), arena);

        // false_label: (LHS was false)
        add_instruction_to_function(current_function, create_tac_instruction_label(false_label_op, arena), arena);
        // dst_operand = 0
        add_instruction_to_function(current_function, create_tac_instruction_copy(dst_operand, create_tac_operand_const(0), arena), arena);

        // end_label:
        add_instruction_to_function(current_function, create_tac_instruction_label(end_label_op, arena), arena);
    } else {
        fprintf(stderr, "Error: Unexpected operator type %d in visit_logical_op_expression.\n", logical_node->op);
        return create_invalid_operand();
    }

    // The result of this expression is dst_operand
    return dst_operand;
}

// Visitor for assignment expressions
static TacOperand visit_assignment_expression(AssignmentExpNode *assign_node, TacFunction *current_function, Arena *arena,
                                            int *next_temp_id_ptr, int *label_counter_ptr) {
    // 1. Evaluate the target lvalue (e.g., get the temp_id for a variable)
    TacOperand target_operand = visit_lvalue_expression(assign_node->target, current_function, arena, next_temp_id_ptr, label_counter_ptr);
    if (!is_valid_operand(target_operand)) {
        fprintf(stderr, "Error: Assignment expression's left-hand side is not a valid l-value.\n");
        return create_invalid_operand();
    }

    // 2. Evaluate the right-hand side (the value to be assigned)
    TacOperand rhs_operand = visit_expression(assign_node->value, current_function, arena, next_temp_id_ptr, label_counter_ptr, -1); // RHS computes into its own temporary, or uses hint if the assignment is part of a VarDecl.

    if (!is_valid_operand(rhs_operand)) {
        fprintf(stderr, "Error: Assignment expression's right-hand side did not yield a valid result.\n");
        return create_invalid_operand();
    }

    // 3. Create the TAC_INS_COPY instruction (target = value)
    const TacInstruction *copy_instr = create_tac_instruction_copy(target_operand, rhs_operand, arena);
    if (!copy_instr) {
        fprintf(stderr, "Error: Failed to create TAC instruction for assignment.\n");
        return create_invalid_operand();
    }
    add_instruction_to_function(current_function, copy_instr, arena);

    // 4. The result of an assignment expression is the value assigned (rhs_operand)
    return rhs_operand;
}

// Visitor for lvalue expressions (e.g., identifiers on the LHS of an assignment)
// Returns an operand representing the lvalue (typically a temporary variable).
static TacOperand visit_lvalue_expression(AstNode *node, TacFunction *current_function, Arena *arena,
                                        int *next_temp_id_ptr, int *label_counter_ptr) { // No target_temp_id_hint for lvalue itself
    if (!node) {
        fprintf(stderr, "Error: Encountered NULL AST node in visit_lvalue_expression.\n");
        return create_invalid_operand();
    }

    switch (node->type) {
        case NODE_IDENTIFIER: {
            const IdentifierNode *id_node = (IdentifierNode *) node;
            // Validator should have populated tac_temp_id and tac_name_hint.
            // Ensure tac_temp_id is valid (e.g., not negative).
            if (id_node->tac_temp_id < 0) { 
                fprintf(stderr, "Error: L-value Identifier '%s' (in visit_lvalue_expression) does not have a valid TAC temporary ID (id: %d). Validator issue?\n", id_node->name, id_node->tac_temp_id);
                return create_invalid_operand();
            }
            return create_tac_operand_temp(id_node->tac_temp_id, id_node->tac_name_hint);
        }
        // Add cases for other l-value expressions like array indexing, dereferences, etc.
        default:
            fprintf(stderr, "Error: Unexpected AST node type %d for l-value in visit_lvalue_expression.\n", node->type);
            return create_invalid_operand();
    }
}
