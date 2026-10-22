#pragma once
#include "expression.hh"

namespace baka::types {
    // Semantic-only nodes: the parser never constructs these. Type IDs use the
    // same arena as ASTSemanticInfo::Type; keep that arena alive while lowering.
    enum class ImplicitConversionKind {
        LValueToRValue, ArrayToPointer, FunctionToPointer,
        IntegralPromotion, NumericConversion, BooleanConversion,
        NullToPointer, PointerConversion, QualificationConversion, ConstructorConversion
    };

    inline const char* ImplicitConversionName(ImplicitConversionKind kind) {
        switch (kind) {
        case ImplicitConversionKind::LValueToRValue: return "LValueToRValue";
        case ImplicitConversionKind::ArrayToPointer: return "ArrayToPointer";
        case ImplicitConversionKind::FunctionToPointer: return "FunctionToPointer";
        case ImplicitConversionKind::IntegralPromotion: return "IntegralPromotion";
        case ImplicitConversionKind::NumericConversion: return "NumericConversion";
        case ImplicitConversionKind::BooleanConversion: return "BooleanConversion";
        case ImplicitConversionKind::NullToPointer: return "NullToPointer";
        case ImplicitConversionKind::PointerConversion: return "PointerConversion";
        case ImplicitConversionKind::QualificationConversion: return "QualificationConversion";
        case ImplicitConversionKind::ConstructorConversion: return "ConstructorConversion";
        }
        return "Unknown";
    }

    class ImplicitCastNode : public ExpressionNode {
        ExpressionNode* Operand;
        uint32_t SourceType, DestinationType;
        ImplicitConversionKind Kind;
    public:
        ImplicitCastNode(ExpressionNode* operand, uint32_t source, uint32_t destination,
                         ImplicitConversionKind kind)
            : Operand(operand), SourceType(source), DestinationType(destination), Kind(kind) {}
        ExpressionNode* getOperand() const { return Operand; }
        uint32_t getSourceType() const { return SourceType; }
        uint32_t getDestinationType() const { return DestinationType; }
        ImplicitConversionKind getConversionKind() const { return Kind; }
            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Operand = rewrite(Operand); }
            void visitExpressions(const ExpressionVisitor& visit) const override { visit(Operand); }
        void Print(size_t tabs = 0) const override {
            INDENT(tabs);
            std::cout << "ImplicitCast(" << ImplicitConversionName(Kind) << ", "
                      << SourceType << " -> " << DestinationType << ")\n";
            if (Operand) Operand->Print(tabs + 1);
        }
    };

    // A compound assignment evaluates Target's address exactly once. Its Value
    // subtree uses this placeholder to read that saved address, never to repeat
    // evaluation of Target (which may contain calls or index increments).
    class CompoundTargetNode : public ExpressionNode {
    public:
        void visitExpressions(const ExpressionVisitor&) const override {}
    public:
        void Print(size_t tabs = 0) const override {
            INDENT(tabs); std::cout << "CompoundTarget(saved address)\n";
        }
    };
    class CompoundAssignmentNode : public ExpressionNode {
        ExpressionNode* Target;
        ExpressionNode* Value;
    public:
        CompoundAssignmentNode(ExpressionNode* target, ExpressionNode* value) : Target(target), Value(value) {}
        ExpressionNode* getTarget() const { return Target; }
        ExpressionNode* getValue() const { return Value; }
            void rewriteExpressions(const ExpressionRewriter& rewrite) override { Target = rewrite(Target); Value = rewrite(Value); }
            void visitExpressions(const ExpressionVisitor& visit) const override { visit(Target); visit(Value); }
        void Print(size_t tabs = 0) const override {
            INDENT(tabs); std::cout << "CompoundAssignment(\n";
            Target->Print(tabs + 1); Value->Print(tabs + 1);
            INDENT(tabs); std::cout << ")\n";
        }
    };
}
