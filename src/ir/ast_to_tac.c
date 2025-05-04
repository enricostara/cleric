#include "ast_to_tac.h"

#include <stdio.h>  // For error reporting (potentially)
#include <stdlib.h> // For NULL
#include <stdbool.h>// For bool type

// --- Static Helper Function Declarations ---

// Forward declaration for visiting statements (like ReturnStmt)
static void visit_statement(AstNode* node, TacFunction* current_function, Arena* arena, int* next_temp_id_ptr);

// Forward declaration for visiting expressions (which produce a value)
static TacOperand visit_expression(AstNode* node, TacFunction* current_function, Arena* arena, int* next_temp_id_ptr);

// Helper to create an invalid operand (useful for nodes that don't return a value)
static TacOperand create_invalid_operand() {
    return (TacOperand){ .type = -1 }; // Use an invalid type marker
}

// Helper to check if an operand is valid
static bool is_valid_operand(TacOperand op) {
    return op.type != -1;
}

// --- Main Translation Function ---

TacProgram* ast_to_tac(ProgramNode* ast_root, Arena* arena) {
    if (!ast_root || ast_root->base.type != NODE_PROGRAM) {
        fprintf(stderr, "Error: Invalid AST root node for TAC generation.\n");
        return NULL;
    }

    TacProgram* tac_program = create_tac_program(arena);
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

    const FuncDefNode* func_def = ast_root->function;

    // Create the TacFunction corresponding to the FuncDefNode
    TacFunction* tac_function = create_tac_function(func_def->name, arena);
    if (!tac_function) {
        fprintf(stderr, "Error: Failed to create TAC function for '%s'.\n", func_def->name);
        return NULL;
    }

    // Add the function to the program
    add_function_to_program(tac_program, tac_function, arena);

    // Visit the function body (statements)
    // Initialize the temporary ID counter for this function
    int next_temp_id = 0;
    // For now, assume the body is a single statement (like the ReturnStmt)
    // A real implementation would handle blocks/sequences of statements.
    if (func_def->body) {
        visit_statement(func_def->body, tac_function, arena, &next_temp_id);
    } else {
        // Handle functions with empty bodies if necessary
        fprintf(stderr, "Warning: Function '%s' has an empty body.\n", func_def->name);
    }

    return tac_program;
}

// --- Static Helper Function Implementations ---

// Visitor for statement nodes
static void visit_statement(AstNode* node, TacFunction* current_function, Arena* arena, int* next_temp_id_ptr) {
    // ReSharper disable once CppDFAConstantConditions
    // ReSharper disable once CppDFAUnreachableCode
    if (!node) return;

     switch (node->type) {
        case NODE_RETURN_STMT: {
            const ReturnStmtNode* ret_node = (ReturnStmtNode*)node;
            // Visit the expression to generate its TAC and get the result operand
            const TacOperand result_operand = visit_expression(ret_node->expression, current_function, arena, next_temp_id_ptr);

            // Check if the expression produced a valid result
            if (!is_valid_operand(result_operand)) {
                fprintf(stderr, "Error: Return statement's expression did not yield a valid result operand.\n");
                // Potentially set an error flag in the context?
                return;
            }

            // Create and add the RETURN instruction
            const TacInstruction* ret_instr = create_tac_instruction_return(result_operand, arena);
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

// Visitor for expression nodes (returns the operand holding the result)
static TacOperand visit_expression(AstNode* node, TacFunction* current_function, Arena* arena, int* next_temp_id_ptr) {
    if (!node) return create_invalid_operand();

    switch (node->type) {
        case NODE_INT_LITERAL: {
            const IntLiteralNode* int_node = (IntLiteralNode*)node;
            // Integer literals directly translate to constant operands
            return create_tac_operand_const(int_node->value);
        }
        case NODE_UNARY_OP: {
            const UnaryOpNode* unary_node = (UnaryOpNode*)node;

            // 1. Visit the operand expression first
            const TacOperand operand_result = visit_expression(unary_node->operand, current_function, arena, next_temp_id_ptr);
            if (!is_valid_operand(operand_result)) {
                fprintf(stderr, "Error: Unary operator's operand did not yield a valid result.\n");
                return create_invalid_operand();
            }

            // 2. Create a new temporary to store the result of the unary operation
            //    Use the counter passed via pointer.
            const TacOperand dest_temp = create_tac_operand_temp((*next_temp_id_ptr)++);

            // 3. Create the specific TAC instruction based on the operator
            const TacInstruction* unary_instr = NULL;
            switch (unary_node->op) {
                case OPERATOR_NEGATE:
                    unary_instr = create_tac_instruction_negate(dest_temp, operand_result, arena);
                    break;
                case OPERATOR_COMPLEMENT:
                     unary_instr = create_tac_instruction_complement(dest_temp, operand_result, arena);
                    break;
                // case OPERATOR_LOGICAL_NOT: // Add if/when this operator exists
                //     unary_instr = create_tac_instruction_logical_not(dest_temp, operand_result, arena);
                //     break;
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
        // Add cases for other expressions (BinaryOp, FunctionCall, etc.)
        default:
            fprintf(stderr, "Error: Unexpected AST node type %d in visit_expression.\n", node->type);
            return create_invalid_operand();
    }
}
