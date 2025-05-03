#ifndef CLERIC_TAC_H
#define CLERIC_TAC_H

#include <stddef.h> // For size_t
#include <stdint.h> // For int values (though standard int might suffice)

#include "../memory/arena.h" // For arena allocation

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
    // Future instructions: ADD, SUB, MUL, DIV, GOTO, IFZ, CALL, PARAM, LABEL, etc.
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
        // Future: struct { TacOperand dst; TacOperand src1; TacOperand src2; } binary_op;
        // Future: struct { const char* label; } label;
        // Future: struct { const char* label; TacOperand condition; } conditional_jump; // e.g., ifz condition goto label
        // Future: struct { const char* label; } unconditional_jump; // goto label
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

// Function and Program manipulation
TacFunction* create_tac_function(const char* name, Arena* arena);
void add_instruction_to_function(TacFunction* func, const TacInstruction* instr, Arena* arena);

TacProgram* create_tac_program(Arena* arena);
void add_function_to_program(TacProgram* prog, TacFunction* func, Arena* arena);

#endif // CLERIC_TAC_H
