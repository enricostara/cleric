#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "driver.h"
#include "../strings/strings.h" // Include StringBuffer header
#include "../files/files.h"
#include "../memory/arena.h" // Include Arena header
#include "compiler.h" // Added: Include new compiler header

/**
 * Runs the gcc preprocessor on the input file and writes output to .i file.
 * Returns 0 on success, 1 on failure.
 */
int run_preprocessor(const char *input_file) {
    // Use utility to check extension
    if (!filename_has_ext(input_file, ".c")) {
        fprintf(stderr, "Input file should have a .c extension\n");
        return 1;
    }

    char output_file[1024];
    if (!filename_replace_ext(input_file, ".i", output_file, sizeof(output_file))) {
        fprintf(stderr, "Failed to construct .i filename for %s\n", input_file);
        return 1;
    }

    char command[2048];
    snprintf(command, sizeof(command), "gcc -E -P %s -o %s", input_file, output_file);
    const int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Failed to preprocess %s\n", input_file);
        return 1;
    }
    printf("Preprocessed output written to %s\n", output_file);
    return 0;
}

// Function: compilation from .i file to .s file (using the core compiler logic)
int run_compiler(const char *input_file, const bool lex_only, const bool parse_only, const bool irgen_only,
                 const bool codegen_only) {
    // Use utility to check extension
    if (!filename_has_ext(input_file, ".i")) {
        fprintf(stderr, "Error: Input file '%s' does not have '.i' extension.\n", input_file);
        return 1;
    }

    char output_file[1024];
    if (!filename_replace_ext(input_file, ".s", output_file, sizeof(output_file))) {
        fprintf(stderr, "Failed to construct .s filename for %s\n", input_file);
        return 1;
    }

    long f_size = 0;
    char *src = read_entire_file(input_file, &f_size);
    if (!src) {
        fprintf(stderr, "Error reading input file '%s'.\n", input_file);
        free(src); // Free the buffer from read_file (though it should be NULL)
        return 1;
    }

    // Create the main arena for this compilation run
    Arena main_arena = arena_create(1024 * 1024); // Example size: 1MB
    if (!main_arena.start) {
        fprintf(stderr, "Driver Error: Failed to create main arena.\n");
        return 1; // Indicate failure
    }

    StringBuffer sb;
    string_buffer_init(&sb, &main_arena, 1024); // Initialize buffer for core function output
    // ReSharper disable once CppDFAUnusedValue
    int result = 1; // Default to error

    const bool core_success = compile(src, lex_only, parse_only, irgen_only, codegen_only, &sb, &main_arena);

    if (core_success) {
        // If only lexing, parsing, irgen or codegen-to-stdout was requested, we are done successfully.
        if (lex_only || parse_only || irgen_only || codegen_only) {
            result = 0;
        } else {
            // Full compilation succeeded, write assembly to file
            printf("Writing assembly code to %s...\n", output_file);
            const char *assembly_code = string_buffer_content_str(&sb); // Use read-only access
            if (!write_string_to_file(output_file, assembly_code)) {
                fprintf(stderr, "Failed to write assembly to %s\n", output_file);
                remove(output_file); // Attempt removal
                result = 1; // Mark as failure
            } else {
                printf("Assembly code written to %s\n", output_file);
                // Remove intermediate .i file only on full success
                if (remove(input_file) != 0) {
                    fprintf(stderr, "Warning: could not remove intermediate file %s\n", input_file);
                }
                result = 0; // Mark as success
            }
        }
    } else {
        // Core compilation failed, error messages already printed by core function
        // Make sure output file is removed if it exists from a previous run
        remove(output_file);
        result = 1;
    }

    // --- Cleanup ---
    free(src); // Free source code buffer
    arena_destroy(&main_arena); // Destroy the arena

    return result;
}

// Final step: assemble and link .s to executable, then remove .s if successful
int run_assembler_linker(const char *input_file) {
    // Use utility to check extension
    if (!filename_has_ext(input_file, ".s")) {
        fprintf(stderr, "Input file should have a .s extension\n");
        return 1;
    }

    char output_file[1024];
    // Use empty string to remove the extension
    if (!filename_replace_ext(input_file, "", output_file, sizeof(output_file))) {
        fprintf(stderr, "Failed to construct output executable filename for %s\n", input_file);
        return 1;
    }

    char command[2048];
    snprintf(command, sizeof(command), "gcc %s -o %s", input_file, output_file);
    const int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Failed to assemble/link %s\n", input_file);
        return 1;
    }
    // Remove the .s file if all is fine
    if (remove(input_file) != 0) {
        fprintf(stderr, "Warning: could not remove %s\n", input_file);
    }
    printf("Assembled and linked output: %s\n", output_file);
    return 0;
}

// -----------------------------------------------------------------------------
// Core Compilation Logic (Source String -> Assembly String Buffer)
// -----------------------------------------------------------------------------
// The run_compiler_core function has been moved to src/driver/compiler.c
// and renamed to compile().
