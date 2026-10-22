#include "lowering.hh"

namespace baka {
    namespace ir {
        Operand Lowerer::address(const types::ExpressionNode* expression) {
            if (!expression) {
                fail(expression, "missing lvalue in IR lowering");
            }
            const auto& info = expression->semanticInfo();
            if (!info.LValue && Result.type(type(expression)).kind == TypeKind::Record) {
                // Member access on a returned record needs addressable storage. This
                // materialization is local IR storage, never a hidden call argument.
                auto captured = value(expression);
                if (auto found = State.ValueLocations.find(captured.id);
                    captured.kind == OperandKind::Temporary && found != State.ValueLocations.end()) {
                    return found->second;
                }
                Object storage;
                storage.id = NextObject++;
                storage.name = ".materialized";
                storage.type = captured.type;
                auto id = storage.id;
                function().objects.push_back(std::move(storage));
                auto location = object(id);
                store(location, captured);
                return location;
            }
            // Assignment and prefix mutation are lvalues in baka. Their outer
            // LValueToRValue conversion must evaluate the write once, then load the
            // saved location, rather than trying to recover an address from a value.
            if (auto* conversion = dynamic_cast<const types::ImplicitCastNode*>(expression);
                conversion && conversion->getConversionKind() == types::ImplicitConversionKind::LValueToRValue) {
                return address(conversion->getOperand());
            }
            if (auto* assignment = dynamic_cast<const types::BinaryExpressionNode*>(expression);
                assignment && assignment->getBinaryOp() == types::OP_ASSIGN) {
                auto location = address(assignment->getLeft());
                auto result = value(assignment->getRight());
                store(location, result);
                return location;
            }
            if (auto* assignment = dynamic_cast<const types::CompoundAssignmentNode*>(expression)) {
                return compoundAssignment(assignment, true);
            }
            if (auto* prefix = dynamic_cast<const types::FactorNode*>(expression);
                prefix && (prefix->getUnaryOp() == types::ASTUnaryOp::OP_INC ||
                           prefix->getUnaryOp() == types::ASTUnaryOp::OP_DEC)) {
                return increment(prefix->getOperand(), prefix->getUnaryOp() == types::ASTUnaryOp::OP_DEC, false, true);
            }
            if (auto* ternary = dynamic_cast<const types::TernaryOpNode*>(expression)) {
                return conditionalAddress(ternary);
            }
            if (auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression)) {
                return primaryAddress(primary);
            }
            if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression)) {
                const auto& expressions = comma->getExpressions();
                if (expressions.empty()) {
                    fail(expression, "empty comma expression has no address");
                }
                for (size_t i = 0; i + 1 < expressions.size(); ++i) {
                    value(expressions[i]);
                }
                return address(expressions.back());
            }
            if (auto* target = dynamic_cast<const types::CompoundTargetNode*>(expression)) {
                if (State.SavedTargets.empty()) {
                    fail(target, "compound target used outside its assignment");
                }
                return State.SavedTargets.back();
            }
            if (auto* unary = dynamic_cast<const types::FactorNode*>(expression);
                unary && unary->getUnaryOp() == types::ASTUnaryOp::OP_DEREF) {
                return value(unary->getOperand());
            }
            if (auto* index = dynamic_cast<const types::IndexPostfixExpr*>(expression)) {
                return indexAddress(index);
            }
            if (auto* member = dynamic_cast<const types::MemberPostfixExpr*>(expression)) {
                auto base = address(member->getBase());
                return memberAddress(expression, base, {member->getBase()->semanticInfo().Type});
            }
            if (auto* member = dynamic_cast<const types::ArrowPostfixExpr*>(expression)) {
                auto base = value(member->getBase());
                return memberAddress(expression, base, {member->getBase()->semanticInfo().Type});
            }
            fail(expression, "expression has no supported IR address");
        }

        Operand Lowerer::primaryAddress(const types::PrimaryExpressionNode* primary) {
            const auto& info = primary->semanticInfo();
            const auto& payload = primary->getExpression();
            if (auto* name = std::get_if<types::IdentifierNode*>(&payload)) {
                auto symbol = (*name)->semanticInfo().Symbol;
                if (!symbol) {
                    symbol = info.Symbol;
                }
                if (auto member = Members.find(symbol); member != Members.end() && !member->second.isStatic) {
                    const auto& selected =
                        Analyzer.types().get({member->second.owner}).members[member->second.index - 1];
                    if (Analyzer.types().get(Analyzer.types().unConst(selected.type)).kind ==
                        semantic::TypeKind::Function) {
                        fail(primary, "an instance method cannot escape as an ordinary function pointer");
                    }
                    auto receiver = self();
                    return memberAddress(primary, receiver, State.CurrentOwner);
                }
                const auto& selected = Analyzer.symbols().get({symbol});
                if (selected.kind == semantic::SymbolKind::Function ||
                    selected.kind == semantic::SymbolKind::BuiltinFunction) {
                    return callable(symbol);
                }
                return object(symbol);
            }
            if (auto* literal = std::get_if<types::ConstantNode*>(&payload)) {
                auto result = constant(*literal, type(primary));
                if (result.kind == OperandKind::Object) {
                    return result;
                }
            }
            fail(primary, "expression has no supported IR address");
        }

        Operand Lowerer::indexAddress(const types::IndexPostfixExpr* index) {
            auto base = value(index->getBase());
            auto offset = value(index->getIndex());
            const auto& element = Result.type(type(index));
            offset = cast(offset, integerType());
            offset = emitValue(Opcode::Multiply, integerType(), offset, integer(integerType(), element.size));
            return emitValue(Opcode::AddressOffset, Result.pointer(type(index)), base, offset);
        }

        Operand Lowerer::memberAddress(const types::ASTNode* node, Operand receiver, semantic::TypeId receiverType) {
            const auto& metadata = node->semanticInfo();
            if (!metadata.MemberOwner || !metadata.MemberIndex) {
                fail(node, "member expression lacks selected-member metadata");
            }
            const auto& owner = Analyzer.types().get({metadata.MemberOwner});
            const auto& member = owner.members.at(metadata.MemberIndex - 1);
            if (member.isStatic) {
                return object(member.symbol.value);
            }
            auto source = Analyzer.types().unConst(receiverType);
            if (Analyzer.types().get(source).kind == semantic::TypeKind::Pointer) {
                source = Analyzer.types().unConst(Analyzer.types().get(source).element);
            }
            uint64_t offset = 0;
            while (source.value != metadata.MemberOwner) {
                const auto& current = Analyzer.types().get(source);
                auto baseOffset = Result.type(importType(source)).baseOffset;
                if (!current.base.value || !baseOffset) {
                    fail(node, "cannot compute inherited member receiver adjustment");
                }
                offset += *baseOffset;
                source = Analyzer.types().unConst(current.base);
            }
            auto memberOffset =
                Result.type(importType({metadata.MemberOwner})).memberOffsets.at(metadata.MemberIndex - 1);
            if (!memberOffset) {
                fail(node, "selected member has no object offset");
            }
            offset += *memberOffset;
            return emitValue(Opcode::AddressOffset, Result.pointer(importType(member.type)), receiver,
                             integer(integerType(), offset));
        }
    }
}
