#include "lowering.hh"
#include <algorithm>
#include <limits>

namespace baka {
    namespace ir {
        [[noreturn]] void Lowerer::fail(const types::ASTNode* node, std::string message) {
            auto span = node ? node->getSourceSpan() : types::ASTSourceSpan{};
            Diagnostics.push_back(
                {std::move(message),
                 {span.Valid ? span.Line : State.Source.line, span.Valid ? span.Col : State.Source.column}});
            throw Failure(Diagnostics.back().message);
        }

        TypeId Lowerer::importType(semantic::TypeId id) {
            id = Analyzer.types().unConst(id);
            if (!id.value) {
                return 0;
            }
            if (auto it = TypeMap.find(id.value); it != TypeMap.end()) {
                return it->second;
            }
            const auto& source = Analyzer.types().get(id);
            Type target;
            auto layout = Analyzer.types().layout(id);
            if (layout) {
                target.complete = true;
                target.size = layout.layout.size;
                target.alignment = layout.layout.alignment;
                if (source.kind == semantic::TypeKind::Struct || source.kind == semantic::TypeKind::Class ||
                    source.kind == semantic::TypeKind::Union) {
                    target.memberOffsets = layout.layout.offsets;
                    target.baseOffset = layout.layout.baseOffset;
                }
            }
            using K = semantic::TypeKind;
            switch (source.kind) {
            case K::Builtin:
            case K::Enum: {
                if (Analyzer.types().isVoid(id)) {
                    target.kind = TypeKind::Void;
                } else {
                    auto arithmetic = Analyzer.types().arithmetic(id);
                    target.kind = arithmetic->floating ? TypeKind::Floating : TypeKind::Integer;
                    target.bits =
                        source.kind == K::Builtin && source.builtin == semantic::Builtin::Bool ? 1 : arithmetic->bits;
                    target.isSigned = arithmetic->isSigned;
                }
                break;
            }
            case K::Pointer: {
                auto result = Result.pointer(importType(source.element));
                TypeMap[id.value] = result;
                return result;
            }
            case K::Array:
                target.kind = TypeKind::Array;
                target.element = importType(source.element);
                target.count = source.bound.count;
                break;
            case K::Function: {
                std::vector<TypeId> parameters;
                for (auto parameter : source.parameters) {
                    parameters.push_back(importType(parameter));
                }
                auto result = Result.function(importType(source.element), std::move(parameters), source.variadic);
                TypeMap[id.value] = result;
                return result;
            }
            case K::Struct:
            case K::Class:
            case K::Union:
                target.kind = TypeKind::Record;
                target.name = source.name;
                target.isUnion = source.kind == K::Union;
                target.memberTypes.resize(target.memberOffsets.size());
                break;
            default:
                fail(nullptr, "cannot import erroneous semantic type into IR");
            }
            // Builtins and enum representation types can share an IR identity;
            // nominal records remain distinct even if their names/layouts match.
            if (target.kind != TypeKind::Record) {
                for (TypeId i = 1; i < Result.types.size(); ++i) {
                    if (Result.types[i] == target) {
                        TypeMap[id.value] = i;
                        return i;
                    }
                }
            }
            Result.types.push_back(std::move(target));
            auto result = static_cast<TypeId>(Result.types.size() - 1);
            TypeMap[id.value] = result;
            if (Result.types[result].kind == TypeKind::Record && Result.types[result].complete) {
                // Publish the nominal identity before importing fields: pointers
                // may refer back to this record, or to mutually recursive records.
                if (source.base.value) {
                    auto base = importType(source.base);
                    Result.types[result].base = base;
                }
                for (size_t i = 0; i < source.members.size(); ++i) {
                    if (Result.types[result].memberOffsets[i]) {
                        auto field = importType(source.members[i].type);
                        // Recursive imports can reallocate Result.types.
                        Result.types[result].memberTypes[i] = field;
                    }
                }
            }
            return result;
        }

        TypeId Lowerer::type(const types::ASTNode* node) {
            return node ? importType({node->semanticInfo().Type}) : 0;
        }

        TypeId Lowerer::integerType() const {
            return TypeMap.at(Analyzer.types().builtin(semantic::Builtin::Long).value);
        }

        Function& Lowerer::function() {
            return Result.functions.at(State.FunctionIndex);
        }

        Block& Lowerer::block() {
            if (State.CurrentBlock && State.CurrentBlock <= function().blocks.size()) {
                auto& result = function().blocks[State.CurrentBlock - 1];
                if (result.id == State.CurrentBlock) {
                    return result;
                }
            }
            throw Failure("IR builder lost current block");
        }

        BlockId Lowerer::newBlock() {
            auto id = State.NextBlock++;
            function().blocks.push_back({id, {}, {}, {}});
            return id;
        }

        void Lowerer::select(BlockId id) {
            State.CurrentBlock = id;
        }

