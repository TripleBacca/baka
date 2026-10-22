#include "expressions.hh"

namespace baka {
    namespace semantic {
        ExprResult ExpressionChecker::checkMember(const types::ExpressionNode* baseNode,
                                                  const types::IdentifierNode* member, bool arrow,
                                                  const types::ASTNode* source) {
            auto base = check(baseNode, false);
            if (base.error) {
                return makeError();
            }
            if (base.category == ValueCategory::RValue) {
                base.type = decay(base.type);
            }
            auto access =
                DeclarationChecker.memberAccess(base.type, std::string(member->GetName()), arrow, false, source);
            if (!access.type.value) {
                return makeError();
            }
            if (access.candidates.size() > 1) {
                SymbolTable.diagnose("overload", "overload set requires a call", source);
                return makeError();
            }
            if (Annotating && access.candidates.size() == 1) {
                auto& metadata =
                    const_cast<types::ASTNode*>(static_cast<const types::ASTNode*>(source))->semanticInfo();
                metadata.MemberOwner = access.owner.value;
                metadata.MemberIndex = static_cast<uint32_t>(access.candidates.front() + 1);
                const auto& selected = TypeSystem.get(access.owner).members[access.candidates.front()];
                metadata.Symbol = selected.symbol.value;
                if (selected.symbol.value) {
                    auto* identifier = const_cast<types::IdentifierNode*>(member);
                    identifier->resolve({selected.symbol.value},
                                        selected.name + "." + std::to_string(selected.symbol.value));
                    identifier->semanticInfo().Type = access.type.value;
                }
            }
            auto category =
                (base.category == ValueCategory::LValue || arrow) ? ValueCategory::LValue : ValueCategory::RValue;
            auto result = makeValue(access.type, category);
            if (access.candidates.size() == 1) {
                const auto& entry = TypeSystem.get(access.owner).members[access.candidates.front()];
                result.boundInstanceFunction =
                    !entry.isStatic && TypeSystem.get(TypeSystem.unConst(entry.type)).kind == TypeKind::Function;
            }
            return result;
        }

        std::optional<Member> ExpressionChecker::memberSignature(types::ASTNode* node, TypeId owner) {
            if (auto* function = dynamic_cast<types::FunctionNode*>(node)) {
                auto* returnName = function->getReturnTypeName();
                if (!returnName) {
                    return std::nullopt;
                }
                auto type =
                    ASTResolver.specifier(returnName, function->getModifier(), function->getIsConst(), function);
                type = ASTResolver.declarator(type, function->getDeclarator());
                if (!type.value) {
                    return std::nullopt;
                }
                Member member;
                member.name = std::string(function->getDeclarator() ? function->getDeclarator()->getVariableName()
                                                                    : std::string_view{});
                member.type = type;
                if (auto* params = function->getDeclarator() ? dynamic_cast<types::FunctionParameterList*>(
                                                                   function->getDeclarator()->getFunctionParameters())
                                                             : nullptr) {
                    size_t required = 0;
                    for (auto* parameter : params->getParameters()) {
                        if (parameter && !parameter->hasInitalizer()) {
                            ++required;
                        }
                    }
                    member.minimumArity = required;
                }
                member.isStatic = function->getIsStatic();
                member.isConstMethod = false; // leading const is return qualification, not a receiver qualifier
                member.source = function;
                return member;
            }
            if (auto* constructor = dynamic_cast<types::ConstructorNode*>(node)) {
                std::vector<TypeId> parameters;
                if (auto* args = constructor->getArgs()) {
                    SymbolTable.enter(ScopeKind::Prototype);
                    for (auto* parameter : args->getParameters()) {
                        auto type = ASTResolver.specifier(parameter->getTypeName(), parameter->getModifier(),
                                                          parameter->getIsConst(), parameter);
                        if (auto* declaration = parameter->getDeclaration()) {
                            type = ASTResolver.declarator(type, declaration->getDeclarator());
                        }
                        parameters.push_back(TypeSystem.adjustParameter(type));
                    }
                    SymbolTable.leave();
                }
                auto type = TypeSystem.function(TypeSystem.builtin(Builtin::Void), std::move(parameters));
                if (!type.value) {
                    return std::nullopt;
                }
                Member member;
                size_t required = 0;
                if (auto* args = constructor->getArgs()) {
                    for (auto* parameter : args->getParameters()) {
                        if (parameter && !parameter->hasInitalizer()) {
                            ++required;
                        }
                    }
                    member.minimumArity = required;
                }
                member.name = TypeSystem.get(owner).name;
                member.type = type;
                member.isConstructor = true;
                member.source = constructor;
                return member;
            }
            if (auto* destructor = dynamic_cast<types::DestructorNode*>(node)) {
                auto type = TypeSystem.function(TypeSystem.builtin(Builtin::Void));
                if (!type.value) {
                    return std::nullopt;
                }
                (void)destructor;
                Member member;
                member.name = "~" + TypeSystem.get(owner).name;
                member.type = type;
                member.isDestructor = true;
                member.source = destructor;
                return member;
            }
            return std::nullopt;
        }
    }
}
