#include "codegen.h"
#include "../ir/tac.h" // Include TAC definitions for TacInstructionNode, TacFunction, etc.
#include "../src/strings/strings.h" // Use the new string buffer
#include <stdio.h>
#include <stdbool.h> // Needed for bool
#include <string.h>

// --- Helper function to calculate maximum temporary ID used in a function ---
int calculate_max_temp_id(const TacFunction *func) {
    int max_id = -1;
    if (!func || !func->instructions) {
        return -1;
    }

    // Correct loop:
    for (size_t i = 0; i < func->instruction_count; ++i) {
        const TacInstruction *instr = &func->instructions[i];
        // Array to hold operands of the current instruction that might be temporaries
        const TacOperand *operands_to_inspect[3]; // Max 3 for binary ops (dst, src1, src2)
        int num_ops_to_inspect = 0;

        switch (instr->type) {
            case TAC_INS_COPY:
                operands_to_inspect[0] = &instr->operands.copy.dst;
                operands_to_inspect[1] = &instr->operands.copy.src;
                num_ops_to_inspect = 2;
                break;
            case TAC_INS_RETURN:
                operands_to_inspect[0] = &instr->operands.ret.src;
                num_ops_to_inspect = 1;
                break;
            case TAC_INS_NEGATE:
            case TAC_INS_COMPLEMENT:
                operands_to_inspect[0] = &instr->operands.unary_op.dst;
                operands_to_inspect[1] = &instr->operands.unary_op.src;
                num_ops_to_inspect = 2;
                break;
            // Future: Add cases for ADD, SUB, MULT, DIV, etc.
            // Example for a binary operation:
            // case TAC_INS_ADD:
            //     operands_to_inspect[0] = &instr->operands.binary_op.dst;
            //     operands_to_inspect[1] = &instr->operands.binary_op.op1;
            //     operands_to_inspect[2] = &instr->operands.binary_op.op2;
            //     num_ops_to_inspect = 3;
            //     break;
            default:
                // Instructions like LABEL, JUMP, CALL might need different handling or no temp inspection here.
                break;
        }

        for (int i = 0; i < num_ops_to_inspect; ++i) {
            if (operands_to_inspect[i] && operands_to_inspect[i]->type == TAC_OPERAND_TEMP) {
                if (operands_to_inspect[i]->value.temp_id > max_id) {
                    max_id = operands_to_inspect[i]->value.temp_id;
                }
            }
        }
    }
    return max_id;
}

// --- Forward declarations for static helper functions (TAC processors) ---
static bool generate_tac_function(const TacFunction *func, StringBuffer *sb);

static bool generate_tac_instruction(const TacInstruction *instr, const TacFunction *current_function,
                                     StringBuffer *sb);

// --- Main function ---

bool codegen_generate_program(TacProgram *tac_program, StringBuffer *sb) {
    if (!tac_program) {
        fprintf(stderr, "Codegen Error: Cannot generate assembly from NULL TAC program\n");
        return false;
    }
    // string_buffer_clear(sb); // Ensure buffer is empty if needed, or manage externally

    // Iterate through each function in the TAC program
    for (size_t i = 0; i < tac_program->function_count; ++i) {
        if (!generate_tac_function(tac_program->functions[i], sb)) {
            fprintf(stderr, "Codegen Error: Failed to generate function %s\n", tac_program->functions[i]->name);
            return false; // Propagate error
        }
    }

    return true;
}

// --- Static helper function implementations (stubs for now) ---

