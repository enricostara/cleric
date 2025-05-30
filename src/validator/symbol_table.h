#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>
#include "../memory/arena.h"
#include "../lexer/lexer.h" // For Token

// Represents a declared symbol (e.g., a variable)
typedef struct {
    char *name;                // Name of the symbol
    // Future: Add data type information here, e.g., DataType type;

    // --- For TAC Generation ---
    int tac_temp_id;           // Unique ID for the TAC temporary representing this symbol
    char *decorated_name;      // Decorated name for TAC (e.g., "x.0"), owned by arena
} Symbol;

// Represents a single scope (e.g., a block or function body)
typedef struct {
    Symbol *symbols;           // Dynamic array of symbols in this scope
    int symbol_count;          // Number of symbols in this scope
    int symbol_capacity;       // Capacity of the symbols array
} Scope;

// Represents the symbol table, managing a stack of scopes
typedef struct {
    Scope *scopes;             // Dynamic array acting as a stack of scopes
    int scope_count;           // Number of active scopes (current stack depth)
    int scope_capacity;        // Capacity of the scopes array
    Arena *arena;              // Arena allocator for all symbol table memory
} SymbolTable;

// --- Public Symbol Table Interface ---

/**
 * @brief Initializes a symbol table.
 * @param st Pointer to the SymbolTable to initialize.
 * @param arena Pointer to the memory arena to use for allocations.
 */
void symbol_table_init(SymbolTable *st, Arena *arena);

/**
 * @brief Frees resources associated with the symbol table.
 *        Note: Memory for symbols/scopes themselves is managed by the arena.
 *        This function primarily resets counts and capacities if needed or frees
 *        the top-level 'scopes' array if it wasn't allocated as a single block
 *        from the arena (depends on implementation strategy).
 * @param st Pointer to the SymbolTable to free.
 */
void symbol_table_free(SymbolTable *st); // Typically, arena handles actual memory.

/**
 * @brief Enters a new scope (pushes it onto the scope stack).
 * @param st Pointer to the SymbolTable.
 * @return true if successful, false on allocation failure.
 */
bool symbol_table_enter_scope(SymbolTable *st);

/**
 * @brief Exits the current scope (pops it from the scope stack).
 * @param st Pointer to the SymbolTable.
 */
void symbol_table_exit_scope(SymbolTable *st);

/**
 * @brief Adds a new symbol to the current (topmost) scope.
 *        Checks for re-declaration within the same scope.
 * @param st Pointer to the SymbolTable.
 * @param name The name of the symbol to add.
 * @param declaration_token The token corresponding to this symbol's declaration.
 * @param tac_temp_id The unique TAC temporary ID for this symbol.
 * @param decorated_name The decorated name for TAC output (e.g., "x.0").
 * @return true if the symbol was added successfully, false if it's a re-declaration
 *         in the current scope or if memory allocation failed.
 */
bool symbol_table_add_symbol(const SymbolTable *st, const char *name, Token declaration_token, int tac_temp_id, const char *decorated_name);

/**
 * @brief Looks up a symbol by name, searching from the current scope outwards to global.
 * @param st Pointer to the SymbolTable.
 * @param name The name of the symbol to find.
 * @return Const pointer to the Symbol if found, otherwise NULL.
 */
const Symbol *symbol_table_lookup_symbol(const SymbolTable *st, const char *name);

/**
 * @brief Looks up a symbol by name only in the current (topmost) scope.
 *        Useful for checking for re-declarations before adding a new symbol.
 * @param st Pointer to the SymbolTable.
 * @param name The name of the symbol to find.
 * @return Const pointer to the Symbol if found in the current scope, otherwise NULL.
 */
const Symbol *symbol_table_lookup_symbol_in_current_scope(const SymbolTable *st, const char *name);

#endif // SYMBOL_TABLE_H
