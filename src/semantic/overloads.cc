#include "expressions.hh"
#include <algorithm>

namespace baka::semantic {
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

    ExpressionChecker::ConversionMatch ExpressionChecker::implicitArgument(const ExprResult& argument, TypeId target,
                                                                           bool constructors) const {
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

    ExpressionChecker::OverloadSelection ExpressionChecker::convertingConstructor(TypeId target,
                                                                                  const ExprResult& argument) const {
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

    ExpressionChecker::OverloadSelection ExpressionChecker::selectOverload(const std::vector<FunctionCandidate>& input,
                                                                           const std::vector<ExprResult>& arguments,
                                                                           bool constructors) const {
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

        std::vector<FunctionCandidate> viable;
        bool ambiguousConversion = false;
        for (auto candidate : candidates) {
            const auto& signature = TypeSystem.get(candidate.type);
            bool matches = true, ambiguous = false;
            for (size_t i = 0; matches && i < arguments.size(); ++i) {
                if (i >= signature.parameters.size()) {
                    matches = !arguments[i].error && isScalar(arguments[i].type);
                    continue;
                }
                const auto conversion = implicitArgument(arguments[i], signature.parameters[i], constructors);
                matches = conversion != ConversionMatch::None;
                ambiguous |= conversion == ConversionMatch::Ambiguous;
            }
            if (matches) {
                viable.push_back(candidate);
                ambiguousConversion |= ambiguous;
            }
        }
        if (viable.empty()) {
            return {};
        }
        if (viable.size() > 1 || ambiguousConversion) {
            return {{}, true};
        }
        return {viable.front(), false};
    }
}
