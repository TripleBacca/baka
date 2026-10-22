#include "expressions.hh"
#include <algorithm>

namespace baka {
    namespace semantic {
        void ExpressionChecker::retainConversions(const types::FunctionCallPostfixExpr* call,
                                                  const OverloadSelection& selection) {
            if (!Annotating) {
                return;
            }
            std::vector<const types::ExpressionNode*> arguments;
            if (auto* list = dynamic_cast<const types::CommaExpressionNode*>(call->getArguments())) {
                arguments.assign(list->getExpressions().begin(), list->getExpressions().end());
            } else if (call->getArguments()) {
                arguments.push_back(call->getArguments());
            }
            for (size_t i = 0; i < selection.conversions.size() && i < arguments.size(); ++i) {
                const auto& conversion = selection.conversions[i];
                if (conversion.owner.value) {
                    ConstructorConversions[{arguments[i], conversion.owner.value}] = conversion;
                }
            }
        }

        ConstructorSelection ExpressionChecker::defaultConstructor(TypeId type) const {
            type = TypeSystem.unConst(type);
            std::vector<FunctionCandidate> candidates;
            const auto& record = TypeSystem.get(type);
            for (size_t i = 0; i < record.members.size(); ++i) {
                const auto& member = record.members[i];
                if (member.isConstructor) {
                    candidates.push_back({member.type, {}, member.minimumArity, type, static_cast<uint32_t>(i + 1)});
                }
            }
            if (candidates.empty()) {
                return {type, {}, 0, true, {}};
            }
            auto selected = selectOverload(candidates, {});
            if (selected.ambiguous) {
                return {type, {}, 0, false, "ambiguous overload resolution for default construction"};
            }
            if (!selected.candidate) {
                return {type, {}, 0, false, "object has no callable default constructor"};
            }
            ConstructorSelection result{type, selected.candidate->type, selected.candidate->memberIndex, true, {}};
            const auto* declaration =
                static_cast<const types::ConstructorNode*>(record.members[result.member - 1].source);
            for (auto* parameter : declaration->getArgs()->getParameters()) {
                result.arguments.push_back(parameter->getDeclaration()->getInitializer());
            }
            return result;
        }

        bool ExpressionChecker::exactArgument(TypeId from, TypeId to) const {
            // By-value parameter normalization and aliases already use canonical IDs.
            // Top-level const does not distinguish values; pointee const still does.
            return from.value && to.value && TypeSystem.unConst(from) == TypeSystem.unConst(to);
        }

        bool ExpressionChecker::compatibleArity(const FunctionCandidate& candidate, size_t count) const {
            const auto& signature = TypeSystem.get(candidate.type);
            if (signature.kind != TypeKind::Function) {
                return false;
            }
            const auto required =
                candidate.required == static_cast<size_t>(-1) ? signature.parameters.size() : candidate.required;
            return count >= required && (signature.variadic || count <= signature.parameters.size());
        }

        ExpressionChecker::ConversionMatch ExpressionChecker::implicitArgument(const ExprResult& argument,
                                                                               TypeId target, bool constructors) const {
            if (argument.error) {
                return ConversionMatch::None;
            }
            // Keep the existing standard conversion policy in one place: arithmetic,
            // adding qualification, null pointers and compatible/derived pointers.
            if (isAssignable(argument.type, target, argument)) {
                return ConversionMatch::Unique;
            }
            if (!constructors || !TypeSystem.isAggregate(target)) {
                return ConversionMatch::None;
            }
            auto selection = convertingConstructor(target, argument);
            if (selection.ambiguous) {
                return ConversionMatch::Ambiguous;
            }
            return selection.candidate ? ConversionMatch::Unique : ConversionMatch::None;
        }

