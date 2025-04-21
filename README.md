# cleric

A minimal C compiler implemented in C for educational purposes.

## Overview
Cleric is a simple compiler pipeline that demonstrates the basic stages of compiling C code:
- **Preprocessing** (`gcc -E`): Converts `.c` files to `.i` files.
- **Compilation** (mocked): Converts `.i` files to `.s` (assembly) files, with optional lex-only mode to print tokens.
- **Assembly & Linking**: Produces an executable from `.s` files (unless lex-only mode is enabled).

## Usage
### Build
Cleric uses CMake for cross-platform builds:

```sh
mkdir build && cd build
cmake ..
make
```

### Run the Compiler
```sh
./cleric [--lex] <input_file.c>
```
- `--lex`: Only lex the input and print tokens (no codegen or linking)

### Example
```sh
./cleric --lex example.c
```

## Testing
Unit tests are provided for all major modules:
- Run all tests:
  ```sh
  ./test_cleric
  ```

## License
MIT (or specify your license here)

---
For educational reference. Contributions and suggestions welcome!
