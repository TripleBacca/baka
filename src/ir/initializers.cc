#include "lowering.hh"
#include "constant_folding.hh"
#include "types/parser/ast/castFactorNode.hh"
#include "types/parser/ast/ternary.hh"
#include "types/parser/ast/sizeof.hh"
#include <functional>

namespace baka::ir {
    namespace {
        const types::ExpressionNode* Transparent(const types::ExpressionNode* expression) {
            for (;;) {
                if (auto* list = dynamic_cast<const types::InitializerNode*>(expression);
                    list && !list->isBraced() && list->expressions().size() == 1) {
                    expression = list->expressions().front();
                } else if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression);
                           comma && comma->getExpressions().size() == 1) {
                    expression = comma->getExpressions().front();
                } else {
                    return expression;
                }
            }
        }

        std::optional<std::string> String(const types::ExpressionNode* expression) {
            expression = Transparent(expression);
            auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression);
            if (!primary) {
                return {};
            }
            auto* constant = std::get_if<types::ConstantNode*>(&primary->getExpression());
            if (!constant) {
                return {};
            }
            if (auto* literal = dynamic_cast<const types::ConstantTNode<base::SLString>*>(*constant)) {
                return std::string(literal->getValue());
            }
            if (auto* literal = dynamic_cast<const types::ConstantTNode<std::string>*>(*constant)) {
                return literal->getValue();
            }
            return {};
        }
    }

    std::vector<std::pair<TypeId, uint64_t>> Lowerer::subobjects(TypeId id) {
        auto type = Result.type(id);
        std::vector<std::pair<TypeId, uint64_t>> result;
        if (type.kind == TypeKind::Array) {
            for (uint64_t i = 0; i < type.count; ++i) {
                result.push_back({type.element, i * Result.type(type.element).size});
            }
        } else if (type.kind == TypeKind::Record) {
            semantic::TypeId semanticType;
            for (auto [original, imported] : TypeMap) {
                if (imported == id) {
                    semanticType = {original};
                    break;
                }
            }
            if (!semanticType.value) {
                fail(nullptr, "record initializer lacks a semantic layout");
            }
            auto layout = Analyzer.types().layout(semanticType);
            if (!layout) {
                fail(nullptr, "record initializer has no complete layout");
            }
            const auto& original = Analyzer.types().get(semanticType);
            for (const auto& child : Analyzer.types().subobjects(semanticType)) {
                auto offset =
                    child.memberIndex ? layout.layout.offsets.at(*child.memberIndex) : layout.layout.baseOffset;
                if (!offset) {
                    fail(nullptr, "aggregate initializer member lacks layout offset");
                }
                result.push_back({importType(child.type), *offset});
                if (original.kind == semantic::TypeKind::Union) {
                    break;
                }
            }
        }
        return result;
    }

    std::vector<Lowerer::InitializerEntry> Lowerer::initializerPlan(TypeId target,
                                                                    const types::ExpressionNode* initializer) {
        std::vector<InitializerEntry> result;
        std::function<void(TypeId, uint64_t, const types::ExpressionNode*)> whole;
        std::function<void(TypeId, uint64_t, const std::vector<types::ExpressionNode*>&, size_t&)> consume;
        consume = [&](TypeId type, uint64_t offset, const auto& expressions, size_t& position) {
            if (position == expressions.size()) {
                whole(type, offset, nullptr);
                return;
            }
            auto* expression = Transparent(expressions[position]);
            auto kind = Result.type(type).kind;
            if (dynamic_cast<const types::InitializerNode*>(expression) ||
                (kind != TypeKind::Array && kind != TypeKind::Record) || String(expression)) {
                ++position;
                whole(type, offset, expression);
                return;
            }
            for (auto [child, childOffset] : subobjects(type)) {
                consume(child, offset + childOffset, expressions, position);
            }
        };
        whole = [&](TypeId type, uint64_t offset, const types::ExpressionNode* expression) {
            expression = Transparent(expression);
            const auto info = Result.type(type);
            if (info.kind != TypeKind::Record && info.kind != TypeKind::Array) {
                if (auto* list = dynamic_cast<const types::InitializerNode*>(expression)) {
                    if (list->expressions().size() > 1) {
                        fail(list, "excess scalar initializer in IR");
                    }
                    expression = list->expressions().empty() ? nullptr : list->expressions().front();
                }
                result.push_back({offset, type, expression});
                return;
            }
            auto* list = dynamic_cast<const types::InitializerNode*>(expression);
            auto stringExpression = expression;
            if (list && list->expressions().size() == 1) {
                stringExpression = list->expressions().front();
            }
            if (info.kind == TypeKind::Array && Result.type(info.element).kind == TypeKind::Integer &&
                Result.type(info.element).bits == 8 && String(stringExpression)) {
                // Byte elements are recorded separately by initialize/initializeGlobal.
                result.push_back({offset, type, stringExpression});
                return;
            }
            if (expression && !list) {
                fail(expression, "aggregate copies are not supported by IR yet");
            }
            auto children = subobjects(type);
            size_t position = 0;
            for (auto [child, childOffset] : children) {
                if (list) {
                    consume(child, offset + childOffset, list->expressions(), position);
                } else {
                    whole(child, offset + childOffset, nullptr);
                }
            }
            if (list && position != list->expressions().size()) {
                fail(list, "aggregate initializer could not be lowered completely");
            }
        };
        whole(target, 0, initializer);
        return result;
    }

    void Lowerer::initialize(Operand destination, TypeId target, const types::ExpressionNode* initializer, bool zero) {
        if (!initializer && !zero) {
            return;
        }
        for (const auto& entry : initializerPlan(target, initializer)) {
            auto info = Result.type(entry.type);
            if (info.kind == TypeKind::Array) {
                auto text = *String(entry.expression);
                text.push_back('\0');
                for (uint64_t i = 0; i < info.count; ++i) {
                    auto location = emitValue(Opcode::AddressOffset, Result.pointer(info.element), destination,
                                              integer(integerType(), entry.offset + i));
                    auto byte = i < text.size() ? static_cast<unsigned char>(text[i]) : 0;
                    emit(Opcode::Store, location, integer(info.element, byte));
                }
                continue;
            }
            Operand initial;
            if (entry.expression) {
                initial = cast(value(entry.expression), entry.type);
            } else if (info.kind == TypeKind::Floating) {
                initial = {OperandKind::Floating, entry.type, 0, 0, 0};
            } else {
                initial = integer(entry.type, 0);
            }
            auto location = destination;
            auto pointer = Result.pointer(entry.type);
            if (entry.offset || location.type != pointer) {
                location = emitValue(Opcode::AddressOffset, pointer, destination, integer(integerType(), entry.offset));
            }
            emit(Opcode::Store, location, initial);
        }
    }

    Operand Lowerer::globalAddress(const types::ExpressionNode* expression, TypeId target) {
        expression = Transparent(expression);
        if (auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression)) {
            const auto& payload = primary->getExpression();
            if (auto* identifier = std::get_if<types::IdentifierNode*>(&payload)) {
                auto symbol = (*identifier)->semanticInfo().Symbol;
                if (Result.functionById(symbol)) {
                    return {OperandKind::Function, target, symbol};
                }
                for (const auto& object : Result.globals) {
                    if (object.id == symbol) {
                        return {OperandKind::Object, target, symbol};
                    }
                }
            }
            if (auto* literal = std::get_if<types::ConstantNode*>(&payload)) {
                auto result = constant(*literal, type(expression));
                if (result.kind == OperandKind::Object) {
                    result.type = target;
                    return result;
                }
            }
        }
        if (auto* unary = dynamic_cast<const types::FactorNode*>(expression);
            unary && unary->getUnaryOp() == types::ASTUnaryOp::OP_DEREF) {
            return globalConstant(unary->getOperand(), target);
        }
        if (auto* index = dynamic_cast<const types::IndexPostfixExpr*>(expression)) {
            auto result = globalConstant(index->getBase(), target);
            auto offset = globalConstant(index->getIndex(), integerType());
            if (offset.kind != OperandKind::Integer) {
                fail(index, "static subscript address lacks a constant index");
            }
            result.integer += offset.integer * Result.type(type(expression)).size;
            return result;
        }
        if (auto* member = dynamic_cast<const types::MemberPostfixExpr*>(expression)) {
            auto result = globalAddress(member->getBase(), target);
            const auto& info = expression->semanticInfo();
            const auto& selected = Analyzer.types().get({info.MemberOwner}).members.at(info.MemberIndex - 1);
            if (selected.isStatic) {
                if (Result.functionById(selected.symbol.value)) {
                    return {OperandKind::Function, target, selected.symbol.value};
                }
                return {OperandKind::Object, target, selected.symbol.value};
            }
            auto source = Analyzer.types().unConst({member->getBase()->semanticInfo().Type});
            while (source.value != info.MemberOwner) {
                auto layout = Analyzer.types().layout(source);
                if (!layout || !layout.layout.baseOffset) {
                    fail(member, "static member address has no inherited layout");
                }
                result.integer += *layout.layout.baseOffset;
                source = Analyzer.types().unConst(Analyzer.types().get(source).base);
            }
            auto layout = Analyzer.types().layout({info.MemberOwner});
            if (!layout || !layout.layout.offsets.at(info.MemberIndex - 1)) {
                fail(member, "static member address lacks a layout offset");
            }
            result.integer += *layout.layout.offsets[info.MemberIndex - 1];
            return result;
        }
        fail(expression, "static address initializer has no representable object or function relocation");
    }

    Operand Lowerer::constantCast(Operand value, TypeId target, const types::ASTNode* source) {
        if (value.type == target) {
            return value;
        }
        const auto destination = Result.type(target);
        if (value.kind == OperandKind::Object || value.kind == OperandKind::Function) {
            if (destination.kind == TypeKind::Integer && destination.bits == 1) {
                return integer(target, 1);
            }
            if (destination.kind == TypeKind::Pointer || destination.kind == TypeKind::Integer) {
                value.type = target;
                return value;
            }
            fail(source, "static relocation cannot be converted to this type");
        }
        Instruction conversion{Opcode::Cast, {OperandKind::Temporary, target, 1}, value};
        if (auto result = FoldConstant(Result, conversion)) {
            return *result;
        }
        fail(source, "static scalar conversion is not representable in IR");
    }

    Operand Lowerer::globalConstant(const types::ExpressionNode* expression, TypeId target) {
        expression = Transparent(expression);
        if (!expression) {
            return Result.type(target).kind == TypeKind::Floating ? Operand{OperandKind::Floating, target}
                                                                  : integer(target, 0);
        }
        const auto own = type(expression);
        if (own && own != target) {
            return constantCast(globalConstant(expression, own), target, expression);
        }
        auto truth = [&](Operand value) {
            if (value.kind == OperandKind::Object || value.kind == OperandKind::Function) {
                return true;
            }
            return value.kind == OperandKind::Floating ? value.floating != 0 : value.integer != 0;
        };
        auto evaluate = [&](Opcode opcode, Operand left, Operand right = Operand{}) {
            Instruction operation{opcode, {OperandKind::Temporary, target, 1}, left, right};
            if (auto result = FoldConstant(Result, operation)) {
                return *result;
            }
            fail(expression, "static arithmetic is not representable as an IR constant");
        };
        if (auto* implicit = dynamic_cast<const types::ImplicitCastNode*>(expression)) {
            if (implicit->getConversionKind() == types::ImplicitConversionKind::ArrayToPointer ||
                implicit->getConversionKind() == types::ImplicitConversionKind::FunctionToPointer) {
                return globalAddress(implicit->getOperand(), target);
            }
            return constantCast(globalConstant(implicit->getOperand(), type(implicit->getOperand())), target,
                                expression);
        }
        if (auto* explicitCast = dynamic_cast<const types::CastFactorNode*>(expression)) {
            return constantCast(globalConstant(explicitCast->getExpression(), type(explicitCast->getExpression())),
                                target, expression);
        }
        if (auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression)) {
            const auto& payload = primary->getExpression();
            if (auto* literal = std::get_if<types::ConstantNode*>(&payload)) {
                return constant(*literal, target);
            }
            if (auto* identifier = std::get_if<types::IdentifierNode*>(&payload)) {
                const auto symbolId = (*identifier)->semanticInfo().Symbol;
                const auto& symbol = Analyzer.symbols().get({symbolId});
                if (Result.functionById(symbolId)) {
                    return callable(symbolId);
                }
                if (Result.type(target).kind == TypeKind::Array) {
                    return globalAddress(expression, target);
                }
                if (symbol.kind == semantic::SymbolKind::Enumerator && symbol.constant) {
                    return integer(target, *symbol.constant);
                }
                if (Analyzer.types().isConst(symbol.type)) {
                    if (auto* declaration = dynamic_cast<const types::SingleDeclarationNode*>(
                            static_cast<const types::ASTNode*>(symbol.source));
                        declaration && declaration->getInitializer()) {
                        return globalConstant(declaration->getInitializer(), target);
                    }
                    if (symbol.constant) {
                        return integer(target, *symbol.constant);
                    }
                }
            }
        }
        if (auto* list = dynamic_cast<const types::InitializerNode*>(expression)) {
            if (list->expressions().empty()) {
                return globalConstant(nullptr, target);
            }
            if (list->expressions().size() == 1) {
                return globalConstant(list->expressions().front(), target);
            }
        }
        if (auto* unary = dynamic_cast<const types::FactorNode*>(expression)) {
            if (unary->getUnaryOp() == types::ASTUnaryOp::OP_ADDR_OF) {
                return globalAddress(unary->getOperand(), target);
            }
            auto operand = globalConstant(unary->getOperand(), type(unary->getOperand()));
            switch (unary->getUnaryOp()) {
            case types::ASTUnaryOp::OP_UNARY_PLUS:
                return constantCast(operand, target, expression);
            case types::ASTUnaryOp::OP_UNARY_MINUS:
                return evaluate(Opcode::Negate, operand);
            case types::ASTUnaryOp::OP_TILDE:
                return evaluate(Opcode::Complement, operand);
            case types::ASTUnaryOp::OP_LOGICAL_NOT:
                return integer(target, !truth(operand));
            default:
                break;
            }
        }
        if (auto* ternary = dynamic_cast<const types::TernaryOpNode*>(expression)) {
            auto condition = globalConstant(ternary->getCondition(), type(ternary->getCondition()));
            return globalConstant(truth(condition) ? ternary->getThen() : ternary->getElse(), target);
        }
        if (auto* size = dynamic_cast<const types::SizeofNode*>(expression); size && size->semanticInfo().HasConstant) {
            return integer(target, size->semanticInfo().Constant);
        }
        if (auto* binary = dynamic_cast<const types::BinaryExpressionNode*>(expression)) {
            auto left = globalConstant(binary->getLeft(), type(binary->getLeft()));
            auto opcode = binary->getBinaryOp();
            if (opcode == types::OP_LOGICAL_AND && !truth(left)) {
                return integer(target, 0);
            }
            if (opcode == types::OP_LOGICAL_OR && truth(left)) {
                return integer(target, 1);
            }
            auto right = globalConstant(binary->getRight(), type(binary->getRight()));
            if (opcode == types::OP_LOGICAL_AND || opcode == types::OP_LOGICAL_OR) {
                return integer(target, truth(right));
            }
            if ((opcode == types::OP_ADD || opcode == types::OP_SUB) && Result.type(target).kind == TypeKind::Pointer) {
                if (Result.type(left.type).kind != TypeKind::Pointer && opcode == types::OP_ADD) {
                    std::swap(left, right);
                }
                auto pointerType = Result.type(left.type);
                if (pointerType.kind == TypeKind::Pointer &&
                    (left.kind == OperandKind::Object || left.kind == OperandKind::Function)) {
                    right = constantCast(right, integerType(), expression);
                    auto offset = right.integer * Result.type(pointerType.element).size;
                    left.integer += opcode == types::OP_SUB ? uint64_t{0} - offset : offset;
                    left.type = target;
                    return left;
                }
            }
            switch (opcode) {
            case types::OP_ADD:
                return evaluate(Opcode::Add, left, right);
            case types::OP_SUB:
                return evaluate(Opcode::Subtract, left, right);
            case types::OP_MUL:
                return evaluate(Opcode::Multiply, left, right);
            case types::OP_DIV:
                return evaluate(Opcode::Divide, left, right);
            case types::OP_MOD:
                return evaluate(Opcode::Remainder, left, right);
            case types::OP_LSHIFT:
                return evaluate(Opcode::ShiftLeft, left, right);
            case types::OP_RSHIFT:
                return evaluate(Opcode::ShiftRight, left, right);
            case types::OP_BIT_AND:
                return evaluate(Opcode::BitAnd, left, right);
            case types::OP_BIT_OR:
                return evaluate(Opcode::BitOr, left, right);
            case types::OP_BIT_XOR:
                return evaluate(Opcode::BitXor, left, right);
            case types::OP_EQ:
                return evaluate(Opcode::Equal, left, right);
            case types::OP_NEQ:
                return evaluate(Opcode::NotEqual, left, right);
            case types::OP_LT:
                return evaluate(Opcode::Less, left, right);
            case types::OP_GT:
                return evaluate(Opcode::Greater, left, right);
            case types::OP_LE:
                return evaluate(Opcode::LessEqual, left, right);
            case types::OP_GE:
                return evaluate(Opcode::GreaterEqual, left, right);
            default:
                break;
            }
        }
        fail(expression, "static initializer is not representable as an IR scalar constant or relocation yet");
    }

    void Lowerer::initializeGlobal(Object& destination, TypeId target, const types::ExpressionNode* initializer,
                                   uint64_t offset) {
        // Do not retain references into Result.globals while lowering string
        // literals: creating their backing objects may reallocate that vector.
        auto id = destination.id;
        std::vector<Object::InitialValue> values;
        for (const auto& entry : initializerPlan(target, initializer)) {
            auto info = Result.type(entry.type);
            if (info.kind == TypeKind::Array) {
                auto text = *String(entry.expression);
                text.push_back('\0');
                for (uint64_t i = 0; i < info.count && i < text.size(); ++i) {
                    values.push_back(
                        {offset + entry.offset + i, integer(info.element, static_cast<unsigned char>(text[i]))});
                }
            } else if (entry.expression) {
                auto value = globalConstant(entry.expression, entry.type);
                if (value.type != entry.type) {
                    fail(entry.expression, "static initializer IR type does not match its object");
                }
                values.push_back({offset + entry.offset, value});
            }
        }
        for (auto& global : Result.globals) {
            if (global.id == id) {
                global.initialValues = std::move(values);
                return;
            }
        }
        fail(initializer, "global initializer lost its destination object");
    }

    void Lowerer::declarations(const types::DeclarationList* list, bool global) {
        for (auto* declaration : list->getDeclarations()) {
            auto symbol = declaration->semanticInfo().Symbol;
            if (!symbol) {
                fail(declaration, "declaration lacks a resolved symbol");
            }
            const auto& entry = Analyzer.symbols().get({symbol});
            if (entry.kind == semantic::SymbolKind::Function) {
                continue;
            }
            if (global || entry.isStatic) {
                for (auto& object : Result.globals) {
                    if (object.id == symbol) {
                        if (declaration->getInitializer()) {
                            initializeGlobal(object, object.type, declaration->getInitializer());
                        }
                        break;
                    }
                }
            } else {
                initialize(object(symbol), importType(entry.type), declaration->getInitializer());
            }
        }
    }
}
