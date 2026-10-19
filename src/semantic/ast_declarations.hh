#pragma once

#include "declarations.hh"
#include "types/parser/ast/declaration.hh"
#include "types/parser/ast/typenode.hh"
#include "types/parser/ast/typedef.hh"
#include "types/parser/ast/enums.hh"

namespace baka {
    namespace semantic {
        struct EvaluationContext {
            enum class Purpose { ArrayBound, Enumerator } purpose;
            ScopeId scope;
            TypeId expected;
        };

        // B implements this interface. Evaluation runs in the live declaration environment.
        class DeclarationEvaluator {
        public:
            virtual ~DeclarationEvaluator() = default;
            virtual IntegerResult integer(const types::ExpressionNode*, EvaluationContext) = 0;
        };

        class ASTDeclarations {
            Types& TypeSystem;
            Symbols& SymbolTable;
            Declarations& DeclarationChecker;
            DeclarationEvaluator* Evaluator;

            IntegerResult EvaluateInteger(const types::ExpressionNode* expression, EvaluationContext::Purpose purpose) {
                if (!Evaluator) {
                    return {};
                }
                return Evaluator->integer(expression,
                                          {purpose, SymbolTable.currentScope(), TypeSystem.builtin(Builtin::Int)});
            }

        public:
            ASTDeclarations(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                            DeclarationEvaluator* Evaluator = nullptr)
                : TypeSystem(TypeSystem), SymbolTable(SymbolTable), DeclarationChecker(DeclarationChecker),
                  Evaluator(Evaluator) {}

            TypeId specifier(const types::IdentifierNode* name, types::TypeSpecifierModifier modifier, bool constant,
                             const void* source = nullptr) {
                if (!name || name->hasError()) {
                    return {};
                }
                std::optional<TypeKind> expected;
                switch (modifier) {
                case types::TypeSpecifierModifier::STRUCT:
                    expected = TypeKind::Struct;
                    break;
                case types::TypeSpecifierModifier::CLASS:
                    expected = TypeKind::Class;
                    break;
                case types::TypeSpecifierModifier::UNION:
                    expected = TypeKind::Union;
                    break;
                case types::TypeSpecifierModifier::ENUM:
                    expected = TypeKind::Enum;
                    break;
                default:
                    break;
                }
                return DeclarationChecker.specifier(std::string(name->GetName()), constant,
                                                    modifier == types::TypeSpecifierModifier::UNSIGNED, expected,
                                                    source);
            }

            TypeId typeName(const types::TypeNode& node) {
                return specifier(node.getTypeName(), node.getModifier(), node.getIsConst(), &node);
            }

            // Build outward suffixes before recursing inward: int (*p)[3] is pointer-to-array.
            TypeId declarator(TypeId base, const types::DeclarationIdentifierNode* node) {
                if (!node || node->hasError() || !base.value) {
                    return {};
                }
                for (size_t Index = 0; Index < node->getPointerCount(); ++Index) {
                    base = TypeSystem.pointer(base);
                }
                const auto& bounds = node->getArraySizes();
                for (auto it = bounds.rbegin(); it != bounds.rend(); ++it) {
                    auto value = *it ? EvaluateInteger(*it, EvaluationContext::Purpose::ArrayBound) : IntegerResult{};
                    base = DeclarationChecker.boundArray(base, value, *it, !*it);
                }
                if (auto* raw = node->getFunctionParameters()) {
                    auto* params = dynamic_cast<const types::FunctionParameterList*>(raw);
                    if (!params) {
                        SymbolTable.diagnose("parameters", "invalid parameter-list AST", raw);
                        return {};
                    }
                    std::vector<TypeId> parameters;
                    bool valid = true;
                    SymbolTable.enter(ScopeKind::Prototype);
                    for (auto* Parameter : params->getParameters()) {
                        if (!Parameter || Parameter->hasError()) {
                            parameters.push_back({});
                            valid = false;
                            continue;
                        }
                        auto type = specifier(Parameter->getTypeName(), Parameter->getModifier(),
                                              Parameter->getIsConst(), Parameter);
                        auto* decl = Parameter->getDeclaration();
                        if (decl) {
                            type = declarator(type, decl->getDeclarator());
                        }
                        if (!DeclarationChecker.validate(type, {true, false, true, false}, Parameter)) {
                            type = {};
                        }
                        type = TypeSystem.parameterObject(type);
                        parameters.push_back(TypeSystem.adjustParameter(type));
                        if (decl && decl->getDeclarator() && decl->getDeclarator()->getIdentifier() && type.value) {
                            auto Symbol = SymbolTable.declare({std::string(decl->getDeclarator()->getVariableName()),
                                                               SymbolKind::Parameter, type, true, false, Linkage::None,
                                                               Parameter});
                            valid = Symbol.value != 0 && valid;
                        }
                    }
                    SymbolTable.leave();
                    base = valid ? TypeSystem.function(base, std::move(parameters), params->isVariadic()) : TypeId{};
                    if (!base.value) {
                        SymbolTable.diagnose("signature", "invalid function return or parameter type", node);
                    }
                }
                const auto& inner = node->getInnerDeclaration();
                if (auto* nested = std::get_if<types::DeclarationIdentifierNode*>(&inner); nested && *nested) {
                    base = declarator(base, *nested);
                }
                return base;
            }

