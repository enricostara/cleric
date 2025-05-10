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
            case TAC_INS_ADD:
            case TAC_INS_SUB:
            case TAC_INS_MUL:
            case TAC_INS_DIV:
            case TAC_INS_MOD:
                operands_to_inspect[0] = &instr->operands.binary_op.dst;
                operands_to_inspect[1] = &instr->operands.binary_op.src1;
                operands_to_inspect[2] = &instr->operands.binary_op.src2;
                num_ops_to_inspect = 3;
                break;
            default:
                // Instructions like LABEL, JUMP, CALL might need different handling or no temp inspection here.
                break;
        }

        for (int j = 0; j < num_ops_to_inspect; ++j) {
            if (operands_to_inspect[j] && operands_to_inspect[j]->type == TAC_OPERAND_TEMP) {
                if (operands_to_inspect[j]->value.temp_id > max_id) {
                    max_id = operands_to_inspect[j]->value.temp_id;
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
    char op2_str[64];
    char dest_str[64];

    switch (instr->type) {
        case TAC_INS_RETURN: {
            if (!operand_to_assembly_string(&instr->operands.ret.src, op1_str, sizeof(op1_str))) {
                fprintf(stderr, "Codegen Error: Could not convert operand for RETURN in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }
            string_buffer_append(sb, "    movl %s, %%eax\n", op1_str); // Result in %eax for return
            break;
        }
        case TAC_INS_COPY: {
            if (!operand_to_assembly_string(&instr->operands.copy.src, op1_str, sizeof(op1_str)) ||
                !operand_to_assembly_string(&instr->operands.copy.dst, dest_str, sizeof(dest_str))) {
                fprintf(stderr, "Codegen Error: Could not convert operands for COPY in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }
            // If both are stack temporaries (memory operands), use an intermediate register.
            // A simple check for now: if they start with '-' (e.g., "-8(%rbp)")
            if (op1_str[0] == '-' && dest_str[0] == '-') { // Temp-to-Temp
                string_buffer_append(sb, "    movl %s, %%r10d\n", op1_str); // mov src_temp to r10d
                string_buffer_append(sb, "    movl %%r10d, %s\n", dest_str); // mov r10d to dst_temp
            } else if (op1_str[0] == '$' && dest_str[0] == '-') { // Const-to-Temp
                string_buffer_append(sb, "    movl %s, %s\n", op1_str, dest_str); // movl $const, temp_mem
            } else {
                 // Other cases (e.g., Temp-to-Register (if we had them), or more complex scenarios)
                 // For now, use intermediate register as a general approach.
                string_buffer_append(sb, "    movl %s, %%r10d\n", op1_str); 
                string_buffer_append(sb, "    movl %%r10d, %s\n", dest_str); 
            }
            break;
        }
        case TAC_INS_NEGATE: {
            const TacOperand *src_op = &instr->operands.unary_op.src;
            const TacOperand *dst_op = &instr->operands.unary_op.dst;

            if (!operand_to_assembly_string(src_op, op1_str, sizeof(op1_str)) ||
                !operand_to_assembly_string(dst_op, dest_str, sizeof(dest_str))) {
                fprintf(stderr, "Codegen Error: Could not convert operands for NEGATE in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }

            if (src_op->type == TAC_OPERAND_TEMP &&
                dst_op->type == TAC_OPERAND_TEMP &&
                src_op->value.temp_id == dst_op->value.temp_id) {
                // Operate in place: negl memory_operand
                string_buffer_append(sb, "    negl %s\n", dest_str);
            } else {
                // Use %eax as intermediary
                string_buffer_append(sb, "    movl %s, %%eax\n", op1_str);
                string_buffer_append(sb, "    negl %%eax\n");
                string_buffer_append(sb, "    movl %%eax, %s\n", dest_str);
            }
            break;
        }
        case TAC_INS_COMPLEMENT: {
            const TacOperand *src_op = &instr->operands.unary_op.src;
            const TacOperand *dst_op = &instr->operands.unary_op.dst;

            if (!operand_to_assembly_string(src_op, op1_str, sizeof(op1_str)) ||
                !operand_to_assembly_string(dst_op, dest_str, sizeof(dest_str))) {
                fprintf(stderr, "Codegen Error: Could not convert operands for COMPLEMENT in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }

            if (src_op->type == TAC_OPERAND_TEMP &&
                dst_op->type == TAC_OPERAND_TEMP &&
                src_op->value.temp_id == dst_op->value.temp_id) {
                 // Operate in place: notl memory_operand
                string_buffer_append(sb, "    notl %s\n", dest_str);
            } else {
                // Use %eax as intermediary
                string_buffer_append(sb, "    movl %s, %%eax\n", op1_str);
                string_buffer_append(sb, "    notl %%eax\n");
                string_buffer_append(sb, "    movl %%eax, %s\n", dest_str);
            }
            break;
        }
        case TAC_INS_ADD:
        case TAC_INS_SUB:
        case TAC_INS_MUL: {
            if (!operand_to_assembly_string(&instr->operands.binary_op.src1, op1_str, sizeof(op1_str)) ||
                !operand_to_assembly_string(&instr->operands.binary_op.src2, op2_str, sizeof(op2_str)) ||
                !operand_to_assembly_string(&instr->operands.binary_op.dst, dest_str, sizeof(dest_str))) {
                fprintf(stderr, "Codegen Error: Could not convert operands for ADD/SUB/MUL in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }
            string_buffer_append(sb, "    movl %s, %%eax\n", op1_str); // mov src1 to eax
            if (instr->type == TAC_INS_ADD) {
                string_buffer_append(sb, "    addl %s, %%eax\n", op2_str); // add src2 to eax
            } else if (instr->type == TAC_INS_SUB) {
                string_buffer_append(sb, "    subl %s, %%eax\n", op2_str); // sub src2 from eax
            } else {
                // TAC_INS_MUL
                string_buffer_append(sb, "    imull %s, %%eax\n", op2_str); // mul eax by src2
            }
            string_buffer_append(sb, "    movl %%eax, %s\n", dest_str); // mov result from eax to dst
            break;
        }
        case TAC_INS_DIV:
        case TAC_INS_MOD: {
            if (!operand_to_assembly_string(&instr->operands.binary_op.src1, op1_str, sizeof(op1_str)) || // Dividend
                !operand_to_assembly_string(&instr->operands.binary_op.src2, op2_str, sizeof(op2_str)) || // Divisor
                !operand_to_assembly_string(&instr->operands.binary_op.dst, dest_str, sizeof(dest_str))) {
                // Destination
                fprintf(stderr, "Codegen Error: Could not convert operands for DIV/MOD in function %s.\n",
                        current_function ? current_function->name : "<unknown>");
                return false;
            }
            string_buffer_append(sb, "    movl %s, %%eax\n", op1_str); // Move dividend (src1) into eax
            string_buffer_append(sb, "    cltd\n"); // Sign-extend eax into edx:eax (cdq for 32-bit)
            string_buffer_append(sb, "    idivl %s\n", op2_str);
            // Divide edx:eax by src2. Quotient in eax, Remainder in edx.

            if (instr->type == TAC_INS_DIV) {
                string_buffer_append(sb, "    movl %%eax, %s\n", dest_str); // Store quotient (eax) into dst
            } else {
                // TAC_INS_MOD
                string_buffer_append(sb, "    movl %%edx, %s\n", dest_str); // Store remainder (edx) into dst
            }
            break;
        }
        default:
            fprintf(stderr, "Codegen Warning: Unhandled TAC instruction type %d in function %s. No code generated.\n",
                    instr->type, current_function ? current_function->name : "<unknown>");
            // return false; // Optionally treat unhandled instructions as an error
            break;
    }
    return true;
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
