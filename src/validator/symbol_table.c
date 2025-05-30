#include "symbol_table.h"
#include "../memory/arena.h" // For arena_alloc
#include "../strings/strings.h" // For arena_strdup
#include <string.h> // For strcmp, memcpy
#include <stdio.h>  // For NULL (though stddef.h is better), and potential debug fprintf

#define INITIAL_SCOPE_CAPACITY 4
#define INITIAL_SYMBOL_CAPACITY 8

void symbol_table_init(SymbolTable *st, Arena *arena) {
    st->arena = arena;
    st->scopes = NULL;
    st->scope_count = 0;
    st->scope_capacity = 0;

    // Initialize with a global scope
    if (!symbol_table_enter_scope(st)) {
        // This is a critical failure, usually indicates arena out of memory at startup
        // In a real compiler, might need more robust error handling or panic
        fprintf(stderr, "Critical: Failed to initialize global scope for symbol table.\n");
    }
}

void symbol_table_free(SymbolTable *st) {
    // All memory is managed by the arena.
    // We can reset the structure's state if it might be reused with a new arena,
    // but typically the arena itself is freed, taking all this with it.
    st->scopes = NULL; // Dereference to be safe if st itself isn't immediately destroyed
    st->scope_count = 0;
    st->scope_capacity = 0;
    // st->arena remains, as it's owned externally
}

bool symbol_table_enter_scope(SymbolTable *st) {
    if (st->scope_count >= st->scope_capacity) {
        const int new_capacity = st->scope_capacity == 0 ? INITIAL_SCOPE_CAPACITY : st->scope_capacity * 2;
        Scope *new_scopes_array = arena_alloc(st->arena, new_capacity * sizeof(Scope));
        if (!new_scopes_array) return false; // Arena allocation failed

        if (st->scopes) {
            memcpy(new_scopes_array, st->scopes, st->scope_count * sizeof(Scope));
            // Old st->scopes block is still in the arena, which is fine.
        }
        st->scopes = new_scopes_array;
        st->scope_capacity = new_capacity;
    }

    Scope *new_scope = &st->scopes[st->scope_count];
    new_scope->symbols = NULL;
    new_scope->symbol_count = 0;
    new_scope->symbol_capacity = 0;

    st->scope_count++;
    return true;
}

void symbol_table_exit_scope(SymbolTable *st) {
    if (st->scope_count > 0) {
        // Should not exit global scope if only 1, but allow for now
        // Memory for the scope and its symbols remains in the arena.
        // If arena had a per-scope sub-allocator or stack marker, it could be reset here.
        st->scope_count--;
    }
}

const Symbol *symbol_table_lookup_symbol_in_current_scope(const SymbolTable *st, const char *name) {
    if (st->scope_count == 0) return NULL;

    const Scope *current_scope = &st->scopes[st->scope_count - 1];
    for (int i = 0; i < current_scope->symbol_count; ++i) {
        if (strcmp(current_scope->symbols[i].name, name) == 0) {
            return &current_scope->symbols[i];
        }
    }
    return NULL;
}

bool symbol_table_add_symbol(const SymbolTable *st, const char *name, const Token declaration_token,
                             const int tac_temp_id, const char *decorated_name) {
    if (st->scope_count == 0) return false; // Should not happen if global scope is always present

    Scope *current_scope = &st->scopes[st->scope_count - 1];

    // Check for re-declaration in the current scope
    if (symbol_table_lookup_symbol_in_current_scope(st, name)) {
        return false; // Re-declaration error
    }

    if (current_scope->symbol_count >= current_scope->symbol_capacity) {
        const int new_capacity = current_scope->symbol_capacity == 0
                                     ? INITIAL_SYMBOL_CAPACITY
                                     : current_scope->symbol_capacity * 2;
        Symbol *new_symbols_array = arena_alloc(st->arena, new_capacity * sizeof(Symbol));
        if (!new_symbols_array) return false; // Arena allocation failed

        if (current_scope->symbols) {
            memcpy(new_symbols_array, current_scope->symbols, current_scope->symbol_count * sizeof(Symbol));
            // Old current_scope->symbols block is still in the arena.
        }
        current_scope->symbols = new_symbols_array;
        current_scope->symbol_capacity = new_capacity;
    }

    Symbol *new_symbol = &current_scope->symbols[current_scope->symbol_count];
    new_symbol->name = arena_strdup(st->arena, name);
    if (!new_symbol->name) return false; // arena_strdup failed

    new_symbol->tac_temp_id = tac_temp_id;
    new_symbol->decorated_name = (char *) decorated_name; // Store the pointer, validator owns allocation via arena

    current_scope->symbol_count++;
    return true;
}

const Symbol *symbol_table_lookup_symbol(const SymbolTable *st, const char *name) {
    for (int i = st->scope_count - 1; i >= 0; --i) {
        const Scope *scope = &st->scopes[i];
        for (int j = 0; j < scope->symbol_count; ++j) {
            if (strcmp(scope->symbols[j].name, name) == 0) {
                return &scope->symbols[j];
            }
        }
    }
    return NULL;
}
