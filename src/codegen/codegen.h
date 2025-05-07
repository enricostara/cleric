#ifndef CODEGEN_H
#define CODEGEN_H

#include "../ir/tac.h"          // Include TAC definitions (TacProgram)
#include "../strings/strings.h" // Include StringBuffer definition
#include <stdbool.h>      // For bool return type

/**
 * Generates assembly code (x86-64 macOS AT&T syntax) from TAC.
 *
 * @param tac_program The Three-Address Code program.
 * @param sb Pointer to the StringBuffer to store the generated assembly code.
 * @return true if code generation was successful, false otherwise.
 */
bool codegen_generate_program(TacProgram *tac_program, StringBuffer *sb);

#endif // CODEGEN_H
