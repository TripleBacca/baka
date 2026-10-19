#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>


namespace baka {
namespace types {

class ExpressionNode;
using ExpressionRewriter = std::function<ExpressionNode*(ExpressionNode*)>;

struct ASTSourceSpan {
    size_t Line = 0;
    size_t Col = 0;
    bool Valid = false;
};

// IDs belong to the SemanticAnalyzer that analyzed this tree. The parser leaves
// them zeroed. Declaration/type resolution fills bindings; annotation fills the
// expression fields. Checks and later lowering only read this record.
struct ASTSemanticInfo {
    uint32_t Type = 0, Symbol = 0, Scope = 0, Target = 0;
    bool Resolved = false, Annotated = false, Error = false, LValue = false;
    bool ConstantExpression = false, HasConstant = false, NullPointerConstant = false;
    int64_t Constant = 0;
    bool HasFloatingConstant = false;
    long double FloatingConstant = 0;
    // CallSignature is the selected function type, also for indirect calls and
    // constructors. MemberIndex is one-based (zero means no selected member).
    // layout(MemberOwner).offsets[MemberIndex-1] supplies a field's byte offset.
    uint32_t CallSignature = 0, MemberOwner = 0, MemberIndex = 0;
    bool Construction = false;
    bool operator==(const ASTSemanticInfo&) const = default;
};

class ASTNode {
    bool hasError_ = false;
    ASTSourceSpan Source;
    ASTSemanticInfo Semantic;
public:
    ASTNode() = default;
    virtual ~ASTNode() = default;

    virtual void Print(size_t Tabs = 0) const = 0;

    // Rewrites direct expression slots only. Semantic annotation supplies the
    // recursion and context; declaration/statement discovery is not repeated.
    virtual void rewriteExpressions(const ExpressionRewriter&) {}

    virtual void setHasError() {
        hasError_ = true;
    }

    bool hasError() const {
        return hasError_;
    }

    void setSourceSpan(size_t Line, size_t Col) {
        Source = {Line, Col, true};
    }

    const ASTSourceSpan& getSourceSpan() const {
        return Source;
    }

    ASTSemanticInfo& semanticInfo() { return Semantic; }
    const ASTSemanticInfo& semanticInfo() const { return Semantic; }

};
}
}
