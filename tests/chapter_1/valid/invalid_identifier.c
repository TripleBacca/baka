/* '1foo' is not a valid token, because identifier can't start with digits. */
int main(void) {
    return 1foo;
}
// this is valid in lex but we will throw in parser