static bool generate_tac_function(const TacFunction *func, StringBuffer *sb) {
    if (!func) {
        fprintf(stderr, "Codegen Error: Cannot generate code for NULL TacFunction.\n");
        return false;
    }

    // 1. Emit function label and global directive
    string_buffer_append(sb, ".globl _%s\n", func->name);
    string_buffer_append(sb, "_%s:\n", func->name);

    // 2. Function Prologue
    string_buffer_append(sb, "    pushq %%rbp\n");
    string_buffer_append(sb, "    movq %%rsp, %%rbp\n");

    // Calculate stack space needed for temporaries
    const int max_temp_id = calculate_max_temp_id(func);
    const size_t bytes_for_temps = max_temp_id == -1 ? 0 : (size_t) (max_temp_id + 1) * 8; // 8 bytes per temp

    // Round up to nearest multiple of 16 for stack alignment.
    // Example: if bytes_for_temps = 0,  (0 + 15) & ~15UL = 15 & 0xFF..F0 = 0
    //          if bytes_for_temps = 1,  (1 + 15) & ~15UL = 16 & 0xFF..F0 = 16
    //          if bytes_for_temps = 15, (15 + 15) & ~15UL = 30 & 0xFF..F0 = 16
    //          if bytes_for_temps = 16, (16 + 15) & ~15UL = 31 & 0xFF..F0 = 16
    //          if bytes_for_temps = 17, (17 + 15) & ~15UL = 32 & 0xFF..F0 = 32
    // ~15UL is a mask like ...1111111111110000, effectively clearing the last 4 bits.
    size_t stack_allocation_size = bytes_for_temps + 15 & ~15UL;

    // Enforce a minimum stack frame size (e.g., 32 bytes for general ABI compliance/simplicity)
    if (stack_allocation_size < 32) {
        stack_allocation_size = 32;
    }

    // Allocate stack space if needed (it will always be at least 32 now)
    string_buffer_append(sb, "    subq $%zu, %%rsp\n", stack_allocation_size);

    // Save callee-saved registers if any (TODO: Implement this later)

    // 4. Iterate through instructions and call generate_tac_instruction
    for (size_t i = 0; i < func->instruction_count; ++i) {
        if (!generate_tac_instruction(&func->instructions[i], func, sb)) {
            fprintf(stderr, "Codegen Error: Failed to generate instruction in function %s\n", func->name);
            return false; // Propagate error
        }
    }

    // 5. Function Epilogue
    // The 'leave' instruction is equivalent to 'movq %rbp, %rsp; popq %rbp'
    // string_buffer_append(sb, "    movq %%rbp, %%rsp\n");
    // string_buffer_append(sb, "    popq %%rbp\n");
    // Using leave is more concise if stack_space was allocated with subq.
    if (stack_allocation_size > 0) {
        // If we modified rsp, restore it properly.
        string_buffer_append(sb, "    leave\n");
    } else {
        // If no stack space was allocated, rbp is still rsp, so just pop rbp.
        // This case might be rare or indicate an empty function that doesn't need full prologue/epilogue.
        // However, for a standard function call structure, even empty ones might have the push/mov/pop/ret.
        // Let's assume for now that if we pushed rbp, we pop it.
        string_buffer_append(sb, "    popq %%rbp\n"); // This should balance the initial pushq %rbp
    }
    string_buffer_append(sb, "    retq\n");

    // fprintf(stdout, "Placeholder: generate_tac_function for %s (not fully implemented)\n", func->name);
    return true;
}

