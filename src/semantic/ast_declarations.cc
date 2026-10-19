#include "ast_declarations.hh"
#include "types/parser/ast/initializer.hh"

namespace baka {
    namespace semantic {
        namespace {
            void Attach(const types::ASTNode* node, TypeId type, SymbolId symbol, ScopeId scope) {
                if (!node) return;
                auto& info = const_cast<types::ASTNode*>(node)->semanticInfo();
                info.Type = type.value; info.Symbol = symbol.value; info.Scope = scope.value;
                info.Resolved = true;
            }
            void AttachDeclaration(const types::DeclarationIdentifierNode* node, TypeId type,
                                   SymbolId symbol, Symbols& table) {
                Attach(node, type, symbol, table.currentScope());
                if (node && node->getIdentifier() && symbol.value) {
                    auto* identifier = node->getIdentifier();
                    identifier->resolve({symbol.value}, std::string(identifier->GetName()) + "." + std::to_string(symbol.value));
                    Attach(identifier, type, symbol, table.currentScope());
                }
            }
        }

        IntegerResult ASTDeclarations::EvaluateInteger(const types::ExpressionNode* expression,
                                                       EvaluationContext::Purpose purpose) {
            if (PrepareExpression) PrepareExpression(const_cast<types::ExpressionNode*>(expression));
            if (!Evaluator) {
                return {};
            }
            return Evaluator->integer(expression,
                                      {purpose, SymbolTable.currentScope(), TypeSystem.builtin(Builtin::Int)});
        }

        ASTDeclarations::ASTDeclarations(Types& TypeSystem, Symbols& SymbolTable, Declarations& DeclarationChecker,
                                         DeclarationEvaluator* Evaluator)
            : TypeSystem(TypeSystem), SymbolTable(SymbolTable), DeclarationChecker(DeclarationChecker),
              Evaluator(Evaluator) {}

        void ASTDeclarations::setEvaluator(DeclarationEvaluator* evaluator) {
            Evaluator = evaluator;
        }

        TypeId ASTDeclarations::specifier(const types::IdentifierNode* name,
                                          types::TypeSpecifierModifier modifier, bool constant,
                                          const void* source) {
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
            auto type = DeclarationChecker.specifier(std::string(name->GetName()), constant,
                                                modifier == types::TypeSpecifierModifier::UNSIGNED, expected, source);
            Attach(name, type, {}, SymbolTable.currentScope());
            return type;
        }

        TypeId ASTDeclarations::typeName(const types::TypeNode& node) {
            if (node.semanticInfo().Resolved) return {node.semanticInfo().Type};
            auto type = specifier(node.getTypeName(), node.getModifier(), node.getIsConst(), &node);
            Attach(&node, type, {}, SymbolTable.currentScope());
            return type;
        }

        TypeId ASTDeclarations::declarator(TypeId base, const types::DeclarationIdentifierNode* node) {
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
            Attach(node, base, {}, SymbolTable.currentScope());
            return base;
        }