        ExpressionChecker::OverloadSelection
        ExpressionChecker::convertingConstructor(TypeId target, const ExprResult& argument) const {
            target = TypeSystem.unConst(target);
            std::vector<FunctionCandidate> candidates;
            const auto& record = TypeSystem.get(target);
            for (size_t index = 0; index < record.members.size(); ++index) {
                const auto& member = record.members[index];
                if (!member.isConstructor) {
                    continue;
                }
                const auto& signature = TypeSystem.get(member.type);
                // A converting constructor needs a typed first parameter and must be
                // callable with one argument. Trailing default parameters are allowed.
                if (signature.parameters.empty()) {
                    continue;
                }
                candidates.push_back({member.type, {}, member.minimumArity, target, static_cast<uint32_t>(index + 1)});
            }
            // Permit one user-defined conversion, never a recursive constructor
            // chain. Its own argument may still use a standard implicit conversion.
            return selectOverload(candidates, {argument}, false);
        }

        ExpressionChecker::OverloadSelection
        ExpressionChecker::selectOverload(const std::vector<FunctionCandidate>& input,
                                          const std::vector<ExprResult>& arguments, bool constructors) const {
            std::vector<FunctionCandidate> candidates;
            for (auto candidate : input) {
                if (!compatibleArity(candidate, arguments.size())) {
                    continue;
                }
                auto sameCallable = [&](const FunctionCandidate& previous) {
                    if (candidate.symbol.value || previous.symbol.value) {
                        return candidate.symbol.value && candidate.symbol == previous.symbol;
                    }
                    if (candidate.memberIndex || previous.memberIndex) {
                        return candidate.owner == previous.owner && candidate.memberIndex == previous.memberIndex;
                    }
                    return candidate.type == previous.type; // one indirect callee expression
                };
                if (std::none_of(candidates.begin(), candidates.end(), sameCallable)) {
                    candidates.push_back(candidate);
                }
            }
            std::vector<FunctionCandidate> exact;
            for (auto candidate : candidates) {
                const auto& signature = TypeSystem.get(candidate.type);
                // Ellipsis operands have no declared parameter type to match exactly.
                bool matches = arguments.size() <= signature.parameters.size();
                for (size_t i = 0; matches && i < arguments.size(); ++i) {
                    matches = !arguments[i].error && exactArgument(arguments[i].type, signature.parameters[i]);
                }
                if (matches) {
                    exact.push_back(candidate);
                }
            }
            if (!exact.empty()) {
                return {exact.size() == 1 ? std::optional{exact.front()} : std::nullopt, exact.size() > 1};
            }

            struct Viable {
                FunctionCandidate candidate;
                std::vector<ConstructorSelection> conversions;
            };

            std::vector<Viable> viable;
            bool ambiguousConversion = false;
            for (auto candidate : candidates) {
                const auto& signature = TypeSystem.get(candidate.type);
                bool matches = true, ambiguous = false;
                std::vector<ConstructorSelection> conversions(arguments.size());
                for (size_t i = 0; matches && i < arguments.size(); ++i) {
                    if (i >= signature.parameters.size()) {
                        matches = !arguments[i].error && isScalar(arguments[i].type);
                        continue;
                    }
                    auto conversion = implicitArgument(arguments[i], signature.parameters[i], false);
                    if (conversion == ConversionMatch::None && constructors &&
                        TypeSystem.isAggregate(signature.parameters[i])) {
                        auto selected = convertingConstructor(signature.parameters[i], arguments[i]);
                        if (selected.ambiguous) {
                            conversion = ConversionMatch::Ambiguous;
                        } else if (selected.candidate) {
                            conversion = ConversionMatch::Unique;
                            const auto& constructor = *selected.candidate;
                            conversions[i] = {constructor.owner, constructor.type, constructor.memberIndex, true, {}};
                        }
                    }
                    matches = conversion != ConversionMatch::None;
                    ambiguous |= conversion == ConversionMatch::Ambiguous;
                }
                if (matches) {
                    viable.push_back({candidate, std::move(conversions)});
                    ambiguousConversion |= ambiguous;
                }
            }
            if (viable.empty()) {
                return {};
            }
            if (viable.size() > 1 || ambiguousConversion) {
                return {{}, true};
            }
            return {viable.front().candidate, false, std::move(viable.front().conversions)};
        }
    }
}