static bool generate_tac_instruction(const TacInstruction *instr, const TacFunction *current_function,
                                     StringBuffer *sb) {
    if (!instr) {
        fprintf(stderr, "Codegen Error: Cannot generate code for NULL TacInstruction.\n");
        return false;
    }

    // Buffer for operand string representations
    char op1_str[64];
    // char op2_str[64]; // For binary operations, if needed
    // char dest_str[64]; // For destination operands, if needed

    switch (instr->type) {
        case TAC_INS_RETURN:
            // No direct null check on instr->operands.ret.src as it's a struct.
            // create_tac_instruction_return ensures it's populated.
            // Error handling in operand_to_assembly_string will catch issues with the operand itself.
            if (!operand_to_assembly_string(&instr->operands.ret.src, op1_str, sizeof(op1_str))) {
                fprintf(stderr, "Codegen Error: Could not convert operand for RETURN in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }
        // Assuming integer return type, fits in %eax.
        // The actual retq instruction is handled by the function epilogue.
            string_buffer_append(sb, "    movl %s, %%eax\n", op1_str);
            break;

        case TAC_INS_COPY: {
            // Scoped for local vars src_str, dst_str
            char src_str[64];
            char dst_str[64];

            if (!operand_to_assembly_string(&instr->operands.copy.src, src_str, sizeof(src_str))) {
                fprintf(stderr, "Codegen Error: Could not convert source operand for COPY in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }
            if (!operand_to_assembly_string(&instr->operands.copy.dst, dst_str, sizeof(dst_str))) {
                fprintf(stderr, "Codegen Error: Could not convert destination operand for COPY in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }
            string_buffer_append(sb, "    movl %s, %s\n", src_str, dst_str);
            break;
        }

        case TAC_INS_NEGATE: {
            // dst = -src
            char src_str[64];
            char dst_str[64];

            // Ensure destination is a temporary
            if (instr->operands.unary_op.dst.type != TAC_OPERAND_TEMP) {
                fprintf(stderr, "Codegen Error: Destination for NEGATE must be a temporary operand in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }

            if (!operand_to_assembly_string(&instr->operands.unary_op.src, src_str, sizeof(src_str))) {
                fprintf(stderr, "Codegen Error: Could not convert source operand for NEGATE in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }
            if (!operand_to_assembly_string(&instr->operands.unary_op.dst, dst_str, sizeof(dst_str))) {
                fprintf(stderr, "Codegen Error: Could not convert destination operand for NEGATE in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }

            // If src and dst are not the same temporary, mov src to dst first.
            // A simple string comparison works if both are temps mapped to stack locations.
            if (strcmp(src_str, dst_str) != 0) {
                string_buffer_append(sb, "    movl %s, %s\n", src_str, dst_str);
            }
            string_buffer_append(sb, "    negl %s\n", dst_str);
            break;
        }

        case TAC_INS_COMPLEMENT: {
            // dst = ~src
            char src_str[64];
            char dst_str[64];

            // Ensure destination is a temporary
            if (instr->operands.unary_op.dst.type != TAC_OPERAND_TEMP) {
                fprintf(
                    stderr, "Codegen Error: Destination for COMPLEMENT must be a temporary operand in function %s.\n",
                    current_function ? current_function->name : "<unknown>");
                return false;
            }

            if (!operand_to_assembly_string(&instr->operands.unary_op.src, src_str, sizeof(src_str))) {
                fprintf(stderr, "Codegen Error: Could not convert source operand for COMPLEMENT in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }
            if (!operand_to_assembly_string(&instr->operands.unary_op.dst, dst_str, sizeof(dst_str))) {
                fprintf(stderr, "Codegen Error: Could not convert destination operand for COMPLEMENT in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }

            if (strcmp(src_str, dst_str) != 0) {
                string_buffer_append(sb, "    movl %s, %s\n", src_str, dst_str);
            }
            string_buffer_append(sb, "    notl %s\n", dst_str);
            break;
        }

        default:
            fprintf(stderr, "Codegen Error: Unhandled TAC operation type %d in function %s.\n",
                    instr->type, current_function ? current_function->name : "<unknown>");
        // For debugging, append a comment indicating the unhandled instruction
            string_buffer_append(sb, "    # Unhandled TAC Op: %d\n", instr->type);
            return false; // For now, consider unhandled operations as an error
    }

    return true; // Placeholder
}

// Convert a TAC operand to its assembly string representation.
bool operand_to_assembly_string(const TacOperand *op, char *out_buffer, size_t buffer_size) {
    if (!op) {
        fprintf(stderr, "operand_to_assembly_string: NULL operand provided\n");
        if (buffer_size > 0) out_buffer[0] = '\0'; // Ensure null-terminated empty string
        return false;
    }
    if (buffer_size == 0) {
        fprintf(stderr, "operand_to_assembly_string: Output buffer size is zero\n");
        return false;
    }

    int written = 0;
    switch (op->type) {
        case TAC_OPERAND_CONST:
            written = snprintf(out_buffer, buffer_size, "$%d", op->value.constant_value);
            break;
        case TAC_OPERAND_TEMP:
            written = snprintf(out_buffer, buffer_size, "-%d(%%rbp)", (op->value.temp_id + 1) * 8);
            break;
        default:
            fprintf(stderr, "operand_to_assembly_string: Unhandled operand type %d\n", op->type);
            snprintf(out_buffer, buffer_size, "<UNHANDLED_OPERAND_TYPE_%d>", op->type);
            return false; // Indicate failure for unhandled types
    }

    if (written < 0 || (size_t) written >= buffer_size) {
        fprintf(stderr, "operand_to_assembly_string: snprintf error or buffer too small.\n");
        // Ensure buffer is null-terminated even on error if possible
        out_buffer[buffer_size - 1] = '\0';
        return false; // Error or truncation
    }

    return true;
}
