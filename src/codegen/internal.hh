#pragma once
#include "ir/ir.hh"
#include "mips.hh"
#include <iosfwd>
#include <stdexcept>
#include <string>

namespace baka {
    namespace codegen {
        // Thrown for IR the backend cannot represent; Generate turns it into a diagnostic.
        struct Unsupported : std::runtime_error {
            using std::runtime_error::runtime_error;
        };

        // Assembly symbol for a function or global. Declared-only (external)
        // entities keep their source name so the linker resolves them against libc.
        // Defined entities keep the IR's unique `name.id` spelling; a leading dot
        // (compiler-generated names such as `.module_init`) becomes `__baka.`.
        std::string FunctionSymbol(const ir::Function& function);
        std::string ObjectSymbol(const ir::Object& object);
        // The user's entry function (`main`) if defined.
        const ir::Function* UserMain(const ir::Module& module);

        // select.cc / allocator.cc: lowers one defined function to complete
        // machine code (prologue, body, epilogue) with physical registers.
        MFunction SelectFunction(const ir::Module& module, const ir::Function& function, LiteralPool& pool);

        // peephole.cc: block-local machine optimizations, then delay-slot filling.
        void Peephole(MFunction& function);

        // printer.cc: text of one function under `.set noreorder`.
        void PrintFunction(const MFunction& function, std::ostream& output);

        // data.cc: .data/.rodata/.bss for module globals plus the literal pool.
        void PrintData(const ir::Module& module, const LiteralPool& pool, std::ostream& output);

        // runtime.cc: `__baka_start`, the weak C `main`, and runtime helpers the
        // module references (`__baka_allocate`, `__baka_deallocate`).
        void PrintRuntime(const ir::Module& module, std::ostream& output);
    }
}
