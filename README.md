# cleric

A minimal C compiler implemented in C for educational purposes.

## Overview
Cleric demonstrates the basic stages of compiling a subset of C code:
- **Preprocessing**: Uses `gcc -E` to handle includes, macros, and comments, converting `.c` files to `.i` files.
- **Lexing**: Breaks the source code into a stream of tokens.
- **Parsing**: Constructs an Abstract Syntax Tree (AST) from the token stream, verifying the code's structure.
- **Intermediate Representation (IR)**: Converts the AST into Three-Address Code (TAC), a simpler, machine-independent representation. This step facilitates optimizations and simplifies the final code generation.
- **Code Generation**: Translates the TAC into assembly code (currently targeting x86-64 AT&T syntax).
- **Assembly & Linking**: Uses the system assembler (e.g., `as`) and linker (e.g., `ld`) to produce an executable file from the generated assembly.

## Features
- **Modular Design**: Clear separation between lexer, parser, code generator, and compiler driver components.
- **Abstract Syntax Tree (AST)**: Uses an AST to represent the code's structure, built by a parser that understands operator precedence.
- **Robust Parsing & Error Handling**: Implements Precedence Climbing for parsing expressions and provides detailed error messages for syntax issues.
- **Intermediate Stage Inspection**: Allows viewing the output of different stages:
    - `--lex`: Print the token stream.
    - `--parse`: Print the generated Abstract Syntax Tree.
    - `--tac`: Print the generated Three-Address Code.
    - `--codegen`: Print the generated assembly code to standard output.
- **Arena Allocator**: Utilizes a custom arena allocator for efficient memory management across multiple compilation stages. It handles allocations for lexer tokens, Abstract Syntax Tree (AST) nodes, Three-Address Code (TAC) structures, and other dynamic data within the parser and code generation phases, simplifying cleanup and potentially improving allocation performance.
- **Testable Core Logic**: The core compilation pipeline (lexing, parsing, codegen) is handled by the `compile` function, which is testable.
- **Comprehensive Testing**: Includes both unit tests (for individual modules) and integration tests (verifying the core string-to-string compilation).
- **Three-Address Code (TAC)**: Employs TAC as an intermediate representation, simplifying the translation from the high-level AST to low-level assembly, and paving the way for future optimizations.

## Usage

### Build
Cleric uses CMake. Ensure you have `cmake` and a C compiler (like `gcc` or `clang`) installed.

```sh
# Create a build directory
mkdir build
cd build

# Configure the build (use Debug for development/testing)
# Example using clang:
# cmake -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug ..
# Example using default compiler:
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
# Or, use make directly after cmake:
# make

# Executables will be in the build directory: build/cleric and build/test_all
cd .. # Return to project root if desired
```

### Run the Compiler
```sh
./build/cleric [<options>] <input_file.c>
```

**Options:**
- `--lex`: Lex the input, print tokens to stdout, and exit.
- `--parse`: Lex and parse the input, print the AST to stdout, and exit.
- `--tac`  : Lex, parse, and generate Three-Address Code; print TAC to stdout, and exit.
- `--codegen`: Lex, parse, generate TAC, and then assembly; print assembly to stdout, and exit.
- *(No options)*: Run the full pipeline (preprocess, lex, parse, codegen, assemble, link) to create an executable in the same directory as the input file.

**Examples:**
```sh
# View tokens for example.c
./build/cleric --lex examples/example.c

# View AST for example.c
./build/cleric --parse examples/example.c

# View generated Three-Address Code for example.c
./build/cleric --tac examples/example.c

# View generated assembly for example.c
./build/cleric --codegen examples/example.c

# Compile example.c to an executable (named 'example' in the examples/ dir)
./build/cleric examples/example.c
```

## Testing
The project includes unit tests (using the Unity framework) and integration tests.

```sh
# Ensure you've built with Debug configuration for tests
# From the project root directory:
cmake --build build && ./build/test_all

# Or, if you are in the build directory:
# cmake --build . && ./test_all
```

## License
Permissive MIT License (see LICENSE file for details).

---
For educational reference. Contributions and suggestions welcome!
