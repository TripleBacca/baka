#include "semantic_analyzer.hh"
#include "types/parser/ast/struct.hh"
#include "types/parser/ast/class.hh"
#include "types/parser/ast/union.hh"

namespace baka::semantic {
    namespace {
        types::FunctionParameterList* Parameters(types::DeclarationIdentifierNode* declarator) {
            if (!declarator) return nullptr;
            if (auto* parameters = dynamic_cast<types::FunctionParameterList*>(declarator->getFunctionParameters()))
                return parameters;
            if (auto* inner = std::get_if<types::DeclarationIdentifierNode*>(&declarator->getInnerDeclaration()))
                return Parameters(*inner);
            return nullptr;
        }
    }

    bool SemanticAnalyzer::registerDeclarations(types::ProgramNode& program) {
        if (Phase != SemanticPhase::Parsed) return false;
        TopLevel.assign(program.getBody().begin(), program.getBody().end());
        // This is the only ProgramNode traversal. Definitions enqueue bodies;
        // they never inspect those bodies while top-level names are collected.
        for (auto* node : TopLevel) registerNode(node);
        Phase = SemanticPhase::Registered;
        refreshDiagnostics();
        return Diagnostics.empty();
    }

    void SemanticAnalyzer::registerDefaults(types::FunctionParameterList* parameters, TypeId signature,
                                           SymbolId symbol, const types::ASTNode* source) {
        if (!parameters || !signature.value || TypeSystem.get(signature).kind != TypeKind::Function) return;
        const auto& types = TypeSystem.get(signature).parameters;
        std::vector<const void*> defaults(types.size());
        for (size_t i = 0; i < parameters->getParameters().size() && i < types.size(); ++i) {
            auto* parameter = parameters->getParameters()[i];
            if (!parameter || !parameter->getDeclaration()) continue;
            auto* initializer = parameter->getDeclaration()->getInitializer();
            defaults[i] = initializer;
            if (initializer) {
                auto index = queue(initializer, parameter, ExpressionUse::Default, {}, types[i]);
                // Defaults are declared outside the eventual function invocation;
                // there is no implicit instance object in that environment.
                if (Current.owner.value) Expressions[index].context.isStatic = true;
            }
        }
        if (symbol.value) SymbolTable.defaults(symbol, defaults);
        (void)source;
    }

    void SemanticAnalyzer::registerDeclarations(types::DeclarationList* list) {
        if (!list) return;
        const auto ids = ASTResolver.declarationList(*list, [this](SymbolId symbol, const types::ExpressionNode* initializer) {
            // Resolve a const scalar dependency while its declaration-point scope
            // is live, so subsequent array bounds and enumerators can use it.
            if (initializer && TypeSystem.isConst(SymbolTable.get(symbol).type) &&
                TypeSystem.isIntegral(SymbolTable.get(symbol).type)) {
                resolveInitializer(const_cast<types::ExpressionNode*>(initializer));
                auto value = Checker.integer(initializer, {EvaluationContext::Purpose::ArrayBound,
                    SymbolTable.currentScope(), TypeSystem.builtin(Builtin::Int)});
                if (value.state == ConstantState::Value) SymbolTable.setConstant(symbol, value.value);
            }
        });
        const bool global = SymbolTable.scope(SymbolTable.currentScope()).kind == ScopeKind::Global;
        for (size_t i = 0; i < ids.size(); ++i) {
            auto id = ids[i];
            if (!id.value) continue;
            auto* item = list->getDeclarations()[i];
            const auto& symbol = SymbolTable.get(id);
            Current.globalLimit = id.value;
            if (symbol.kind == SymbolKind::Function) {
                registerDefaults(Parameters(item->getDeclarator()), symbol.type, id, item);
                if (item->getInitializer()) error("initializer", "function declaration cannot have an initializer", item);
            } else {
                queue(item->getInitializer(), item, ExpressionUse::Initializer, id, symbol.type,
                      global || list->getIsStatic());
            }
        }
    }

    void SemanticAnalyzer::registerFunction(types::FunctionNode* node, TypeId owner) {
        auto type = owner.value ? TypeId{node->semanticInfo().Type} :
            ASTResolver.declarator(ASTResolver.specifier(node->getReturnTypeName(), node->getModifier(),
                                                        node->getIsConst(), node), node->getDeclarator());
        if (!type.value || TypeSystem.get(type).kind != TypeKind::Function) {
            error("function", "function body requires a function declarator", node);
            return;
        }
        SymbolId symbol{node->semanticInfo().Symbol};
        if (!owner.value) {
            symbol = DeclarationChecker.functionDefinition({std::string(node->getDeclarator()->getVariableName()),
                SymbolKind::Function, type, true, node->getIsStatic(),
                node->getIsStatic() ? Linkage::Internal : Linkage::External, node});
            node->semanticInfo().Type = type.value;
            node->semanticInfo().Symbol = symbol.value;
        }
        auto* name = node->getDeclarator()->getIdentifier();
        if (name && symbol.value) {
            name->resolve({symbol.value}, std::string(name->GetName()) + "." + std::to_string(symbol.value));
            name->semanticInfo().Type = type.value;
        }
        Current.globalLimit = SymbolTable.symbolCount();
        auto context = Current;
        context.owner = owner; context.result = TypeSystem.get(type).element;
        context.isStatic = node->getIsStatic(); context.isConst = false; // leading const qualifies return type
        auto* parameters = Parameters(node->getDeclarator());
        registerDefaults(parameters, type, symbol, node);
        Bodies.push_back({node, node->getBody(), parameters, context, type, symbol});
    }

