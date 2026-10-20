#include "lowering.hh"
#include <algorithm>
#include <functional>

namespace baka::ir {
    void Lowerer::lowerFunction(const types::FunctionNode* declaration) {
        if (!declaration->getBody()) {
            return;
        }
        auto symbol = declaration->semanticInfo().Symbol;
        auto binding = Members.find(symbol);
        lowerCallable(declaration, declaration->getBody(), symbol,
                      binding != Members.end() && !binding->second.isStatic ? semantic::TypeId{binding->second.owner}
                                                                            : semantic::TypeId{});
    }

    void Lowerer::lowerCallable(const types::ASTNode* declaration, const types::StatementNode* body, uint32_t symbol,
                                semantic::TypeId owner, bool constructor, bool destructor) {
        auto it = std::find_if(Result.functions.begin(), Result.functions.end(),
                               [&](const Function& item) { return item.id == symbol; });
        if (it == Result.functions.end()) {
            fail(declaration, "function definition has no IR declaration");
        }
        FunctionIndex = static_cast<size_t>(it - Result.functions.begin());
        function().defined = true;
        CurrentBlock = 0;
        NextBlock = 1;
        Self = 0;
        SelfOverride = {};
        CurrentOwner = owner;
        InDestructor = destructor;
        LocalCleanups.clear();
        ActiveCleanups.clear();
        LabelCleanups.clear();
        CaseCleanups.clear();
        BreakDepths.clear();
        ContinueDepths.clear();
        BuildingTemps.clear();
        PendingTemps.clear();
        ValueOwners.clear();
        ValueLocations.clear();
        Destinations.clear();
        DirectDestinations.clear();
        Breaks.clear();
        Continues.clear();
        Labels.clear();
        Cases.clear();
        SavedTargets.clear();
        auto functionScope = declaration ? declaration->semanticInfo().Scope : 0;
        for (uint32_t i = 1; i <= Analyzer.symbols().symbolCount(); ++i) {
            const auto& symbol = Analyzer.symbols().get({i});
            if (symbol.kind != semantic::SymbolKind::Object && symbol.kind != semantic::SymbolKind::Parameter) {
                continue;
            }
            if (symbol.isStatic) {
                continue;
            }
            auto scope = symbol.scope;
            while (scope.value && scope.value != functionScope) {
                scope = Analyzer.symbols().scope(scope).parent;
            }
            if (!scope.value) {
                continue;
            }
            Object object;
            object.id = i;
            object.name = symbol.name;
            object.type = importType(symbol.type);
            function().objects.push_back(std::move(object));
            if (symbol.kind == semantic::SymbolKind::Parameter && symbol.scope.value == functionScope) {
                function().parameters.push_back(i);
                if (symbol.name == "this") {
                    Self = i;
                }
            }
        }
        if (owner.value && !Self) {
            auto receiver = storage(Result.type(function().signature).parameters.front(), "this");
            Self = receiver.id;
            function().parameters.push_back(Self);
        }
        select(newBlock());
        for (const auto& local : function().objects) {
            if (!needsDestruction(local.type) && !needsConstruction(local.type)) {
                continue;
            }
            auto flag = activeFlag();
            LocalCleanups[local.id] = {object(local.id), local.type, flag};
            if (std::find(function().parameters.begin(), function().parameters.end(), local.id) !=
                function().parameters.end()) {
                emit(Opcode::Copy, flag, integer(flag.type, 1));
                ActiveCleanups.push_back(local.id);
            }
        }
        if (constructor) {
            BuildingExpression = true;
            initializeMembers(self(), Result.type(Result.type(function().signature).parameters.front()).element);
            finishTemps();
            BuildingExpression = false;
        }
        const auto entry = CurrentBlock;
        planCleanupTargets(body, ActiveCleanups);
        ExpressionRegions.clear();
        RegionCursor.clear();
        // Pass 1 emits independent expression regions. Pass 2 connects those
        // regions in source order and adds statements/control-flow edges.
        collectExpressionRegions(body);
        select(entry);
        statement(body);
        auto signature = Result.type(function().signature);
        if (block().instructions.empty() || !IsTerminator(block().instructions.back().opcode)) {
            Operand result;
            const auto& type = Result.type(signature.element);
            if (type.kind == TypeKind::Floating) {
                result = {OperandKind::Floating, signature.element, 0, 0, 0};
            } else if (type.kind == TypeKind::Record) {
                Object storage;
                storage.id = NextObject++;
                storage.name = ".fallthrough";
                storage.type = signature.element;
                auto id = storage.id;
                function().objects.push_back(std::move(storage));
                initialize(object(id), signature.element, nullptr, true);
                result = load(object(id), signature.element);
            } else if (type.kind != TypeKind::Void) {
                result = integer(signature.element, 0);
            }
            cleanupTo(0);
            if (InDestructor) {
                destroyMembers(self(), Result.type(Result.type(function().signature).parameters.front()).element);
            }
            emit(Opcode::Return, {}, result);
        }
    }

