#include "tac.h"
#include <stdlib.h> // For NULL, realloc (though we'll use arena)
#include <string.h> // For strcmp, strdup (use arena_alloc for strings)
#include <stdio.h>  // For error printing if needed

// Initial capacity for dynamic arrays
#define INITIAL_CAPACITY 8

//------------------------------------------------------------------------------
// Operand Creation
//------------------------------------------------------------------------------

TacOperand create_tac_operand_const(const int value) {
    // Operands are usually passed by value or allocated as part of instructions,
    // so direct arena allocation isn't typically needed *for the operand struct itself*,
    // unless it contains allocated data like strings (which we don't have yet).
    TacOperand op;
    op.type = TAC_OPERAND_CONST;
    op.value.constant_value = value;
    return op;
}

TacOperand create_tac_operand_temp(const int temp_id) {
    TacOperand op;
    op.type = TAC_OPERAND_TEMP;
    op.value.temp_id = temp_id;
    return op;
}

//------------------------------------------------------------------------------
// Instruction Creation
//------------------------------------------------------------------------------

// Helper to create a generic instruction structure
static TacInstruction* create_base_instruction(const TacInstructionType type, Arena* arena) {
    TacInstruction* instr = arena_alloc(arena, sizeof(TacInstruction));
    if (!instr) {
        // Handle allocation failure (e.g., return NULL, exit, depends on strategy)
        perror("Failed to allocate TAC instruction");
        exit(EXIT_FAILURE); // Simple strategy for now
    }
    instr->type = type;
    return instr;
}

TacInstruction* create_tac_instruction_copy(const TacOperand dst, const TacOperand src, Arena* arena) {
    TacInstruction* instr = create_base_instruction(TAC_INS_COPY, arena);
    instr->operands.copy.dst = dst;
    instr->operands.copy.src = src;
    return instr;
}

TacInstruction* create_tac_instruction_negate(const TacOperand dst, const TacOperand src, Arena* arena) {
    TacInstruction* instr = create_base_instruction(TAC_INS_NEGATE, arena);
    instr->operands.unary_op.dst = dst;
    instr->operands.unary_op.src = src;
    return instr;
}

TacInstruction* create_tac_instruction_complement(const TacOperand dst, const TacOperand src, Arena* arena) {
    TacInstruction* instr = create_base_instruction(TAC_INS_COMPLEMENT, arena);
    instr->operands.unary_op.dst = dst;
    instr->operands.unary_op.src = src;
    return instr;
}

TacInstruction* create_tac_instruction_return(const TacOperand src, Arena* arena) {
    TacInstruction* instr = create_base_instruction(TAC_INS_RETURN, arena);
    instr->operands.ret.src = src;
    return instr;
}

//------------------------------------------------------------------------------
// Function and Program Manipulation
//------------------------------------------------------------------------------

TacFunction* create_tac_function(const char* name, Arena* arena) {
    TacFunction* func = (TacFunction*)arena_alloc(arena, sizeof(TacFunction));
    if (!func) {
        perror("Failed to allocate TAC function");
        exit(EXIT_FAILURE);
    }

    // Allocate space for the name string in the arena and copy it
    size_t name_len = strlen(name) + 1;
    char* name_copy = (char*)arena_alloc(arena, name_len);
    if (!name_copy) {
         perror("Failed to allocate TAC function name");
         exit(EXIT_FAILURE);
    }
    memcpy(name_copy, name, name_len);
    func->name = name_copy;

    func->instruction_count = 0;
    func->instruction_capacity = INITIAL_CAPACITY;
    func->instructions = (TacInstruction*)arena_alloc(arena, func->instruction_capacity * sizeof(TacInstruction));
    if (!func->instructions) {
        perror("Failed to allocate initial TAC instructions array");
        exit(EXIT_FAILURE);
    }

    return func;
}

void add_instruction_to_function(TacFunction* func, TacInstruction* instr, Arena* arena) {
    if (func->instruction_count >= func->instruction_capacity) {
        // Grow the array using the arena.
        // Note: Arena allocators typically don't support 'realloc'.
        // We allocate a new, larger block and copy the old data.
        // The old block remains allocated in the arena but is unused.
        size_t new_capacity = func->instruction_capacity * 2;
        TacInstruction* new_instructions = (TacInstruction*)arena_alloc(arena, new_capacity * sizeof(TacInstruction));
        if (!new_instructions) {
             perror("Failed to grow TAC instructions array");
             exit(EXIT_FAILURE);
        }
        // Copy existing instructions - *instr is a pointer, copy the struct itself*
        memcpy(new_instructions, func->instructions, func->instruction_count * sizeof(TacInstruction));

        func->instructions = new_instructions;
        func->instruction_capacity = new_capacity;
    }
    // Copy the *content* of the instruction pointed to by instr into the array
    func->instructions[func->instruction_count++] = *instr;
}

TacProgram* create_tac_program(Arena* arena) {
    TacProgram* prog = (TacProgram*)arena_alloc(arena, sizeof(TacProgram));
    if (!prog) {
        perror("Failed to allocate TAC program");
        exit(EXIT_FAILURE);
    }

    prog->function_count = 0;
    prog->function_capacity = INITIAL_CAPACITY;
    // Allocate an array of *pointers* to TacFunction
    prog->functions = (TacFunction**)arena_alloc(arena, prog->function_capacity * sizeof(TacFunction*));
    if (!prog->functions) {
         perror("Failed to allocate initial TAC functions array");
         exit(EXIT_FAILURE);
    }
    return prog;
}

void add_function_to_program(TacProgram* prog, TacFunction* func, Arena* arena) {
    if (prog->function_count >= prog->function_capacity) {
        // Grow the array of function pointers
        size_t new_capacity = prog->function_capacity * 2;
        TacFunction** new_functions = (TacFunction**)arena_alloc(arena, new_capacity * sizeof(TacFunction*));
         if (!new_functions) {
             perror("Failed to grow TAC functions array");
             exit(EXIT_FAILURE);
        }
        // Copy existing function pointers
        memcpy(new_functions, prog->functions, prog->function_count * sizeof(TacFunction*));

        prog->functions = new_functions;
        prog->function_capacity = new_capacity;
    }
    prog->functions[prog->function_count++] = func;
}
