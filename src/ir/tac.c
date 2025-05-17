#include "tac.h"
#include <stdlib.h> // For NULL, realloc (though we'll use arena)
#include <string.h> // For strdup (used in create_tac_function)
#include <stdio.h>  // For snprintf (can be used with string_buffer_append if needed, though often not directly)
#include "../memory/arena.h"
#include "../strings/strings.h" // Include StringBuffer header

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

TacOperand create_tac_operand_label(const char *name, Arena *arena) {
    TacOperand op;
    op.type = TAC_OPERAND_LABEL;
    // Assume name is already managed (e.g., arena-allocated by caller or string literal)
    // If name needs to be copied into the arena specifically for this operand:
    // size_t name_len = strlen(name) + 1;
    // char *name_copy = arena_alloc(arena, name_len);
    // if (!name_copy) { perror("Failed to allocate TAC label name"); exit(EXIT_FAILURE); }
    // memcpy(name_copy, name, name_len);
    // op.value.label_name = name_copy;
    op.value.label_name = name; // Assuming name is persistent
    return op;
}

//------------------------------------------------------------------------------
// Instruction Creation
//------------------------------------------------------------------------------

// Helper to create a generic instruction structure
static TacInstruction *create_base_instruction(const TacInstructionType type, Arena *arena) {
    TacInstruction *instr = arena_alloc(arena, sizeof(TacInstruction));
    if (!instr) {
        // Handle allocation failure (e.g., return NULL, exit, depends on strategy)
        perror("Failed to allocate TAC instruction");
        exit(EXIT_FAILURE); // Simple strategy for now
    }
    instr->type = type;
    return instr;
}

TacInstruction *create_tac_instruction_copy(const TacOperand dst, const TacOperand src, Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_COPY, arena);
    instr->operands.copy.dst = dst;
    instr->operands.copy.src = src;
    return instr;
}

TacInstruction *create_tac_instruction_negate(const TacOperand dst, const TacOperand src, Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_NEGATE, arena);
    instr->operands.unary_op.dst = dst;
    instr->operands.unary_op.src = src;
    return instr;
}

TacInstruction *create_tac_instruction_complement(const TacOperand dst, const TacOperand src, Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_COMPLEMENT, arena);
    instr->operands.unary_op.dst = dst;
    instr->operands.unary_op.src = src;
    return instr;
}

TacInstruction *create_tac_instruction_logical_not(const TacOperand dst, const TacOperand src, Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_LOGICAL_NOT, arena);
    instr->operands.unary_op.dst = dst;
    instr->operands.unary_op.src = src;
    return instr;
}

TacInstruction *create_tac_instruction_return(const TacOperand src, Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_RETURN, arena);
    instr->operands.ret.src = src;
    return instr;
}

// --- Binary Instruction Creation ---
TacInstruction *create_tac_instruction_add(const TacOperand dst, const TacOperand src1, const TacOperand src2,
                                           Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_ADD, arena);
    instr->operands.binary_op.dst = dst;
    instr->operands.binary_op.src1 = src1;
    instr->operands.binary_op.src2 = src2;
    return instr;
}

TacInstruction *create_tac_instruction_sub(const TacOperand dst, const TacOperand src1, const TacOperand src2,
                                           Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_SUB, arena);
    instr->operands.binary_op.dst = dst;
    instr->operands.binary_op.src1 = src1;
    instr->operands.binary_op.src2 = src2;
    return instr;
}

TacInstruction *create_tac_instruction_mul(const TacOperand dst, const TacOperand src1, const TacOperand src2,
                                           Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_MUL, arena);
    instr->operands.binary_op.dst = dst;
    instr->operands.binary_op.src1 = src1;
    instr->operands.binary_op.src2 = src2;
    return instr;
}

TacInstruction *create_tac_instruction_div(const TacOperand dst, const TacOperand src1, const TacOperand src2,
                                           Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_DIV, arena);
    instr->operands.binary_op.dst = dst;
    instr->operands.binary_op.src1 = src1;
    instr->operands.binary_op.src2 = src2;
    return instr;
}

TacInstruction *create_tac_instruction_mod(const TacOperand dst, const TacOperand src1, const TacOperand src2,
                                           Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_MOD, arena);
    instr->operands.binary_op.dst = dst;
    instr->operands.binary_op.src1 = src1;
    instr->operands.binary_op.src2 = src2;
    return instr;
}

TacInstruction *create_tac_instruction_less(const TacOperand dst, const TacOperand src1, const TacOperand src2,
                                            Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_LESS, arena);
    instr->operands.relational_op.dst = dst;
    instr->operands.relational_op.src1 = src1;
    instr->operands.relational_op.src2 = src2;
    return instr;
}