    void Lowerer::node(const types::ASTNode* node) {
        if (!node) {
            return;
        }
        if (auto* stmt = dynamic_cast<const types::StatementNode*>(node)) {
            statement(stmt);
        } else if (auto* declaration = dynamic_cast<const types::DeclarationList*>(node)) {
            declarations(declaration);
        } else if (dynamic_cast<const types::TypedefNode*>(node)) {
            return;
        } else {
            fail(node, "unsupported declaration in an IR function body");
        }
    }

    void Lowerer::collectCases(const types::StatementNode* statement,
                               std::vector<const types::CaseLabelStatementNode*>& cases,
                               const types::DefaultLabelStatementNode*& fallback) {
        if (!statement || dynamic_cast<const types::SwitchBlockStatementNode*>(statement)) {
            return;
        }
        if (auto* label = dynamic_cast<const types::CaseLabelStatementNode*>(statement)) {
            Cases[label] = newBlock();
            cases.push_back(label);
            collectCases(label->getStatement(), cases, fallback);
        } else if (auto* label = dynamic_cast<const types::DefaultLabelStatementNode*>(statement)) {
            Cases[label] = newBlock();
            fallback = label;
            collectCases(label->getStatement(), cases, fallback);
        } else if (auto* compound = dynamic_cast<const types::CompoundStatementNode*>(statement)) {
            for (const auto& item : compound->getStatements()) {
                std::visit(
                    [&](auto* child) {
                        if (auto* stmt = dynamic_cast<const types::StatementNode*>(child)) {
                            collectCases(stmt, cases, fallback);
                        }
                    },
                    item);
            }
        } else if (auto* conditional = dynamic_cast<const types::IfSuperBlockStatementNode*>(statement)) {
            collectCases(conditional->getIf()->getBody(), cases, fallback);
            for (auto* arm : conditional->getElseIfs()) {
                collectCases(arm->getBody(), cases, fallback);
            }
            if (conditional->getElse()) {
                collectCases(conditional->getElse()->getBody(), cases, fallback);
            }
        } else if (auto* loop = dynamic_cast<const types::ForBlockStatementNode*>(statement)) {
            collectCases(loop->getBody(), cases, fallback);
        } else if (auto* loop = dynamic_cast<const types::WhileBlockStatementNode*>(statement)) {
            collectCases(loop->getBody(), cases, fallback);
        } else if (auto* loop = dynamic_cast<const types::DoWhileBlockStatementNode*>(statement)) {
            collectCases(loop->getBody(), cases, fallback);
        } else if (auto* label = dynamic_cast<const types::GotoLabelStatementNode*>(statement)) {
            collectCases(label->getStatement(), cases, fallback);
        }
    }

