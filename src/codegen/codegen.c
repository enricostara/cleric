#include "codegen.h"
#include "../ir/tac.h"          // For TacProgram, TacFunction, TacInstruction, TacOperand
#include "../strings/strings.h" // Use the new string buffer
#include <stdio.h>
#include <stdbool.h> // Needed for bool

// --- Forward declarations for static helper functions (TAC processors) ---
static bool generate_tac_function(const TacFunction *func, StringBuffer *sb);
static bool generate_tac_instruction(const TacInstruction *instr, const TacFunction *current_function, StringBuffer *sb);
static void operand_to_assembly_string(const TacOperand *operand, char *buffer, size_t buffer_size, const TacFunction *current_function);

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
    if (func->instruction_count > 0) { // Only allocate if there are instructions (and thus potentially temporaries)
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
    if (stack_space > 0) { // If we modified rsp, restore it properly.
        string_buffer_append(sb, "    leave\n");
    } else { // If no stack space was allocated, rbp is still rsp, so just pop rbp.
        // This case might be rare or indicate an empty function that doesn't need full prologue/epilogue.
        // However, for a standard function call structure, even empty ones might have the push/mov/pop/ret.
        // Let's assume for now that if we pushed rbp, we pop it.
        string_buffer_append(sb, "    popq %%rbp\n"); // This should balance the initial pushq %rbp
    }
    string_buffer_append(sb, "    retq\n");

    // fprintf(stdout, "Placeholder: generate_tac_function for %s (not fully implemented)\n", func->name);
    return true;
}

static bool generate_tac_instruction(const TacInstruction *instr, const TacFunction *current_function, StringBuffer *sb) {
    if (!instr) {
        fprintf(stderr, "Codegen Error: Cannot generate code for NULL TacInstruction.\n");
        return false;
    }
    // Placeholder: Print instruction type (useful for debugging)
    // string_buffer_append(sb, "# Generating instruction type: %d\n", instr->type);
    fprintf(stdout, "Placeholder: generate_tac_instruction for type %d (not implemented)\n", instr->type);
    return true; // Placeholder
}

// Placeholder for operand to assembly string conversion
static void operand_to_assembly_string(const TacOperand *operand, char *buffer, size_t buffer_size, const TacFunction *current_function) {
    if (!operand) {
        snprintf(buffer, buffer_size, "<null_operand>");
        return;
    }
    // This will be complex, mapping temporaries to stack/registers, constants to immediates, etc.
    snprintf(buffer, buffer_size, "<operand_type_%d_val_%lld>", operand->type, operand->value.constant_value); // Example placeholder
}