    void SemanticAnalyzer::registerMembers(types::StructBodyNode* body, TypeId owner) {
        auto saved = Current;
        Current.scope = {body->semanticInfo().Scope};
        Current.owner = owner;
        Current.globalLimit = SymbolTable.symbolCount();
        select(Current);
        for (auto* member : body->getMembers()) {
            if (auto* function = dynamic_cast<types::FunctionNode*>(member)) registerFunction(function, owner);
            else if (auto* constructor = dynamic_cast<types::ConstructorNode*>(member)) {
                auto context = Current; context.result = TypeSystem.builtin(Builtin::Void);
                auto signature = TypeId{constructor->semanticInfo().Type};
                registerDefaults(constructor->getArgs(), signature, {}, constructor);
                Bodies.push_back({constructor, constructor->getBody(), constructor->getArgs(), context, signature, {}});
            } else if (auto* destructor = dynamic_cast<types::DestructorNode*>(member)) {
                auto context = Current; context.result = TypeSystem.builtin(Builtin::Void);
                Bodies.push_back({destructor, destructor->getBody(), nullptr, context,
                                  TypeId{destructor->semanticInfo().Type}, {}});
            } else if (auto* list = dynamic_cast<types::DeclarationList*>(member)) {
                for (auto* item : list->getDeclarations()) {
                    SymbolId symbol{item->semanticInfo().Symbol};
                    if (!symbol.value) continue;
                    if (SymbolTable.get(symbol).kind == SymbolKind::Function)
                        registerDefaults(Parameters(item->getDeclarator()), SymbolTable.get(symbol).type, symbol, item);
                    else if (item->getInitializer())
                        queue(item->getInitializer(), item, ExpressionUse::Initializer, symbol,
                              SymbolTable.get(symbol).type, list->getIsStatic());
                }
            }
        }
        select(saved);
    }

    void SemanticAnalyzer::registerNode(types::ASTNode* node) {
        if (!node) return;
        Current.scope = SymbolTable.currentScope();
        Current.globalLimit = std::numeric_limits<uint32_t>::max();
        SymbolTable.setGlobalVisibility(Current.globalLimit);
        if (auto* list = dynamic_cast<types::DeclarationList*>(node)) registerDeclarations(list);
        else if (auto* alias = dynamic_cast<types::TypedefNode*>(node)) ASTResolver.aliases(*alias);
        else if (auto* enumeration = dynamic_cast<types::EnumNode*>(node)) ASTResolver.enumeration(*enumeration);
        else if (auto* forward = dynamic_cast<types::StructDeclarationNode*>(node))
            node->semanticInfo().Type = Aggregates.forward(forward->getName(), TypeKind::Struct, node).value;
        else if (auto* forward = dynamic_cast<types::ClassDeclarationNode*>(node))
            node->semanticInfo().Type = Aggregates.forward(forward->getName(), TypeKind::Class, node).value;
        else if (auto* definition = dynamic_cast<types::StructDefinitionNode*>(node)) {
            auto owner = Aggregates.definition(*definition, [this](const types::ASTNode* member, TypeId type) {
                return Checker.memberSignature(member, type);
            });
            if (owner.value) registerMembers(definition->getBody(), owner);
        } else if (auto* definition = dynamic_cast<types::ClassDefinitionNode*>(node)) {
            auto owner = Aggregates.definition(*definition, [this](const types::ASTNode* member, TypeId type) {
                return Checker.memberSignature(member, type);
            });
            if (owner.value) registerMembers(definition->getBody(), owner);
        } else if (auto* definition = dynamic_cast<types::UnionNode*>(node)) {
            auto owner = Aggregates.definition(*definition, [this](const types::ASTNode* member, TypeId type) {
                return Checker.memberSignature(member, type);
            });
            if (owner.value && definition->getBody()) registerMembers(definition->getBody(), owner);
        } else if (auto* function = dynamic_cast<types::FunctionNode*>(node)) registerFunction(function);
        else error("node", "unsupported top-level semantic node", node);
    }
}