    void Lowerer::statement(const types::StatementNode* statement) {
        if (!statement) {
            return;
        }
        auto span = statement->getSourceSpan();
        if (span.Valid) {
            Source = {span.Line, span.Col};
        }
        if (auto* compound = dynamic_cast<const types::CompoundStatementNode*>(statement)) {
            const auto depth = ActiveCleanups.size();
            for (const auto& item : compound->getStatements()) {
                std::visit([&](auto* child) { node(child); }, item);
            }
            if (block().instructions.empty() || !IsTerminator(block().instructions.back().opcode)) {
                cleanupTo(depth);
            }
            ActiveCleanups.resize(depth);
        } else if (dynamic_cast<const types::EmptyStatementNode*>(statement)) {
            return;
        } else if (auto* expression = dynamic_cast<const types::ExpressionStatementNode*>(statement)) {
            value(expression->getExpression());
            finishTemps();
        } else if (auto* result = dynamic_cast<const types::ReturnStatementNode*>(statement)) {
            auto operand = value(result->getExpression());
            transfer(operand);
            finishTemps();
            cleanupTo(0);
            if (InDestructor) {
                destroyMembers(self(), Result.type(Result.type(function().signature).parameters.front()).element);
            }
            emit(Opcode::Return, {}, operand);
        } else if (auto* conditional = dynamic_cast<const types::IfSuperBlockStatementNode*>(statement)) {
            std::vector<std::pair<const types::ExpressionNode*, const types::StatementNode*>> arms;
            arms.push_back({conditional->getIf()->getCondition(), conditional->getIf()->getBody()});
            for (auto* arm : conditional->getElseIfs()) {
                arms.push_back({arm->getCondition(), arm->getBody()});
            }
            auto join = newBlock();
            for (const auto& [condition, body] : arms) {
                auto conditionValue = value(condition);
                finishTemps();
                auto yes = newBlock(), no = newBlock();
                branch(conditionValue, yes, no);
                select(yes);
                this->statement(body);
                link(join);
                select(no);
            }
            if (conditional->getElse()) {
                this->statement(conditional->getElse()->getBody());
            }
            link(join);
            select(join);
        } else if (auto* loop = dynamic_cast<const types::ForBlockStatementNode*>(statement)) {
            const auto before = ActiveCleanups.size();
            std::visit(
                [&](auto* initializer) {
                    if constexpr (std::is_same_v<std::remove_pointer_t<decltype(initializer)>, types::ExpressionNode>) {
                        value(initializer);
                        finishTemps();
                    } else {
                        node(initializer);
                    }
                },
                loop->getInitializer());
            auto condition = newBlock(), body = newBlock(), update = newBlock(), exit = newBlock();
            Breaks[loop->semanticInfo().Target] = exit;
            Continues[loop->semanticInfo().Target] = update;
            BreakDepths[loop->semanticInfo().Target] = ActiveCleanups.size();
            ContinueDepths[loop->semanticInfo().Target] = ActiveCleanups.size();
            link(condition);
            select(condition);
            if (loop->getCondition()) {
                auto test = value(loop->getCondition());
                finishTemps();
                branch(test, body, exit);
            } else {
                jump(body);
            }
            select(body);
            this->statement(loop->getBody());
            link(update);
            select(update);
            value(loop->getUpdate());
            finishTemps();
            link(condition);
            select(exit);
            cleanupTo(before);
            ActiveCleanups.resize(before);
        } else if (auto* loop = dynamic_cast<const types::WhileBlockStatementNode*>(statement)) {
            auto condition = newBlock(), body = newBlock(), exit = newBlock();
            Breaks[loop->semanticInfo().Target] = exit;
            Continues[loop->semanticInfo().Target] = condition;
            BreakDepths[loop->semanticInfo().Target] = ActiveCleanups.size();
            ContinueDepths[loop->semanticInfo().Target] = ActiveCleanups.size();
            link(condition);
            select(condition);
            auto test = value(loop->getCondition());
            finishTemps();
            branch(test, body, exit);
            select(body);
            this->statement(loop->getBody());
            link(condition);
            select(exit);
        } else if (auto* loop = dynamic_cast<const types::DoWhileBlockStatementNode*>(statement)) {
            auto body = newBlock(), condition = newBlock(), exit = newBlock();
            Breaks[loop->semanticInfo().Target] = exit;
            Continues[loop->semanticInfo().Target] = condition;
            BreakDepths[loop->semanticInfo().Target] = ActiveCleanups.size();
            ContinueDepths[loop->semanticInfo().Target] = ActiveCleanups.size();
            link(body);
            select(body);
            this->statement(loop->getBody());
            link(condition);
            select(condition);
            auto test = value(loop->getCondition());
            finishTemps();
            branch(test, body, exit);
            select(exit);
        } else if (auto* selection = dynamic_cast<const types::SwitchBlockStatementNode*>(statement)) {
            auto condition = value(selection->getCondition());
            finishTemps();
            auto exit = newBlock();
            Breaks[selection->semanticInfo().Target] = exit;
            BreakDepths[selection->semanticInfo().Target] = ActiveCleanups.size();
            std::vector<const types::CaseLabelStatementNode*> cases;
            const types::DefaultLabelStatementNode* fallback = nullptr;
            collectCases(selection->getBody(), cases, fallback);
            auto checkEntry = [&](const types::ASTNode* label) {
                for (auto id : CaseCleanups.at(label)) {
                    if (std::find(ActiveCleanups.begin(), ActiveCleanups.end(), id) == ActiveCleanups.end()) {
                        fail(label, "switch dispatch enters an uninitialized object lifetime");
                    }
                }
            };
            for (auto* label : cases) {
                checkEntry(label);
            }
            if (fallback) {
                checkEntry(fallback);
            }
            for (auto* label : cases) {
                const auto& metadata = label->getTag()->semanticInfo();
                if (!metadata.HasConstant) {
                    fail(label, "case label lacks an integral semantic constant");
                }
                auto next = newBlock();
                auto boolean = importType(Analyzer.types().builtin(semantic::Builtin::Bool));
                auto equal = emitValue(Opcode::Equal, boolean, condition,
                                       integer(condition.type, static_cast<uint64_t>(metadata.Constant)));
                branch(equal, Cases.at(label), next);
                select(next);
            }
            jump(fallback ? Cases.at(fallback) : exit);
            // The lexical pre-case region is deliberately disconnected from switch
            // dispatch, but goto can still reach labels within it.
            select(newBlock());
            this->statement(selection->getBody());
            link(exit);
            select(exit);
        } else if (auto* label = dynamic_cast<const types::CaseLabelStatementNode*>(statement)) {
            if (!Cases.count(label)) {
                fail(label, "case label has no owning IR switch");
            }
            link(Cases.at(label));
            select(Cases.at(label));
            this->statement(label->getStatement());
        } else if (auto* label = dynamic_cast<const types::DefaultLabelStatementNode*>(statement)) {
            if (!Cases.count(label)) {
                fail(label, "default label has no owning IR switch");
            }
            link(Cases.at(label));
            select(Cases.at(label));
            this->statement(label->getStatement());
        } else if (auto* label = dynamic_cast<const types::GotoLabelStatementNode*>(statement)) {
            auto target = label->semanticInfo().Target;
            if (!Labels.count(target)) {
                Labels[target] = newBlock();
            }
            link(Labels.at(target));
            select(Labels.at(target));
            this->statement(label->getStatement());
        } else if (auto* jump = dynamic_cast<const types::GotoStatementNode*>(statement)) {
            auto target = jump->semanticInfo().Target;
            if (!target) {
                fail(jump, "goto lacks resolved semantic target");
            }
            if (!Labels.count(target)) {
                Labels[target] = newBlock();
            }
            const auto& retained = LabelCleanups.at(target);
            for (auto id : retained) {
                if (std::find(ActiveCleanups.begin(), ActiveCleanups.end(), id) == ActiveCleanups.end()) {
                    fail(jump, "goto enters an uninitialized object lifetime");
                }
            }
            for (auto it = ActiveCleanups.rbegin(); it != ActiveCleanups.rend(); ++it) {
                if (std::find(retained.begin(), retained.end(), *it) == retained.end()) {
                    cleanup(LocalCleanups.at(*it));
                }
            }
            this->jump(Labels.at(target));
        } else if (auto* jump = dynamic_cast<const types::BreakStatementNode*>(statement)) {
            if (!Breaks.count(jump->semanticInfo().Target)) {
                fail(jump, "break lacks an active resolved IR target");
            }
            cleanupTo(BreakDepths.at(jump->semanticInfo().Target));
            this->jump(Breaks.at(jump->semanticInfo().Target));
        } else if (auto* jump = dynamic_cast<const types::ContinueStatementNode*>(statement)) {
            if (!Continues.count(jump->semanticInfo().Target)) {
                fail(jump, "continue lacks an active resolved IR target");
            }
            cleanupTo(ContinueDepths.at(jump->semanticInfo().Target));
            this->jump(Continues.at(jump->semanticInfo().Target));
        } else {
            fail(statement, "unsupported statement node in IR lowering");
        }
    }
}
