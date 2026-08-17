
void test_integer_literals() {
    int zero = 0;
    int decimal_val = 42;
    int large_val = 123456789;
    int hex_val = 0x1A3F;
    int octal_val = 0755;
    int binary_val = 0b101010;
}

void test_character_literals() {
    char simple_char = 'a';
    char uppercase_char = 'Z';
    char digit_char = '9';

        char newline = '\n';
    char tab = '\t';
    char backslash = '\\';
    char single_quote = '\'';
    char null_char = '\0';
    char bell = '\a';
    char backspace = '\b';
    char form_feed = '\f';
    char carriage_return = '\r';
    char vertical_tab = '\v';
    char octal_char = '\101'; }

void test_floating_point_literals() {
    double pi = 3.1415926535;
    float standard_float = 0.5f;
    double zero_fp = 0.0;
    double exponent_pos = 1.23e5;
    double exponent_neg = 4.56e-7;
    double exp_cap = 7.89E+2;
}

void test_string_literals() {
    const char* empty_str = "";
    const char* simple_str = "Hello, world!";
    const char* escaped_str = "First Line\nSecond Line\tTabbed\r\"Quotes\" and \\Backslash\\";
    const char* octal_escaped_str = "Octal \060 Test";
}

void test_special_literals() {
        bool flag_true = true;
    bool flag_false = false;

        void* null_pointer = nullptr;

}

int main() {
    test_integer_literals();
    test_character_literals();
    test_floating_point_literals();
    test_string_literals();
    test_special_literals();
    return 0;
}