TacInstruction *create_tac_instruction_greater(const TacOperand dst, const TacOperand src1, const TacOperand src2,
                                               Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_GREATER, arena);
    instr->operands.relational_op.dst = dst;
    instr->operands.relational_op.src1 = src1;
    instr->operands.relational_op.src2 = src2;
    return instr;
}

TacInstruction *create_tac_instruction_less_equal(const TacOperand dst, const TacOperand src1, const TacOperand src2,
                                                  Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_LESS_EQUAL, arena);
    instr->operands.relational_op.dst = dst;
    instr->operands.relational_op.src1 = src1;
    instr->operands.relational_op.src2 = src2;
    return instr;
}

TacInstruction *create_tac_instruction_greater_equal(const TacOperand dst, const TacOperand src1, const TacOperand src2,
                                                     Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_GREATER_EQUAL, arena);
    instr->operands.relational_op.dst = dst;
    instr->operands.relational_op.src1 = src1;
    instr->operands.relational_op.src2 = src2;
    return instr;
}

TacInstruction *create_tac_instruction_equal(const TacOperand dst, const TacOperand src1, const TacOperand src2,
                                             Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_EQUAL, arena);
    instr->operands.relational_op.dst = dst;
    instr->operands.relational_op.src1 = src1;
    instr->operands.relational_op.src2 = src2;
    return instr;
}

TacInstruction *create_tac_instruction_not_equal(const TacOperand dst, const TacOperand src1, const TacOperand src2,
                                                 Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_NOT_EQUAL, arena);
    instr->operands.relational_op.dst = dst;
    instr->operands.relational_op.src1 = src1;
    instr->operands.relational_op.src2 = src2;
    return instr;
}

TacInstruction *create_tac_instruction_label(const TacOperand label, Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_LABEL, arena);
    instr->operands.label_def.label = label;
    return instr;
}

TacInstruction *create_tac_instruction_goto(const TacOperand target_label, Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_GOTO, arena);
    instr->operands.go_to.target_label = target_label;
    return instr;
}

TacInstruction *create_tac_instruction_if_false_goto(const TacOperand condition_src, const TacOperand target_label,
                                                     Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_IF_FALSE_GOTO, arena);
    instr->operands.conditional_goto.condition_src = condition_src;
    instr->operands.conditional_goto.target_label = target_label;
    return instr;
}

TacInstruction *create_tac_instruction_if_true_goto(const TacOperand condition_src, const TacOperand target_label,
                                                     Arena *arena) {
    TacInstruction *instr = create_base_instruction(TAC_INS_IF_TRUE_GOTO, arena);
    instr->operands.conditional_goto.condition_src = condition_src;
    instr->operands.conditional_goto.target_label = target_label;
    return instr;
}

//------------------------------------------------------------------------------
// Function and Program Manipulation
//------------------------------------------------------------------------------

TacFunction *create_tac_function(const char *name, Arena *arena) {
    TacFunction *func = arena_alloc(arena, sizeof(TacFunction));
    if (!func) {
        perror("Failed to allocate TAC function");
        exit(EXIT_FAILURE);
    }

    // Allocate space for the name string in the arena and copy it
    size_t name_len = strlen(name) + 1;
    char *name_copy = arena_alloc(arena, name_len);
    if (!name_copy) {
        perror("Failed to allocate TAC function name");
        exit(EXIT_FAILURE);
    }
    memcpy(name_copy, name, name_len);
    func->name = name_copy;

    func->instruction_count = 0;
    func->instruction_capacity = INITIAL_CAPACITY;
    func->instructions = (TacInstruction *) arena_alloc(arena, func->instruction_capacity * sizeof(TacInstruction));
    if (!func->instructions) {
        perror("Failed to allocate initial TAC instructions array");
        exit(EXIT_FAILURE);
    }

    return func;
}

