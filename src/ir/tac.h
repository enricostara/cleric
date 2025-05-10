#ifndef CLERIC_TAC_H
#define CLERIC_TAC_H

#include <stddef.h> // For size_t
#include <stdint.h> // For int values (though standard int might suffice)

#include "../memory/arena.h" // For arena allocation
#include "../strings/strings.h" // For StringBuffer

//------------------------------------------------------------------------------
// Operands
//------------------------------------------------------------------------------

typedef enum {
    TAC_OPERAND_CONST, // Integer Constant
    TAC_OPERAND_TEMP,  // Temporary variable (register/stack slot) identified by ID
    // TAC_OPERAND_VAR // Future: Source variable identifier
    // TAC_OPERAND_LABEL // Future: Label identifier for jumps
} TacOperandType;

typedef struct {
    TacOperandType type;
    union {
        int constant_value; // For TAC_OPERAND_CONST
        int temp_id;        // For TAC_OPERAND_TEMP
        // const char* var_name; // Future: For TAC_OPERAND_VAR
        // const char* label_name; // Future: For TAC_OPERAND_LABEL
    } value;
} TacOperand;

//------------------------------------------------------------------------------
// Instructions
//------------------------------------------------------------------------------

typedef enum {
    TAC_INS_COPY,       // dst = src
    TAC_INS_NEGATE,     // dst = -src
    TAC_INS_COMPLEMENT, // dst = ~src
    TAC_INS_RETURN,     // return src

    // Binary Operations
    TAC_INS_ADD,        // dst = src1 + src2
    TAC_INS_SUB,        // dst = src1 - src2
    TAC_INS_MUL,        // dst = src1 * src2
    TAC_INS_DIV,        // dst = src1 / src2
    TAC_INS_MOD,        // dst = src1 % src2

    // Future instructions: GOTO, IFZ, CALL, PARAM, LABEL, etc.
} TacInstructionType;

// Forward declare TacInstruction for use in struct definitions if needed (not strictly necessary here)
// struct TacInstruction;

typedef struct {
    TacInstructionType type;
    union {
        // dst = src
        struct { TacOperand dst; TacOperand src; } copy;
        // dst = op src
        struct { TacOperand dst; TacOperand src; } unary_op; // Used for NEGATE, COMPLEMENT
        // return src
        struct { TacOperand src; } ret;
        // dst = src1 op src2
        struct { TacOperand dst; TacOperand src1; TacOperand src2; } binary_op; // For ADD, SUB, MUL, DIV, MOD

        // Future: struct { const char* label; } label;
        // Future: struct { const char* label; TacOperand condition; } conditional_jump; // e.g., ifz condition goto label
        // Future: struct { const char* func_name; TacOperand result; } call;
        // Future: struct { TacOperand param; } param;
    } operands;
} TacInstruction;

//------------------------------------------------------------------------------
// Function & Program Structure
//------------------------------------------------------------------------------

// Represents a single function in TAC
typedef struct {
    const char* name;                // Function name (label)
    TacInstruction* instructions;    // Dynamic array of instructions
    size_t instruction_count;        // Number of instructions currently in the array
    size_t instruction_capacity;     // Allocated capacity of the array
    // Future: Could add info about parameters, local variables, etc.
} TacFunction;

// Represents the entire program in TAC
typedef struct {
    TacFunction** functions;         // Dynamic array of function pointers
    size_t function_count;           // Number of functions currently in the array
    size_t function_capacity;        // Allocated capacity of the array
} TacProgram;

//------------------------------------------------------------------------------
// Helper Functions (Declarations - Implementation in tac.c)
//------------------------------------------------------------------------------

// Operand creation
TacOperand create_tac_operand_const(int value);
TacOperand create_tac_operand_temp(int temp_id);

// Instruction creation (simplified examples)
TacInstruction* create_tac_instruction_copy(TacOperand dst, TacOperand src, Arena* arena);
TacInstruction* create_tac_instruction_negate(TacOperand dst, TacOperand src, Arena* arena);
TacInstruction* create_tac_instruction_complement(TacOperand dst, TacOperand src, Arena* arena);
TacInstruction* create_tac_instruction_return(TacOperand src, Arena* arena);

// Binary instruction creation
TacInstruction* create_tac_instruction_add(TacOperand dst, TacOperand src1, TacOperand src2, Arena* arena);
TacInstruction* create_tac_instruction_sub(TacOperand dst, TacOperand src1, TacOperand src2, Arena* arena);
TacInstruction* create_tac_instruction_mul(TacOperand dst, TacOperand src1, TacOperand src2, Arena* arena);
TacInstruction* create_tac_instruction_div(TacOperand dst, TacOperand src1, TacOperand src2, Arena* arena);
TacInstruction* create_tac_instruction_mod(TacOperand dst, TacOperand src1, TacOperand src2, Arena* arena);

// Function and Program manipulation
TacFunction* create_tac_function(const char* name, Arena* arena);
void add_instruction_to_function(TacFunction* func, const TacInstruction* instr, Arena* arena);

TacProgram* create_tac_program(Arena* arena);
void add_function_to_program(TacProgram* prog, TacFunction* func, Arena* arena);

//------------------------------------------------------------------------------
// Pretty Printing Functions (Declarations - Implementation in tac.c)
//------------------------------------------------------------------------------

/**
 * @brief Prints a single TAC operand to a StringBuffer.
 * @param sb The StringBuffer to print to.
 * @param operand The operand to print.
 */
void tac_print_operand(StringBuffer *sb, const TacOperand* operand);

/**
 * @brief Prints a single TAC instruction to a StringBuffer.
 * @param sb The StringBuffer to print to.
 * @param instruction The instruction to print.
 */
void tac_print_instruction(StringBuffer *sb, const TacInstruction* instruction);

/**
 * @brief Prints a TAC function (including its instructions) to a StringBuffer.
 * @param sb The StringBuffer to print to.
 * @param function The function to print.
 * @param indent_level The current indentation level for pretty printing.
 */
void tac_print_function(StringBuffer *sb, const TacFunction* function, int indent_level);

/**
 * @brief Prints an entire TAC program to a StringBuffer.
 * @param sb The StringBuffer to print to.
 * @param program The program to print.
 */
void tac_print_program(StringBuffer *sb, const TacProgram* program);

#endif // CLERIC_TAC_H
