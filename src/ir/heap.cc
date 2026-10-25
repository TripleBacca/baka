#include "lowering.hh"
#include <functional>

namespace baka {
    namespace ir {
        uint32_t Lowerer::heapFunction(bool allocation) {
            auto& id = allocation ? HeapAllocate : HeapDeallocate;
            if (id) {
                return id;
            }
            auto size = importType(Analyzer.types().sizeType());
            auto pointer = Result.pointer(importType(Analyzer.types().builtin(semantic::Builtin::Void)));
            Function helper;
            helper.id = NextObject++;
            helper.name = allocation ? "__baka_allocate" : "__baka_deallocate";
            helper.signature = allocation
                                   ? Result.function(pointer, {size, size}, false)
                                   : Result.function(importType(Analyzer.types().builtin(semantic::Builtin::Void)),
                                                     {pointer}, false);
            id = helper.id;
            Result.functions.push_back(std::move(helper));
            return id;
        }

        Operand Lowerer::heapCall(const types::FunctionCallPostfixExpr* node, bool allocation) {
            auto* list = dynamic_cast<const types::CommaExpressionNode*>(node->getArguments());
            auto* witness = list ? list->getExpressions().front() : node->getArguments();
            auto pointerType = type(witness);
            auto target = Result.type(pointerType).element;
            if (!allocation) {
                auto pointer = value(witness);
                auto destroyBlock = newBlock(), done = newBlock();
                branch(pointer, destroyBlock, done);
                select(destroyBlock);
                if (needsDestruction(target)) {
                    destroy(pointer, target);
                }
                auto helper = heapFunction(false);
                auto parameter = Result.type(Result.functionById(helper)->signature).parameters.front();
                invoke(helper, {cast(pointer, parameter)});
                jump(done);
                select(done);
                return {};
            }

            const auto& plan = Analyzer.executionPlans().call(node);
            auto helper = heapFunction(true);
            auto size = importType(Analyzer.types().sizeType());
            auto raw = invoke(helper, {integer(size, Result.type(target).size),
                                       integer(size, Result.type(target).alignment)});
            auto pointer = cast(raw, pointerType);
            auto result = temporary(pointerType);
            emit(Opcode::Copy, result, pointer);
            auto initialize = newBlock(), done = newBlock();
            branch(pointer, initialize, done);
            select(initialize);

            std::function<void(Operand, TypeId)> initializeOne = [&](Operand address, TypeId type) {
                const auto& info = Result.type(type);
                if (info.kind == TypeKind::Array) {
                    auto element = info.element;
                    forEachElement(address, type, info.count, false, [&](Operand item) {
                        initializeOne(item, element);
                        finishTemps();
                    });
                } else if (info.kind == TypeKind::Record) {
                    auto selected = plan.member ? SpecialFunctions.at({plan.owner.value, plan.member}) : 0;
                    if (selected || needsConstruction(type)) {
                        construct(address, type, selected, plan.arguments);
                    }
                } else if (!plan.arguments.empty()) {
                    store(address, cast(value(plan.arguments.front()), type));
                }
            };
            initializeOne(pointer, target);
            jump(done);
            select(done);
            return result;
        }
    }
}
