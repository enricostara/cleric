#ifndef CODEGEN_H
#define CODEGEN_H

#include "../ir/tac.h"          // Include TAC definitions (TacProgram)
#include "../strings/strings.h" // Include StringBuffer definition
#include <stdbool.h>      // For bool return type
#include <stddef.h>       // For size_t type

/**
 * Generates assembly code (x86-64 macOS AT&T syntax) from TAC.
 *
 * @param tac_program The Three-Address Code program.
 * @param sb Pointer to the StringBuffer to store the generated assembly code.
 * @return true if code generation was successful, false otherwise.
 */
bool codegen_generate_program(TacProgram *tac_program, StringBuffer *sb);

/**
 * Converts a TAC operand to its assembly string representation.
 *
 * @param op The TAC operand to convert.
 * @param out_buffer The character buffer to write the assembly string into.
 * @param buffer_size The size of the output buffer.
 * @return true if the conversion was successful and the string fits, false otherwise.
 */
bool operand_to_assembly_string(const TacOperand *op, char *out_buffer, size_t buffer_size);

// --- Helper function (exposed for testing) ---
/**
 * Calculates the maximum temporary ID used in a function.
 *
 * @param func Pointer to the TacFunction to analyze.
 * @return The highest temporary ID used, or -1 if no temporaries are found or if func is NULL.
 */
int calculate_max_temp_id(const TacFunction *func);

#endif // CODEGEN_H