        Operand Lowerer::temporary(TypeId type) {
            auto id = static_cast<uint32_t>(function().temporaries.size());
            function().temporaries.push_back(type);
            return {OperandKind::Temporary, type, id};
        }

        Operand Lowerer::integer(TypeId type, uint64_t value) const {
            const auto& info = Result.type(type);
            if (info.kind == TypeKind::Integer && info.bits < 64) {
                value &= (uint64_t{1} << info.bits) - 1;
            }
            return {OperandKind::Integer, type, 0, value};
        }

        Operand Lowerer::label(BlockId id) const {
            return {OperandKind::Block, 0, id};
        }

        Operand Lowerer::object(uint32_t symbol) {
            auto* object = Result.object(function(), symbol);
            if (!object) {
                fail(nullptr, "IR object has no registered storage");
            }
            return {OperandKind::Object, Result.pointer(object->type), symbol};
        }

        Operand Lowerer::callable(uint32_t symbol) {
            const auto* target = Result.functionById(symbol);
            if (!target) {
                fail(nullptr, "IR call has no selected function declaration");
            }
            return {OperandKind::Function, Result.pointer(target->signature), symbol};
        }

        void Lowerer::emit(Instruction instruction) {
            if (!block().instructions.empty() && IsTerminator(block().instructions.back().opcode)) {
                select(newBlock());
            }
            block().instructions.push_back(std::move(instruction));
        }

        void Lowerer::emit(Opcode opcode, Operand result, Operand left, Operand right) {
            emit(Instruction{opcode, result, left, right, State.Source});
        }

        void Lowerer::emitCall(Operand result, Operand callee, std::vector<Operand> arguments) {
            // Stage only captured values: nested calls and branching argument
            // expressions have already finished before this contiguous sequence.
            for (auto argument : arguments) {
                emit(Opcode::Param, {}, argument);
            }
            emit(Instruction::call(result, callee, State.Source));
        }

        void Lowerer::store(Operand address, Operand value) {
            emit(Instruction::store(address, value, State.Source));
        }

        Operand Lowerer::emitValue(Opcode opcode, TypeId type, Operand left, Operand right) {
            auto result = temporary(type);
            emit(opcode, result, left, right);
            return result;
        }

        void Lowerer::jump(BlockId destination) {
            emit(Opcode::Jump, {}, label(destination));
        }

        void Lowerer::branch(Operand condition, BlockId yes, BlockId no) {
            emit(Opcode::JumpIf, {}, condition, label(yes));
            jump(no);
        }

        void Lowerer::link(BlockId destination) {
            if (block().instructions.empty() || !IsTerminator(block().instructions.back().opcode)) {
                jump(destination);
            }
        }

        Operand Lowerer::cast(Operand value, TypeId target) {
            if (!target || Result.type(target).kind == TypeKind::Void) {
                return {};
            }
            if (value.type == target) {
                return value;
            }
            if (Result.type(value.type).kind == TypeKind::Record && Result.type(target).kind == TypeKind::Record) {
                // Semantic aggregate initialization also permits derived-to-base
                // slicing. Materialize the captured value, then load just its base
                // subobject. This remains a typed value operation, independent of ABI.
                semantic::TypeId source, destination;
                for (auto [original, imported] : TypeMap) {
                    if (imported == value.type) {
                        source = Analyzer.types().unConst({original});
                    }
                    if (imported == target) {
                        destination = Analyzer.types().unConst({original});
                    }
                }
                uint64_t offset = 0;
                while (source.value && source != destination) {
                    const auto& current = Analyzer.types().get(source);
                    auto baseOffset = Result.type(importType(source)).baseOffset;
                    if (!current.base.value || !baseOffset) {
                        fail(nullptr, "record conversion lacks a compatible base subobject");
                    }
                    offset += *baseOffset;
                    source = Analyzer.types().unConst(current.base);
                }
                if (!source.value || !destination.value) {
                    fail(nullptr, "record conversion lacks semantic type metadata");
                }
                Object storage;
                storage.id = NextObject++;
                storage.name = ".slice";
                storage.type = value.type;
                auto id = storage.id;
                function().objects.push_back(std::move(storage));
                store(object(id), value);
                auto base = emitValue(Opcode::AddressOffset, Result.pointer(target), object(id),
                                      integer(integerType(), offset));
                return load(base, target);
            }
            return emitValue(Opcode::Cast, target, value);
        }

        Operand Lowerer::load(Operand address, TypeId target) {
            auto kind = Result.type(target).kind;
            if (kind == TypeKind::Array) {
                return cast(address, Result.pointer(Result.type(target).element));
            }
            if (kind == TypeKind::Function) {
                return address;
            }
            // A record load captures an independent typed value, just as a scalar
            // load does. Its eventual register/memory ABI representation is deferred.
            return emitValue(Opcode::Load, target, cast(address, Result.pointer(target)));
        }

