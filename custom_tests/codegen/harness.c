/* Test driver linked with every generated .s. Its strong `main` overrides the
 * weak one emitted by the backend. The program's own stdout is followed by two
 * marker lines, \x01result=<int> and (when a global is observed) \x01global=<int>.
 *
 * The lines are printed from an atexit handler registered BEFORE __baka_start,
 * so (atexit being LIFO) it runs after every static destructor the program
 * registers through __cxa_atexit, and nothing the program prints can follow it.
 *
 * The observed global is selected at compile time by run.py, because assembly
 * symbols such as `trace.20` are not C identifiers:
 *   -DBAKA_GLOBAL_SYM='"trace.20"' -DBAKA_GLOBAL_TYPE='int'
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int __baka_start(int argc, char** argv);

#ifdef BAKA_GLOBAL_SYM
extern char baka_global[] __asm__(BAKA_GLOBAL_SYM);
#endif

static int result;

static void report(void) {
    printf("\x01result=%d\n", result);
#ifdef BAKA_GLOBAL_SYM
    BAKA_GLOBAL_TYPE value;
    memcpy(&value, baka_global, sizeof value);
    printf("\x01global=%lld\n", (long long)value);
#endif
    fflush(stdout);
}

int main(int argc, char** argv) {
    atexit(report);
    result = __baka_start(argc, argv);
    return 0;
}
