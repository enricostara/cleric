#include "codegen.h"
#include "../ir/tac.h"          // For TacProgram, TacFunction, TacInstruction, TacOperand
#include "../strings/strings.h" // Use the new string buffer
#include <stdio.h>
#include <stdbool.h> // Needed for bool

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

    // 3. Stack Allocation for locals/temporaries
    // TODO: Calculate the actual stack space needed based on temporaries and local variables.
    // For now, allocate a fixed small amount (e.g., 16 bytes) for initial testing.
    // Each temporary might take 4 or 8 bytes depending on the system/types.
    // If func->max_temp_id is available and represents the highest temp_id used (0-indexed),
    // we might allocate (func->max_temp_id + 1) * 8 bytes (assuming 8-byte slots for simplicity for now).
    // Let's use a fixed 32 bytes for now as a placeholder if max_temp_id is not yet populated or reliable.
    unsigned int stack_space = 32; // Placeholder stack space
    if (func->instruction_count > 0) {
        // Only allocate if there are instructions (and thus potentially temporaries)
        // Example if max_temp_id was a field in TacFunction:
        // if (func->max_temp_id >= 0) { // Check if any temporaries are used
        //    stack_space = (func->max_temp_id + 1) * 8; // 8 bytes per temporary for simplicity
        //    stack_space = (stack_space + 15) & ~15; // Align to 16 bytes (optional but good practice)
        // } else {
        //    stack_space = 16; // Minimum if no temps but still want prologue/epilogue for consistency
        // }
    }
    if (stack_space > 0) {
        string_buffer_append(sb, "    subq $%u, %%rsp\n", stack_space);
    }

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
    if (stack_space > 0) {
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

        // TODO: Add cases for other TAC operations (ASSIGN, ADD, SUB, etc.)

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