        void Lowerer::importSymbols() {
            for (uint32_t i = 1; i <= Analyzer.types().typeCount(); ++i) {
                importType({i});
            }
            for (uint32_t i = 1; i <= Analyzer.types().typeCount(); ++i) {
                const auto& info = Analyzer.types().get({i});
                if (!Analyzer.types().isAggregate({i}) || info.kind == semantic::TypeKind::Const) {
                    continue;
                }
                for (size_t index = 0; index < info.members.size(); ++index) {
                    const auto& member = info.members[index];
                    if (member.symbol.value) {
                        Members[member.symbol.value] = {i, static_cast<uint32_t>(index + 1), member.isStatic};
                    }
                }
            }
            for (const auto& [source, target] : TypeMap) {
                ReverseTypes.try_emplace(target, semantic::TypeId{source});
            }
            NextObject = Analyzer.symbols().symbolCount() + 1;
            using S = semantic::SymbolKind;
            for (uint32_t i = 1; i <= Analyzer.symbols().symbolCount(); ++i) {
                const auto& symbol = Analyzer.symbols().get({i});
                if (symbol.kind == S::Function || symbol.kind == S::BuiltinFunction) {
                    const auto& signature = Analyzer.types().get(symbol.type);
                    auto imported = importType(symbol.type);
                    auto binding = Members.find(i);
                    if (binding != Members.end() && !binding->second.isStatic) {
                        auto parameters = Result.type(imported).parameters;
                        parameters.insert(parameters.begin(), Result.pointer(importType({binding->second.owner})));
                        imported =
                            Result.function(importType(signature.element), std::move(parameters), signature.variadic);
                    }
                    Function target;
                    target.id = i;
                    target.name = symbol.name;
                    target.signature = imported;
                    if (binding != Members.end()) {
                        target.name = Analyzer.types().get({binding->second.owner}).name + "." + target.name;
                    }
                    target.internal = symbol.linkage == semantic::Linkage::Internal;
                    Result.functions.push_back(std::move(target));
                } else if (symbol.kind == S::Object) {
                    auto scope = Analyzer.symbols().scope(symbol.scope).kind;
                    if (scope == semantic::ScopeKind::Aggregate && !symbol.isStatic) {
                        continue;
                    }
                    if (scope != semantic::ScopeKind::Global && !symbol.isStatic) {
                        continue;
                    }
                    Object target;
                    target.id = i;
                    target.name = symbol.name;
                    target.type = importType(symbol.type);
                    if (auto binding = Members.find(i); binding != Members.end()) {
                        target.name = Analyzer.types().get({binding->second.owner}).name + "." + target.name;
                    }
                    target.global = true;
                    target.internal = symbol.isStatic || symbol.linkage == semantic::Linkage::Internal;
                    target.readOnly = Analyzer.types().isConst(symbol.type);
                    Result.globals.push_back(std::move(target));
                }
            }
        }

        void Lowerer::collectBody(const types::StructBodyNode* body) {
            if (!body) {
                return;
            }
            for (auto* declaration : body->getDeclarations()) {
                if (declaration->getIsStatic()) {
                    declarations(declaration, true);
                }
            }
            for (auto* function : body->getFunctions()) {
                collect(function);
            }
        }

        void Lowerer::collect(const types::ASTNode* node) {
            if (auto* function = dynamic_cast<const types::FunctionNode*>(node)) {
                lowerFunction(function);
            } else if (auto* declaration = dynamic_cast<const types::DeclarationList*>(node)) {
                declarations(declaration, true);
            } else if (auto* definition = dynamic_cast<const types::StructDefinitionNode*>(node)) {
                collectBody(definition->getBody());
            } else if (auto* definition = dynamic_cast<const types::ClassDefinitionNode*>(node)) {
                collectBody(definition->getBody());
            } else if (auto* definition = dynamic_cast<const types::UnionNode*>(node)) {
                collectBody(definition->getBody());
            }
        }

        bool Lowerer::lower(const types::ProgramNode& program) {
            if (Started) {
                Diagnostics.push_back({"an IR lowerer can only be used once", {}});
                return false;
            }
            Started = true;
            if (!Analyzer.checked(program)) {
                Diagnostics.push_back({"IR lowering requires successful semantic analysis", {}});
                return false;
            }
            try {
                importSymbols();
                importLifetimes();
                for (auto* item : program.getBody()) {
                    collect(item);
                }
                lowerLifetimes();
                lowerGlobalInitializers();
                for (auto& function : Result.functions) {
                    BuildCFG(function);
                }
                auto errors = Verify(Result);
                Diagnostics.insert(Diagnostics.end(), errors.begin(), errors.end());
                if (!Diagnostics.empty()) {
                    return false;
                }
            } catch (const Failure&) {
                if (Diagnostics.empty()) {
                    Diagnostics.push_back({"IR lowering failed", State.Source});
                }
            }
            return Diagnostics.empty();
        }
    }
}
