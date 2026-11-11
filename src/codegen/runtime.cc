#include "internal.hh"
#include <ostream>

namespace baka {
    namespace codegen {
        namespace {
            // Every call goes through $t9 so glibc's PIC prologues can derive $gp.
            void Call(std::ostream& output, const std::string& symbol) {
                output << "\tdla\t$t9, " << symbol << "\n\tjalr\t$t9\n\tnop\n";
            }

            bool Declares(const ir::Module& module, const std::string& name) {
                for (const auto& function : module.functions) {
                    if (!function.defined && function.name == name) {
                        return true;
                    }
                }
                return false;
            }

            // __baka_start(n, argv): run the initializer, then the user's main with n
            // (and argv for `main(int argc, char** argv)`). $s0/$s1 keep both alive.
            void PrintStart(const ir::Module& module, std::ostream& output) {
                const auto* user = UserMain(module);
                const auto* initializer = module.initializer ? module.functionById(module.initializer) : nullptr;
                output << "\t.globl\t__baka_start\n\t.p2align\t3\n\t.type\t__baka_start, @function\n__baka_start:\n"
                       << "\tdaddiu\t$sp, $sp, -32\n\tsd\t$ra, 16($sp)\n\tsd\t$s0, 8($sp)\n\tsd\t$s1, 0($sp)\n"
                       << "\tmove\t$s0, $a0\n\tmove\t$s1, $a1\n";
                if (initializer) {
                    Call(output, FunctionSymbol(*initializer));
                }
                if (user) {
                    const auto parameters = module.type(user->signature).parameters.size();
                    if (parameters > 0) {
                        output << "\tmove\t$a0, $s0\n";
                    }
                    if (parameters > 1) {
                        output << "\tmove\t$a1, $s1\n";
                    }
                    Call(output, FunctionSymbol(*user));
                    // Results cross calls sign-extended, but a narrower or void main must not leak junk.
                    output << (module.type(module.type(user->signature).element).kind == ir::TypeKind::Void
                                   ? "\tmove\t$v0, $zero\n"
                                   : "\tsll\t$v0, $v0, 0\n");
                } else {
                    output << "\tmove\t$v0, $zero\n";
                }
                output << "\tld\t$ra, 16($sp)\n\tld\t$s0, 8($sp)\n\tld\t$s1, 0($sp)\n\tjr\t$ra\n\tdaddiu\t$sp, $sp, 32\n"
                       << "\t.size\t__baka_start, .-__baka_start\n";
            }

            // C entry point; weak so a test harness can provide its own `main`. A user
            // `main(int argc, char** argv)` receives the real command line; a
            // `main(int n)` receives n = atoi(argv[1]) (0 without an argument).
            void PrintMain(const ir::Module& module, std::ostream& output) {
                output << "\t.weak\tmain\n\t.p2align\t3\n\t.type\tmain, @function\nmain:\n";
                const auto* user = UserMain(module);
                if (user && module.type(user->signature).parameters.size() > 1) {
                    output << "\tdla\t$t9, __baka_start\n\tjr\t$t9\n\tnop\n\t.size\tmain, .-main\n";
                    return;
                }
                output << "\tdaddiu\t$sp, $sp, -16\n\tsd\t$ra, 8($sp)\n\tslti\t$t0, $a0, 2\n"
                       << "\tbne\t$t0, $zero, .Lbaka.main.run\n\tmove\t$a0, $zero\n\tld\t$a0, 8($a1)\n";
                Call(output, "atoi");
                output << "\tmove\t$a0, $v0\n.Lbaka.main.run:\n";
                Call(output, "__baka_start");
                output << "\tld\t$ra, 8($sp)\n\tjr\t$ra\n\tdaddiu\t$sp, $sp, 16\n\t.size\tmain, .-main\n";
            }

            // Same contract as src/ir/heap_runtime.cc: null for a bad alignment or
            // failure, zero bytes allocate one byte, over-aligned requests use
            // posix_memalign (which also rejects size overflow), others malloc.
            void PrintAllocate(std::ostream& output) {
                output << "\t.globl\t__baka_allocate\n\t.p2align\t3\n\t.type\t__baka_allocate, @function\n"
                       << "__baka_allocate:\n\tdaddiu\t$sp, $sp, -32\n\tsd\t$ra, 24($sp)\n"
                       << "\tbeq\t$a1, $zero, .Lbaka.alloc.null\n\tnop\n"
                       << "\tdaddiu\t$t0, $a1, -1\n\tand\t$t0, $t0, $a1\n"
                       << "\tbne\t$t0, $zero, .Lbaka.alloc.null\n\tnop\n"
                       << "\tsltiu\t$t0, $a0, 1\n\tdaddu\t$a0, $a0, $t0\n"
                       << "\tsltiu\t$t0, $a1, 17\n\tbne\t$t0, $zero, .Lbaka.alloc.malloc\n\tnop\n"
                       << "\tmove\t$a2, $a0\n\tsd\t$zero, 0($sp)\n\tdaddiu\t$a0, $sp, 0\n";
                Call(output, "posix_memalign");
                output << "\tbne\t$v0, $zero, .Lbaka.alloc.null\n\tnop\n\tb\t.Lbaka.alloc.done\n\tld\t$v0, 0($sp)\n"
                       << ".Lbaka.alloc.malloc:\n";
                Call(output, "malloc");
                output << "\tb\t.Lbaka.alloc.done\n\tnop\n.Lbaka.alloc.null:\n\tmove\t$v0, $zero\n"
                       << ".Lbaka.alloc.done:\n\tld\t$ra, 24($sp)\n\tjr\t$ra\n\tdaddiu\t$sp, $sp, 32\n"
                       << "\t.size\t__baka_allocate, .-__baka_allocate\n";
            }

            void PrintDeallocate(std::ostream& output) {
                output << "\t.globl\t__baka_deallocate\n\t.p2align\t3\n\t.type\t__baka_deallocate, @function\n"
                       << "__baka_deallocate:\n\tdla\t$t9, free\n\tjr\t$t9\n\tnop\n"
                       << "\t.size\t__baka_deallocate, .-__baka_deallocate\n";
            }
        }

        void PrintRuntime(const ir::Module& module, std::ostream& output) {
            output << "\t.text\n";
            PrintStart(module, output);
            PrintMain(module, output);
            if (Declares(module, "__baka_allocate")) {
                PrintAllocate(output);
            }
            if (Declares(module, "__baka_deallocate")) {
                PrintDeallocate(output);
            }
        }
    }
}