            // C enters one Function scope, calls this, then visits the outer body in that SAME scope.
            // Nested blocks use Block scopes, so only genuine inner-block shadowing is legal.
            std::vector<SymbolId> parameters(const types::FunctionParameterList& params) {
                std::vector<SymbolId> result;
                for (auto* Parameter : params.getParameters()) {
                    if (!Parameter || Parameter->hasError() || !Parameter->getDeclaration()) {
                        continue;
                    }
                    auto* decl = Parameter->getDeclaration()->getDeclarator();
                    auto type = declarator(specifier(Parameter->getTypeName(), Parameter->getModifier(),
                                                     Parameter->getIsConst(), Parameter),
                                           decl);
                    if (decl && decl->getIdentifier()) {
                        result.push_back(
                            DeclarationChecker.object({std::string(decl->getVariableName()), SymbolKind::Parameter,
                                                       type, true, false, Linkage::None, Parameter},
                                                      {true, false, false, true}));
                    }
                }
                return result;
            }

            // B's initializer callback runs AFTER insertion; each declarator's bound runs BEFORE.
            // Thus int x=x; binds the new x, while int x[sizeof(x)]; sees the prior x.
            using InitializerCheck = std::function<void(SymbolId, const types::ExpressionNode*)>;

            std::vector<SymbolId> declarationList(const types::DeclarationList& node,
                                                  InitializerCheck initialize = {}) {
                std::vector<SymbolId> result;
                auto base = specifier(node.getTypeName(), node.getModifier(), node.getIsConst(), &node);
                bool global = SymbolTable.scope(SymbolTable.currentScope()).kind == ScopeKind::Global;
                for (auto* item : node.getDeclarations()) {
                    if (!item || item->hasError() || !item->getDeclarator() ||
                        !item->getDeclarator()->getIdentifier()) {
                        continue;
                    }
                    auto type = declarator(base, item->getDeclarator());
                    // Insert before initializer checking; B may complete an unspecified array
                    // with Symbols::completeObject. Final storage validation runs afterwards.
                    bool infer = type.value && TypeSystem.get(type).kind == TypeKind::Array &&
                                 TypeSystem.get(type).bound.kind == BoundKind::Unspecified && item->getInitializer();
                    auto id = DeclarationChecker.object(
                        {std::string(item->getDeclarator()->getVariableName()), SymbolKind::Object, type, true,
                         node.getIsStatic(),
                         global ? (node.getIsStatic() ? Linkage::Internal : Linkage::External) : Linkage::None, item},
                        {false, false, false, !infer});
                    result.push_back(id);
                    if (id.value && initialize) {
                        initialize(id, item->getInitializer());
                    }
                    if (id.value && infer) {
                        DeclarationChecker.validate(SymbolTable.get(id).type, {}, item);
                    }
                }
                return result;
            }

            std::vector<SymbolId> aliases(const types::TypedefNode& node) {
                std::vector<SymbolId> result;
                auto base = specifier(node.getTypeName(), node.getModifier(), node.getIsConst(), &node);
                for (auto* variable : node.getVariables()) {
                    auto type = declarator(base, variable);
                    if (variable && variable->getIdentifier()) {
                        result.push_back(
                            DeclarationChecker.alias(std::string(variable->getVariableName()), type, variable));
                    }
                }
                return result;
            }

            TypeId enumeration(const types::EnumNode& node) {
                auto* name = node.getEnumName();
                auto type = DeclarationChecker.aggregate(name ? std::string(name->GetName()) : std::string{},
                                                         TypeKind::Enum, &node);
                if (!DeclarationChecker.beginAggregate(type, &node)) {
                    return {};
                }
                std::optional<int64_t> previous;
                bool valid = true;
                for (size_t Index = 0; Index < node.getEnumValues().size(); ++Index) {
                    auto* expr = node.getInitializers().at(Index);
                    auto value = expr ? EvaluateInteger(expr, EvaluationContext::Purpose::Enumerator)
                                      : (valid ? DeclarationChecker.nextEnumerator(previous) : IntegerResult{});
                    auto* name = node.getEnumValues()[Index];
                    auto id = DeclarationChecker.enumerator(std::string(name->GetName()), type, value, name);
                    if (!id.value) {
                        valid = false;
                    } else {
                        previous = value.value;
                    }
                }
                if (!valid || !DeclarationChecker.finishAggregate(type, &node)) {
                    return {};
                }
                return type;
            }
        };
    }
}
