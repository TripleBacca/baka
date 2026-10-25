#include "lowering.hh"

namespace baka {
    namespace ir {
        Operand Lowerer::receiver(const types::ExpressionNode* callee, uint32_t owner) {
            if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(callee)) {
                const auto& expressions = comma->getExpressions();
                if (expressions.empty()) {
                    fail(callee, "missing bound callee");
                }
                for (size_t i = 0; i + 1 < expressions.size(); ++i) {
                    value(expressions[i]);
                }
                return receiver(expressions.back(), owner);
            }
            Operand result;
            semantic::TypeId source;
            if (auto* dot = dynamic_cast<const types::MemberPostfixExpr*>(callee)) {
                result = address(dot->getBase());
                source = {dot->getBase()->semanticInfo().Type};
            } else if (auto* arrow = dynamic_cast<const types::ArrowPostfixExpr*>(callee)) {
                result = value(arrow->getBase());
                source = {arrow->getBase()->semanticInfo().Type};
                source = Analyzer.types().get(Analyzer.types().unConst(source)).element;
            } else {
                result = self();
                source = State.CurrentOwner;
            }
            source = Analyzer.types().unConst(source);
            uint64_t offset = 0;
            while (source.value != owner) {
                auto baseOffset = Result.type(importType(source)).baseOffset;
                const auto& current = Analyzer.types().get(source);
                if (!current.base.value || !baseOffset) {
                    fail(callee, "cannot adjust bound method receiver");
                }
                offset += *baseOffset;
                source = Analyzer.types().unConst(current.base);
            }
            auto target = Result.pointer(importType({owner}));
            if (offset) {
                return emitValue(Opcode::AddressOffset, target, result, integer(integerType(), offset));
            }
            return cast(result, target);
        }

        Operand Lowerer::constructionValue(const types::ExpressionNode* node,
                                           const std::vector<const types::ExpressionNode*>& arguments) {
            const auto& metadata = node->semanticInfo();
            auto target = type(node);
            auto location = destination(node);
            const bool direct = bool(location);
            if (!location) {
                location = storage(target, ".constructed");
            }
            auto selected = metadata.MemberIndex
                                ? SpecialFunctions.at(
                                      {Analyzer.types().unConst({metadata.MemberOwner}).value, metadata.MemberIndex})
                                : 0;
            // Value-initialization through an implicit default constructor zeros
            // scalar subobjects before applying default member initializers.
            if (!selected) {
                zeroObject(location, target);
            }
            if (selected || needsConstruction(target)) {
                construct(location, target, selected, arguments);
            }
            auto captured = load(location, target);
            State.ValueLocations[captured.id] = location;
            return direct ? captured : ownTemporary(captured, location);
        }

        Operand Lowerer::call(const types::FunctionCallPostfixExpr* node) {
            if (auto kind = types::heapCallKind(node); kind != types::HeapCallKind::None) {
                return heapCall(node, kind == types::HeapCallKind::Allocate);
            }
            const auto& metadata = node->semanticInfo();
            const auto& plan = Analyzer.executionPlans().call(node);
            if (plan.construction) {
                return constructionValue(node, plan.arguments);
            }
            const auto& expressions = plan.arguments;
            const auto& semanticSignature = Analyzer.types().get(plan.signature);
            Operand callee;
            std::vector<Operand> arguments;
            if (metadata.MemberOwner && metadata.MemberIndex) {
                const auto& selected = Analyzer.types().get({metadata.MemberOwner}).members[metadata.MemberIndex - 1];
                if (!selected.isStatic && Analyzer.types().get(Analyzer.types().unConst(selected.type)).kind ==
                                              semantic::TypeKind::Function) {
                    auto instance = receiver(node->getCallee(), metadata.MemberOwner);
                    arguments.push_back(instance);
                    callee = callable(metadata.Symbol);
                }
            }
            if (!callee) {
                callee = value(node->getCallee());
            }
            // Capture each value before staging the call's param instructions;
            // later argument side effects cannot reload an earlier source value.
            for (size_t index = 0; index < expressions.size(); ++index) {
                auto argument = value(expressions[index]);
                if (index < semanticSignature.parameters.size()) {
                    transfer(argument);
                }
                arguments.push_back(argument);
            }
            auto resultType = importType(semanticSignature.element);
            auto result = Result.type(resultType).kind == TypeKind::Void ? Operand{} : temporary(resultType);
            emitCall(result, callee, std::move(arguments));
            if (result && needsDestruction(result.type)) {
                if (auto location = destination(node)) {
                    store(location, result);
                } else {
                    result = ownTemporary(result);
                }
            }
            return result;
        }
    }
}
