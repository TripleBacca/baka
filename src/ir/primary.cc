#include "lowering.hh"
#include <algorithm>
#include <bit>
#include <limits>

namespace baka {
    namespace ir {
        Operand Lowerer::constant(const types::ConstantNode* node, TypeId target) {
            if (auto* literal = dynamic_cast<const types::ConstantTNode<int>*>(node)) {
                return integer(target, static_cast<uint64_t>(literal->getValue()));
            }
            if (auto* literal = dynamic_cast<const types::ConstantTNode<char>*>(node)) {
                return integer(target, static_cast<uint64_t>(literal->getValue()));
            }
            if (auto* literal = dynamic_cast<const types::ConstantTNode<long long>*>(node)) {
                return integer(target, static_cast<uint64_t>(literal->getValue()));
            }
            if (auto* literal = dynamic_cast<const types::ConstantTNode<unsigned int>*>(node)) {
                return integer(target, static_cast<uint64_t>(literal->getValue()));
            }
            if (auto* literal = dynamic_cast<const types::ConstantTNode<unsigned long long>*>(node)) {
                return integer(target, static_cast<uint64_t>(literal->getValue()));
            }
            if (auto* literal = dynamic_cast<const types::ConstantTNode<bool>*>(node)) {
                return integer(target, static_cast<uint64_t>(literal->getValue()));
            }

            if (auto* literal = dynamic_cast<const types::ConstantTNode<float>*>(node)) {
                return {OperandKind::Floating, target, 0, 0, literal->getValue()};
            }
            if (auto* literal = dynamic_cast<const types::ConstantTNode<double>*>(node)) {
                return {OperandKind::Floating, target, 0, 0, literal->getValue()};
            }
            if (dynamic_cast<const types::ConstantTNode<std::nullptr_t>*>(node)) {
                return integer(target, 0);
            }
            std::string bytes;
            if (auto* literal = dynamic_cast<const types::ConstantTNode<base::SLString>*>(node)) {
                bytes = literal->getValue();
            } else if (auto* literal = dynamic_cast<const types::ConstantTNode<std::string>*>(node)) {
                bytes = literal->getValue();
            } else {
                fail(node, "unsupported literal in IR lowering");
            }
            bytes.push_back('\0');
            Object string;
            string.id = NextObject++;
            string.name = ".string";
            string.type = target;
            string.global = string.internal = string.readOnly = true;
            string.bytes = std::move(bytes);
            auto id = string.id;
            Result.globals.push_back(std::move(string));
            return {OperandKind::Object, Result.pointer(target), id};
        }

        Operand Lowerer::self() {
            if (State.SelfOverride) {
                return State.SelfOverride;
            }
            if (!State.Self) {
                fail(nullptr, "bound member expression has no active receiver");
            }
            auto location = object(State.Self);
            return load(location, Result.type(location.type).element);
        }

        Operand Lowerer::primaryValue(const types::PrimaryExpressionNode* primary) {
            const auto resultType = type(primary);
            const auto& payload = primary->getExpression();
            if (auto* literal = std::get_if<types::ConstantNode*>(&payload)) {
                auto result = constant(*literal, resultType);
                if (result.kind == OperandKind::Object) {
                    return cast(result, Result.pointer(Result.type(resultType).element));
                }
                return result;
            }
            if (std::holds_alternative<types::ThisNode*>(payload)) {
                return self();
            }
            auto symbol = std::get<types::IdentifierNode*>(payload)->semanticInfo().Symbol;
            if (!symbol) {
                symbol = primary->semanticInfo().Symbol;
            }
            const auto& selected = Analyzer.symbols().get({symbol});
            if (selected.kind == semantic::SymbolKind::Enumerator) {
                return integer(resultType, static_cast<uint64_t>(*selected.constant));
            }
            if (selected.kind == semantic::SymbolKind::Function ||
                selected.kind == semantic::SymbolKind::BuiltinFunction) {
                if (auto member = Members.find(symbol); member != Members.end() && !member->second.isStatic) {
                    fail(primary, "bound instance methods can only be lowered in call context");
                }
                return callable(symbol);
            }
            return load(address(primary), resultType);
        }

        Operand Lowerer::memberValue(const types::ExpressionNode* expression) {
            const auto resultType = type(expression);
            auto symbol = expression->semanticInfo().Symbol;
            if (symbol && Result.functionById(symbol)) {
                if (auto* dot = dynamic_cast<const types::MemberPostfixExpr*>(expression)) {
                    address(dot->getBase());
                }
                if (auto* arrow = dynamic_cast<const types::ArrowPostfixExpr*>(expression)) {
                    value(arrow->getBase());
                }
                auto binding = Members.at(symbol);
                if (!binding.isStatic) {
                    fail(expression, "bound instance methods require a call context");
                }
                return callable(symbol);
            }
            return load(address(expression), resultType);
        }
    }
}
