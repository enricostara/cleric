#ifndef DRIVER_H
#define DRIVER_H

/**
 * Runs the gcc preprocessor on the input file and writes output to .i file.
 * Returns 0 on success, 1 on failure.
 */
int run_preprocessor(const char *input_file);

// Compiles a .i file to a .s file (mocked for now)
int run_compiler(const char *input_file);

#endif // DRIVER_H
