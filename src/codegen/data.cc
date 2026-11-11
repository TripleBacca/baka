#include "internal.hh"
#include <algorithm>
#include <bit>
#include <cstring>
#include <map>
#include <ostream>

namespace baka {
    namespace codegen {
        namespace {
            unsigned Log2(uint64_t value) {
                return value > 1 ? std::bit_width(value - 1) : 0;
            }

            uint64_t ScalarSize(const ir::Module& module, ir::TypeId id) {
                const auto& type = module.type(id);
                return type.kind == ir::TypeKind::Pointer ? 8 : std::max<uint64_t>(1, (type.bits + 7) / 8);
            }

            // Emits one object's contents: literal bytes, then each initial value
            // overriding its range. Relocations are the only items that cannot be
            // folded into bytes, so they are kept aside and printed as `.dword`.
            void PrintContents(const ir::Module& module, const ir::Object& object, uint64_t size,
                               std::ostream& output) {
                std::vector<uint8_t> image(size, 0);
                std::copy_n(object.bytes.begin(), std::min<uint64_t>(object.bytes.size(), size), image.begin());
                std::map<uint64_t, std::string> relocations;
                for (const auto& [offset, value] : object.initialValues) {
                    auto store = [&](uint64_t bits, uint64_t width) {
                        for (uint64_t i = 0; i < width && offset + i < size; ++i) {
                            image[offset + i] = static_cast<uint8_t>(bits >> (8 * i));
                        }
                    };
                    switch (value.kind) {
                    case ir::OperandKind::Integer:
                        store(value.integer, ScalarSize(module, value.type));
                        break;
                    case ir::OperandKind::Floating: {
                        if (module.type(value.type).bits == 32) {
                            auto f = static_cast<float>(value.floating);
                            uint32_t bits;
                            std::memcpy(&bits, &f, 4);
                            store(bits, 4);
                        } else {
                            auto d = static_cast<double>(value.floating);
                            uint64_t bits;
                            std::memcpy(&bits, &d, 8);
                            store(bits, 8);
                        }
                        break;
                    }
                    case ir::OperandKind::Object:
                    case ir::OperandKind::Function: {
                        std::string symbol;
                        if (value.kind == ir::OperandKind::Function) {
                            const auto* function = module.functionById(value.id);
                            if (!function) {
                                throw Unsupported("initializer refers to an unknown function");
                            }
                            symbol = FunctionSymbol(*function);
                        } else {
                            auto found = std::find_if(module.globals.begin(), module.globals.end(),
                                                      [&](const ir::Object& g) { return g.id == value.id; });
                            if (found == module.globals.end()) {
                                throw Unsupported("initializer refers to an unknown global");
                            }
                            symbol = ObjectSymbol(*found);
                        }
                        auto addend = static_cast<int64_t>(value.integer);
                        if (addend) {
                            symbol += (addend > 0 ? "+" : "-") + std::to_string(addend > 0 ? addend : -addend);
                        }
                        relocations[offset] = symbol;
                        break;
                    }
                    default:
                        throw Unsupported("unsupported initializer operand");
                    }
                }
                // A relocation wins over any bytes that overlap it (later stores to the
                // same offset were already merged into the map by overwriting).
                for (uint64_t i = 0; i < size;) {
                    if (auto it = relocations.find(i); it != relocations.end() && i + 8 <= size) {
                        output << "\t.dword\t" << it->second << "\n";
                        i += 8;
                        continue;
                    }
                    uint64_t end = i;
                    while (end < size && !(relocations.count(end) && end + 8 <= size)) {
                        ++end;
                    }
                    if (std::all_of(image.begin() + i, image.begin() + end, [](uint8_t b) { return b == 0; })) {
                        output << "\t.zero\t" << end - i << "\n";
                    } else {
                        for (uint64_t run = i; run < end;) {
                            auto stop = std::min<uint64_t>(end, run + 16);
                            output << "\t.byte\t";
                            for (auto j = run; j < stop; ++j) {
                                output << (j > run ? ", " : "") << static_cast<unsigned>(image[j]);
                            }
                            output << "\n";
                            run = stop;
                        }
                    }
                    i = end;
                }
            }
        }

        void PrintData(const ir::Module& module, const LiteralPool& pool, std::ostream& output) {
            for (const auto& object : module.globals) {
                if (object.external) {
                    continue;
                }
                const auto& type = module.type(object.type);
                auto size = std::max<uint64_t>({type.size, object.bytes.size(), 1});
                auto symbol = ObjectSymbol(object);
                bool zero = object.initialValues.empty() &&
                            std::all_of(object.bytes.begin(), object.bytes.end(), [](char c) { return c == 0; });
                output << (object.readOnly && !object.runtimeInitialized ? "\t.section\t.rodata\n"
                           : zero                                         ? "\t.bss\n"
                                                                          : "\t.data\n");
                if (!object.internal) {
                    output << "\t.globl\t" << symbol << "\n";
                }
                output << "\t.p2align\t" << Log2(type.alignment) << "\n\t.type\t" << symbol << ", @object\n\t.size\t"
                       << symbol << ", " << size << "\n" << symbol << ":\n";
                if (zero) {
                    output << "\t.zero\t" << size << "\n";
                } else {
                    PrintContents(module, object, size, output);
                }
            }
            if (!pool.labels.empty()) {
                output << "\t.section\t.rodata\n";
                for (const auto& [key, label] : pool.labels) {
                    output << "\t.p2align\t3\n" << label << ":\n"
                           << (key.first == 32 ? "\t.word\t0x" : "\t.dword\t0x") << std::hex << key.second << std::dec
                           << "\n";
                }
            }
            output << "\t.text\n";
        }
    }
}