        std::vector<SymbolId> ASTDeclarations::parameters(const types::FunctionParameterList& params) {
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
                    auto id = DeclarationChecker.object({std::string(decl->getVariableName()), SymbolKind::Parameter,
                                                         type, true, false, Linkage::None, Parameter},
                                                        {true, false, false, true});
                    result.push_back(id);
                    Attach(Parameter, SymbolTable.get(id).type, id, SymbolTable.currentScope());
                    AttachDeclaration(decl, SymbolTable.get(id).type, id, SymbolTable);
                }
            }
            return result;
        }

        std::vector<SymbolId> ASTDeclarations::declarationList(const types::DeclarationList& node,
                                                               InitializerCheck initialize) {
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
                if (id.value && infer) {
                    inferArrayBound(id, item->getInitializer());
                    type = SymbolTable.get(id).type;
                }
                Attach(item, type, id, SymbolTable.currentScope());
                AttachDeclaration(item->getDeclarator(), type, id, SymbolTable);
                if (id.value && initialize) {
                    initialize(id, item->getInitializer());
                }
                if (id.value && infer && initialize && SymbolTable.get(id).type != type) {
                    DeclarationChecker.validate(SymbolTable.get(id).type, {}, item);
                }
            }
            return result;
        }

        bool ASTDeclarations::inferArrayBound(SymbolId symbol, const types::ExpressionNode* expression) {
            if (!symbol.value) return false;
            auto type = SymbolTable.get(symbol).type;
            const auto& array = TypeSystem.get(TypeSystem.unConst(type));
            if (array.kind != TypeKind::Array || array.bound.kind != BoundKind::Unspecified) return true;
            auto* initializer = dynamic_cast<const types::InitializerNode*>(expression);
            if (!initializer) return false;
            auto unwrap = [](const types::ExpressionNode* node) {
                while (auto* list = dynamic_cast<const types::InitializerNode*>(node)) {
                    if (list->isBraced() || list->expressions().size() != 1) break;
                    node = list->expressions().front();
                }
                return node;
            };
            auto stringLength = [&](const types::ExpressionNode* node) -> std::optional<size_t> {
                auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(unwrap(node));
                if (!primary) return {};
                auto payload = primary->getExpression();
                auto* constant = std::get_if<types::ConstantNode*>(&payload);
                if (!constant) return {};
                if (auto* string = dynamic_cast<types::ConstantTNode<base::SLString>*>(*constant)) return string->getValue().size()+1;
                return {};
            };
            const auto& values = initializer->expressions();
            size_t count = 0;
            if (values.size() == 1 && stringLength(values.front()) &&
                TypeSystem.unConst(array.element) == TypeSystem.builtin(Builtin::Char)) {
                count = *stringLength(values.front());
            } else if (initializer->isBraced()) {
                // Consume one element's source initializer at a time. This also
                // handles brace elision: int a[][2]={1,2,3,4} has two rows, not four.
                std::function<void(TypeId, size_t&)> consume = [&](TypeId target, size_t& cursor) {
                    if (cursor >= values.size()) return;
                    auto* value = unwrap(values[cursor]);
                    if (auto* list = dynamic_cast<const types::InitializerNode*>(value); list && list->isBraced()) {
                        ++cursor; return;
                    }
                    const auto& info = TypeSystem.get(TypeSystem.unConst(target));
                    if (info.kind == TypeKind::Array) {
                        if (stringLength(value)) { ++cursor; return; }
                        for (uint64_t i=0; i<info.bound.count && cursor<values.size(); ++i) consume(info.element, cursor);
                    } else if (TypeSystem.isAggregate(target)) {
                        if (auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(value)) {
                            auto payload = primary->getExpression();
                            if (auto* name = std::get_if<types::IdentifierNode*>(&payload)) {
                                auto found = SymbolTable.lookup(std::string((*name)->GetName()));
                                if (found.size()==1 && TypeSystem.unConst(SymbolTable.get(found.front()).type)==TypeSystem.unConst(target)) {
                                    ++cursor; return;
                                }
                            }
                        }
                        auto subobjects = TypeSystem.subobjects(target);
                        if (info.kind == TypeKind::Union && subobjects.size() > 1) subobjects.resize(1);
                        for (const auto& child : subobjects) consume(child.type, cursor);
                    } else ++cursor;
                };
                size_t cursor = 0;
                while (cursor < values.size()) {
                    auto before = cursor;
                    consume(array.element, cursor);
                    if (before == cursor) break;
                    ++count;
                }
            }
            if (!count) {
                SymbolTable.diagnose("bound", "cannot infer a positive array bound from this initializer", initializer);
                return false;
            }
            auto completed = TypeSystem.array(array.element, {BoundKind::Constant, count});
            return SymbolTable.completeObject(symbol, completed);
        }

        std::vector<SymbolId> ASTDeclarations::aliases(const types::TypedefNode& node) {
            std::vector<SymbolId> result;
            auto base = specifier(node.getTypeName(), node.getModifier(), node.getIsConst(), &node);
            for (auto* variable : node.getVariables()) {
                auto type = declarator(base, variable);
                if (variable && variable->getIdentifier()) {
                    auto id = DeclarationChecker.alias(std::string(variable->getVariableName()), type, variable);
                    result.push_back(id);
                    AttachDeclaration(variable, type, id, SymbolTable);
                }
            }
            return result;
        }

        TypeId ASTDeclarations::enumeration(const types::EnumNode& node) {
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
                Attach(name, type, id, SymbolTable.currentScope());
                if (id.value) name->resolve({id.value}, std::string(name->GetName()) + "." + std::to_string(id.value));
                if (!id.value) {
                    valid = false;
                } else {
                    previous = value.value;
                }
            }
            Attach(&node, type, {}, SymbolTable.currentScope());
            if (!valid || !DeclarationChecker.finishAggregate(type, &node)) {
                return {};
            }
            return type;
        }
    }
}