void add_instruction_to_function(TacFunction *func, const TacInstruction *instr, Arena *arena) {
    if (func->instruction_count >= func->instruction_capacity) {
        // Grow the array using the arena.
        // Note: Arena allocators typically don't support 'realloc'.
        // We allocate a new, larger block and copy the old data.
        // The old block remains allocated in the arena but is unused.
        const size_t new_capacity = func->instruction_capacity * 2;
        TacInstruction *new_instructions = arena_alloc(arena, new_capacity * sizeof(TacInstruction));
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

TacProgram *create_tac_program(Arena *arena) {
    TacProgram *prog = arena_alloc(arena, sizeof(TacProgram));
    if (!prog) {
        perror("Failed to allocate TAC program");
        exit(EXIT_FAILURE);
    }

    prog->function_count = 0;
    prog->function_capacity = INITIAL_CAPACITY;
    // Allocate an array of *pointers* to TacFunction
    prog->functions = (TacFunction **) arena_alloc(arena, prog->function_capacity * sizeof(TacFunction *));
    if (!prog->functions) {
        perror("Failed to allocate initial TAC functions array");
        exit(EXIT_FAILURE);
    }
    return prog;
}

void add_function_to_program(TacProgram *prog, TacFunction *func, Arena *arena) {
    if (prog->function_count >= prog->function_capacity) {
        // Grow the array of function pointers
        size_t new_capacity = prog->function_capacity * 2;
        TacFunction **new_functions = arena_alloc(arena, new_capacity * sizeof(TacFunction *));
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

//------------------------------------------------------------------------------
// Pretty Printing Functions (Implementations)
//------------------------------------------------------------------------------

// Helper to print indentation
static void print_tac_indent(StringBuffer *sb, const int level) {
    for (int i = 0; i < level; ++i) {
        string_buffer_append(sb, "  "); // Two spaces per level
    }
}

void tac_print_operand(StringBuffer *sb, const TacOperand *operand) {
    if (!operand) {
        string_buffer_append(sb, "<null_op>");
        return;
    }
    switch (operand->type) {
        case TAC_OPERAND_CONST:
            string_buffer_append(sb, "%d", operand->value.constant_value);
            break;
        case TAC_OPERAND_TEMP:
            string_buffer_append(sb, "t%d", operand->value.temp_id);
            break;
        case TAC_OPERAND_LABEL:
            string_buffer_append(sb, "%s", operand->value.label_name ? operand->value.label_name : "<null_label_name>");
            break;
        default:
            string_buffer_append(sb, "<unk_op_type:%d>", operand->type);
            break;
    }
}

void tac_print_instruction(StringBuffer *sb, const TacInstruction *instruction) {
    if (!instruction) {
        string_buffer_append(sb, "<null_instr>\n");
        return;
    }

    switch (instruction->type) {
        case TAC_INS_COPY:
            tac_print_operand(sb, &instruction->operands.copy.dst);
            string_buffer_append(sb, " = ");
            tac_print_operand(sb, &instruction->operands.copy.src);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_NEGATE:
            tac_print_operand(sb, &instruction->operands.unary_op.dst);
            string_buffer_append(sb, " = - ");
            tac_print_operand(sb, &instruction->operands.unary_op.src);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_COMPLEMENT:
            tac_print_operand(sb, &instruction->operands.unary_op.dst);
            string_buffer_append(sb, " = ~ ");
            tac_print_operand(sb, &instruction->operands.unary_op.src);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_RETURN:
            string_buffer_append(sb, "return ");
            tac_print_operand(sb, &instruction->operands.ret.src);
            string_buffer_append(sb, "\n");
            break;
        // Binary Operations
        case TAC_INS_ADD:
            tac_print_operand(sb, &instruction->operands.binary_op.dst);
            string_buffer_append(sb, " = ");
            tac_print_operand(sb, &instruction->operands.binary_op.src1);
            string_buffer_append(sb, " + ");
            tac_print_operand(sb, &instruction->operands.binary_op.src2);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_SUB:
            tac_print_operand(sb, &instruction->operands.binary_op.dst);
            string_buffer_append(sb, " = ");
            tac_print_operand(sb, &instruction->operands.binary_op.src1);
            string_buffer_append(sb, " - ");
            tac_print_operand(sb, &instruction->operands.binary_op.src2);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_MUL:
            tac_print_operand(sb, &instruction->operands.binary_op.dst);
            string_buffer_append(sb, " = ");
            tac_print_operand(sb, &instruction->operands.binary_op.src1);
            string_buffer_append(sb, " * ");
            tac_print_operand(sb, &instruction->operands.binary_op.src2);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_DIV:
            tac_print_operand(sb, &instruction->operands.binary_op.dst);
            string_buffer_append(sb, " = ");
            tac_print_operand(sb, &instruction->operands.binary_op.src1);
            string_buffer_append(sb, " / ");
            tac_print_operand(sb, &instruction->operands.binary_op.src2);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_MOD:
            tac_print_operand(sb, &instruction->operands.binary_op.dst);
            string_buffer_append(sb, " = ");
            tac_print_operand(sb, &instruction->operands.binary_op.src1);
            string_buffer_append(sb, " %% "); // Use %% for literal %
            tac_print_operand(sb, &instruction->operands.binary_op.src2);
            string_buffer_append(sb, "\n");
            break;
        // New Unary Op
        case TAC_INS_LOGICAL_NOT:
            tac_print_operand(sb, &instruction->operands.unary_op.dst);
            string_buffer_append(sb, " = ! ");
            tac_print_operand(sb, &instruction->operands.unary_op.src);
            string_buffer_append(sb, "\n");
            break;
        // New Relational Ops
        case TAC_INS_LESS:
            tac_print_operand(sb, &instruction->operands.relational_op.dst);
            string_buffer_append(sb, " = ");
            tac_print_operand(sb, &instruction->operands.relational_op.src1);
            string_buffer_append(sb, " < ");
            tac_print_operand(sb, &instruction->operands.relational_op.src2);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_GREATER:
            tac_print_operand(sb, &instruction->operands.relational_op.dst);
            string_buffer_append(sb, " = ");
            tac_print_operand(sb, &instruction->operands.relational_op.src1);
            string_buffer_append(sb, " > ");
            tac_print_operand(sb, &instruction->operands.relational_op.src2);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_LESS_EQUAL:
            tac_print_operand(sb, &instruction->operands.relational_op.dst);
            string_buffer_append(sb, " = ");
            tac_print_operand(sb, &instruction->operands.relational_op.src1);
            string_buffer_append(sb, " <= ");
            tac_print_operand(sb, &instruction->operands.relational_op.src2);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_GREATER_EQUAL:
            tac_print_operand(sb, &instruction->operands.relational_op.dst);
            string_buffer_append(sb, " = ");
            tac_print_operand(sb, &instruction->operands.relational_op.src1);
            string_buffer_append(sb, " >= ");
            tac_print_operand(sb, &instruction->operands.relational_op.src2);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_EQUAL:
            tac_print_operand(sb, &instruction->operands.relational_op.dst);
            string_buffer_append(sb, " = ");
            tac_print_operand(sb, &instruction->operands.relational_op.src1);
            string_buffer_append(sb, " == ");
            tac_print_operand(sb, &instruction->operands.relational_op.src2);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_NOT_EQUAL:
            tac_print_operand(sb, &instruction->operands.relational_op.dst);
            string_buffer_append(sb, " = ");
            tac_print_operand(sb, &instruction->operands.relational_op.src1);
            string_buffer_append(sb, " != ");
            tac_print_operand(sb, &instruction->operands.relational_op.src2);
            string_buffer_append(sb, "\n");
            break;
        // New Control Flow Instructions
        case TAC_INS_LABEL:
            tac_print_operand(sb, &instruction->operands.label_def.label);
            string_buffer_append(sb, ":");
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_GOTO:
            string_buffer_append(sb, "goto ");
            tac_print_operand(sb, &instruction->operands.go_to.target_label);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_IF_FALSE_GOTO:
            string_buffer_append(sb, "if_false ");
            tac_print_operand(sb, &instruction->operands.conditional_goto.condition_src);
            string_buffer_append(sb, " goto ");
            tac_print_operand(sb, &instruction->operands.conditional_goto.target_label);
            string_buffer_append(sb, "\n");
            break;
        case TAC_INS_IF_TRUE_GOTO:
            string_buffer_append(sb, "if_not_zero ");
            tac_print_operand(sb, &instruction->operands.conditional_goto.condition_src);
            string_buffer_append(sb, " goto ");
            tac_print_operand(sb, &instruction->operands.conditional_goto.target_label);
            string_buffer_append(sb, "\n");
            break;
        default:
            string_buffer_append(sb, "<unknown_instr_type:%d>\n", instruction->type);
            break;
    }
}

void tac_print_function(StringBuffer *sb, const TacFunction *function, const int indent_level) {
    if (!function) {
        print_tac_indent(sb, indent_level);
        string_buffer_append(sb, "<null_function>\n");
        return;
    }
    print_tac_indent(sb, indent_level);
    string_buffer_append(sb, "function %s:\n", function->name ? function->name : "<anonymous>");

    for (size_t i = 0; i < function->instruction_count; ++i) {
        print_tac_indent(sb, indent_level + 1); // Corrected: Instructions should be indented relative to function
        tac_print_instruction(sb, &function->instructions[i]);
    }
}

void tac_print_program(StringBuffer *sb, const TacProgram *program) {
    if (!program) {
        string_buffer_append(sb, "<null_tac_program>\n");
        return;
    }
    string_buffer_append(sb, "program:\n"); // Added colon
    for (size_t i = 0; i < program->function_count; ++i) {
        tac_print_function(sb, program->functions[i], 1); // Changed indent_level to 1
        if (i < program->function_count - 1) {
            string_buffer_append_char(sb, '\n'); // Add a newline between functions
        }
    }
    string_buffer_append(sb, "end program\n");
}
