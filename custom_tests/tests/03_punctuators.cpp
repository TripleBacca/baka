// ============================================================================
// Test File: 03_punctuators.cpp
// Purpose: Test all Punctuator TokenTypes specified in types/token/token.hh
// ============================================================================

// LPAREN_ROUND '(', RPAREN_ROUND ')'
// LPAREN_CURLY '{', RPAREN_CURLY '}'
// SEMICOLON ';'
void example_function(int param_a, int param_b) {
    // LPAREN_SQUARE '[', RPAREN_SQUARE ']'
    int array[5];

    // Nested array subscript indexing with square brackets
    array[0] = param_a;
    array[1] = param_b;
    array[2] = array[0] + array[1];

    // Round parentheses for expressions & grouping
    int calculated = ((array[0] * (array[1] + 2)) - (array[2] / 3));

    // Curly braces for block scopes
    {
        int inner_scoped_var = 100;
        calculated = calculated + inner_scoped_var;
    }

    // Empty statement semicolons
    ;
    ;
}

// Initializer lists with curly braces
struct Config {
    int id;
    int values[3];
};

int main() {
    Config cfg = {1, {10, 20, 30}};
    example_function(cfg.id, cfg.values[0]);
    return 0;
}
