// These tests inspect actual AST edges, rather than accepting a diagnostic-free
// compiler run as evidence that a conversion was materialized.
#include "semantic/semantic_analyzer.hh"
#include "lexer/lexer.hh"
#include "parser/parser.hh"
#include "driver/gctx.hh"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace baka;
using namespace baka::semantic;
using namespace baka::types;
namespace {
    void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
    template<class T> T* as(ExpressionNode* expression) {
        while (auto* cast = dynamic_cast<ImplicitCastNode*>(expression)) expression = cast->getOperand();
        if (auto* comma = dynamic_cast<CommaExpressionNode*>(expression); comma && comma->getExpressions().size() == 1)
            return as<T>(comma->getExpressions().front());
        return dynamic_cast<T*>(expression);
    }
    std::vector<ExpressionNode*> nodes(ExpressionNode* root) {
        std::vector<ExpressionNode*> result;
        std::function<void(ExpressionNode*)> visit = [&](ExpressionNode* expression) {
            if (!expression) return;
            result.push_back(expression);
            expression->rewriteExpressions([&](auto* child) { visit(child); return child; });
        };
        visit(root); return result;
    }
    std::vector<ImplicitCastNode*> casts(ExpressionNode* root, ImplicitConversionKind kind) {
        std::vector<ImplicitCastNode*> result;
        for (auto* node : nodes(root)) if (auto* cast = dynamic_cast<ImplicitCastNode*>(node); cast && cast->getConversionKind() == kind)
            result.push_back(cast);
        return result;
    }
    SingleDeclarationNode* declaration(CompoundStatementNode* body, size_t index) {
        return std::get<DeclarationList*>(body->getStatements().at(index))->getDeclarations().front();
    }
    ExpressionNode* statement(CompoundStatementNode* body, size_t index) {
        return dynamic_cast<ExpressionStatementNode*>(std::get<StatementNode*>(body->getStatements().at(index)))->getExpression();
    }
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "expected conversion scenario");
        std::string scenario = argv[1], source;
        if (scenario == "numeric") source =
            "double take(double x){return x;}int main(){short s=1;float f=2;double d=s+1+f;int i=d;"
            "s+=f;i=i+1;return take(i);}";
        else if (scenario == "contexts") source =
            "int main(){int a[3]={1,2,3};int i=0;int *p=a;i=sizeof(a);p=&a[0];"
            "if(p){i=1;}while(i;outer){break outer;}for(;i;i=i+1;L){continue L;}char text[3]=\"hi\";return i;}";
        else if (scenario == "calls") source =
            "double f(double x=1){return x;}int v(int first,...){return first;}"
            "int main(){double (*p)(double x)=f;float x=1;short s=2;p(s);f();v(1,x,s);f((1,2));return 0;}";
        else if (scenario == "compound") source =
            "int *get(int *p){return p;}int main(){int a[3]={0,1,2};int i=0;"
            "a[i++]+=2.5;*get(a)<<=1;return i;}";
        else if (scenario == "pointers") source =
            "int global=1;int *gp=&global;int main(){int a[2]={1,2};int *p=a;"
            "const int *q=p;void *v=p;int *n=0;const int c=1;const int *cp=&c;"
            "int b=p==0;int d=p!=q;int e=p?1:2.5;const int *r=p?q:p;return e;}";
        else if (scenario == "array_address") source =
            "int main(){int a[3]={1,2,3};int (*p)[3]=&a;return sizeof(*p);}";
        else if (scenario == "metadata") source =
            "class S{int x;S(int a){}int get(int a){return x+a;}};"
            "S make(){return S(1);}int main(){S s=S(1);int y=s.x;S t=make();return s.get(y);}";
        else throw std::runtime_error("unknown conversion scenario");
        driver::Gctx::AttachLineIndex(std::make_shared<base::LineIndex>(source));
        auto tokens = lexer::Tokenize(source);
        parser::Parser parser(tokens.Tokens, tokens.Locations);
        auto* program = dynamic_cast<ProgramNode*>(parser.Parse());
        require(program && !driver::Gctx::ErrorFound(), "conversion fixture did not parse");
        SemanticAnalyzer analyzer;
        require(analyzer.registerDeclarations(*program) && analyzer.resolveAndRename(), "fixture failed before annotation");
        auto* main = dynamic_cast<FunctionNode*>(program->getBody().back());
        auto* block = dynamic_cast<CompoundStatementNode*>(main->getBody());
        auto* originalReturn = dynamic_cast<ReturnStatementNode*>(std::get<StatementNode*>(block->getStatements().back()));
        require(casts(originalReturn->getExpression(), ImplicitConversionKind::LValueToRValue).empty(),
                "conversion nodes were inserted before annotation");
        if (!analyzer.annotateTypes()) {
            for (const auto& diagnostic : analyzer.diagnostics()) std::cerr << diagnostic.message << '\n';
            throw std::runtime_error("conversion annotation failed");
        }
        const auto intType = analyzer.types().builtin(Builtin::Int).value;
        const auto floatType = analyzer.types().builtin(Builtin::Float).value;
        const auto doubleType = analyzer.types().builtin(Builtin::Double).value;
        using K = ImplicitConversionKind;
        if (scenario == "numeric") {
            auto* initializer = declaration(block, 2)->getInitializer();
            auto promotions = casts(initializer, K::IntegralPromotion);
            require(promotions.size() == 1 && promotions.front()->getDestinationType() == intType,
                    "short arithmetic operand was not explicitly promoted");
            auto numeric = casts(initializer, K::NumericConversion);
            require(numeric.size() == 2, "mixed arithmetic/initializer numeric conversions missing");
            require(!casts(initializer, K::LValueToRValue).empty(), "variable read conversion missing");
            auto returnCasts = casts(originalReturn->getExpression(), K::NumericConversion);
            require(returnCasts.size() == 2, "argument and return conversions were not both inserted");
            auto* assignment = as<BinaryExpressionNode>(statement(block, 5));
            require(assignment && !dynamic_cast<ImplicitCastNode*>(assignment->getLeft()) && assignment->getLeft()->semanticInfo().LValue,
                    "assignment target was turned into a value");
        } else if (scenario == "contexts") {
            require(casts(declaration(block, 2)->getInitializer(), K::ArrayToPointer).size() == 1, "array decay missing");
            auto* measured = as<SizeofNode>(as<BinaryExpressionNode>(statement(block, 3))->getRight());
            require(measured && casts(std::get<ExpressionNode*>(measured->getPayload()), K::ArrayToPointer).empty(), "sizeof decayed its array operand");
            auto* address = as<FactorNode>(as<BinaryExpressionNode>(statement(block, 4))->getRight());
            require(address && !dynamic_cast<ImplicitCastNode*>(address->getOperand()), "address-of converted its target to a value");
            auto* text = declaration(block, 8)->getInitializer();
            auto* literal = dynamic_cast<InitializerNode*>(text)->expressions().front();
            require(literal->semanticInfo().Annotated && literal->semanticInfo().Type &&
                    analyzer.types().get({literal->semanticInfo().Type}).kind == TypeKind::Array &&
                    casts(text, K::ArrayToPointer).empty(), "string-array copy lost its annotated array operand");
            auto* branch = dynamic_cast<IfSuperBlockStatementNode*>(std::get<StatementNode*>(block->getStatements()[5]));
            require(casts(branch->getIf()->getCondition(), K::BooleanConversion).size() == 1, "pointer condition conversion missing");
            auto* loop = dynamic_cast<ForBlockStatementNode*>(std::get<StatementNode*>(block->getStatements()[7]));
            require(casts(loop->getCondition(), K::BooleanConversion).size() == 1, "for condition conversion missing");
            auto* jump = std::get<StatementNode*>(dynamic_cast<CompoundStatementNode*>(loop->getBody())->getStatements().front());
            require(jump->semanticInfo().Target == loop->semanticInfo().Target, "rewriting lost the named-loop target");
        } else if (scenario == "calls") {
            require(casts(declaration(block, 0)->getInitializer(), K::FunctionToPointer).size() == 1, "function decay missing");
            auto* call = as<FunctionCallPostfixExpr>(statement(block, 3));
            require(casts(call->getCallee(), K::LValueToRValue).size() == 1, "function-pointer callee was not loaded");
            require(casts(call->getCallee(), K::FunctionToPointer).empty(), "function pointer was decayed as a function");
            require(casts(call->getArguments(), K::NumericConversion).size() == 1, "fixed argument conversion missing");
            auto* variadic = as<FunctionCallPostfixExpr>(statement(block, 5));
            auto promotions = casts(variadic->getArguments(), K::IntegralPromotion);
            auto floats = casts(variadic->getArguments(), K::NumericConversion);
            require(promotions.size() == 1 && promotions[0]->getDestinationType() == intType && floats.size() == 1 &&
                    floats[0]->getSourceType() == floatType && floats[0]->getDestinationType() == doubleType,
                    "ellipsis default argument promotions missing");
            auto* f = dynamic_cast<FunctionNode*>(program->getBody().front());
            auto* parameter = dynamic_cast<FunctionParameterList*>(f->getDeclarator()->getFunctionParameters())->getParameters().front();
            require(casts(parameter->getDeclaration()->getInitializer(), K::NumericConversion).size() == 1,
                    "default parameter conversion missing");
            auto* commaCall = as<FunctionCallPostfixExpr>(statement(block, 6));
            auto* args = dynamic_cast<CommaExpressionNode*>(commaCall->getArguments());
            require(args && args->getExpressions().size() == 1, "rewriting flattened a parenthesized comma argument");
        } else if (scenario == "compound") {
            auto* compound = as<CompoundAssignmentNode>(statement(block, 2));
            require(compound && !dynamic_cast<ImplicitCastNode*>(compound->getTarget()), "compound assignment target missing");
            auto loadNodes = nodes(compound->getValue());
            size_t loads = 0, increments = 0;
            for (auto* node : loadNodes) { loads += dynamic_cast<CompoundTargetNode*>(node) != nullptr; increments += dynamic_cast<UnaryPostfixExpr*>(node) != nullptr; }
            require(loads == 1 && increments == 0, "compound computation repeats its target side effect");
            require(casts(compound->getValue(), K::NumericConversion).size() == 2, "compound read/store numeric conversions missing");
            auto* shift = as<CompoundAssignmentNode>(statement(block, 3));
            size_t calls = 0; for (auto* node : nodes(shift)) calls += dynamic_cast<FunctionCallPostfixExpr*>(node) != nullptr;
            require(calls == 1, "compound target call was duplicated");
        } else if (scenario == "pointers") {
            require(casts(declaration(block, 2)->getInitializer(), K::QualificationConversion).size() == 1, "pointer qualification missing");
            require(casts(declaration(block, 3)->getInitializer(), K::PointerConversion).size() == 1, "void-pointer conversion missing");
            require(casts(declaration(block, 4)->getInitializer(), K::NullToPointer).size() == 1, "null pointer conversion missing");
            require(casts(declaration(block, 7)->getInitializer(), K::NullToPointer).size() == 1, "null comparison operand conversion missing");
            require(casts(declaration(block, 8)->getInitializer(), K::QualificationConversion).size() == 1, "pointer comparison common qualification missing");
            auto* ternary = as<TernaryOpNode>(dynamic_cast<InitializerNode*>(declaration(block, 9)->getInitializer())->expressions().front());
            require(ternary && ternary->getThen()->semanticInfo().Type == doubleType && ternary->getElse()->semanticInfo().Type == doubleType,
                    "conditional branches did not receive their common type");
            require(casts(declaration(block, 10)->getInitializer(), K::QualificationConversion).size() == 1,
                    "pointer conditional dropped a branch qualifier");
        }
        if (scenario == "array_address") {
            auto* initializer = declaration(block, 1)->getInitializer();
            auto* address = as<FactorNode>(dynamic_cast<InitializerNode*>(initializer)->expressions().front());
            require(address && address->getUnaryOp() == ASTUnaryOp::OP_ADDR_OF, "array address node missing");
            const auto& pointer = analyzer.types().get({address->semanticInfo().Type});
            require(pointer.kind == TypeKind::Pointer, "array address is not a pointer");
            const auto& array = analyzer.types().get(pointer.element);
            require(array.kind == TypeKind::Array && array.bound.count == 3, "array address lost its bound/type");
            require(casts(initializer, K::ArrayToPointer).empty(), "address operand incorrectly decayed");
            auto* size = as<SizeofNode>(originalReturn->getExpression());
            require(size && size->semanticInfo().HasConstant && size->semanticInfo().Constant == 12,
                    "sizeof pointer-to-array dereference lost the complete array type");
        }
        if (scenario == "metadata") {
            auto* construction = as<FunctionCallPostfixExpr>(dynamic_cast<InitializerNode*>(declaration(block, 0)->getInitializer())->expressions().front());
            auto info = construction->semanticInfo();
            require(info.Construction && info.CallSignature && info.MemberOwner && info.MemberIndex,
                    "constructor choice did not survive on the AST");
            const auto& ctor = analyzer.types().get({info.MemberOwner}).members.at(info.MemberIndex - 1);
            require(ctor.isConstructor && ctor.type.value == info.CallSignature, "constructor metadata selects the wrong declaration");
            auto* member = as<MemberPostfixExpr>(dynamic_cast<InitializerNode*>(declaration(block, 1)->getInitializer())->expressions().front());
            info = member->semanticInfo();
            require(info.MemberOwner && info.MemberIndex && info.Symbol, "field identity is missing");
            auto layout = analyzer.types().layout({info.MemberOwner});
            require(layout && layout.layout.offsets.at(info.MemberIndex - 1).has_value(), "field offset cannot be queried without a name lookup");
            auto* make = as<FunctionCallPostfixExpr>(dynamic_cast<InitializerNode*>(declaration(block, 2)->getInitializer())->expressions().front());
            require(!make->semanticInfo().Construction && casts(make->getCallee(), K::FunctionToPointer).size() == 1,
                    "ordinary aggregate-returning call was mistaken for a constructor");
            auto* call = as<FunctionCallPostfixExpr>(originalReturn->getExpression());
            info = call->semanticInfo();
            require(info.CallSignature && info.MemberOwner && info.MemberIndex && info.Symbol,
                    "selected instance method signature/owner was not attached");
        }
        for (const auto& item : block->getStatements()) std::visit([&](auto* itemNode) {
            ASTNode* owner = itemNode;
            std::vector<ExpressionNode*> roots;
            if (auto* declarations = dynamic_cast<DeclarationList*>(owner)) {
                for (auto* decl : declarations->getDeclarations()) roots.push_back(decl->getInitializer());
            } else owner->rewriteExpressions([&](auto* child) { roots.push_back(child); return child; });
            for (auto* root : roots) for (auto* node : nodes(root)) if (auto* cast = dynamic_cast<ImplicitCastNode*>(node)) {
                require(cast->getSourceType() == cast->getOperand()->semanticInfo().Type, "cast source disagrees with operand type");
                require(cast->getDestinationType() == cast->semanticInfo().Type && !cast->semanticInfo().LValue,
                        "cast destination metadata is inconsistent");
            }
        }, item);
        // Read-only checks must preserve the rewritten edges and metadata, not
        // merely leave the root's type unchanged.
        std::vector<std::pair<ExpressionNode*, ASTSemanticInfo>> before;
        for (auto* node : nodes(originalReturn->getExpression())) before.emplace_back(node, node->semanticInfo());
        const auto types = analyzer.types().typeCount();
        const auto symbols = analyzer.symbols().symbolCount();
        if (!analyzer.check()) {
            for (const auto& diagnostic : analyzer.diagnostics()) std::cerr << diagnostic.message << '\n';
            throw std::runtime_error("checks rejected conversion fixture");
        }
        auto after = nodes(originalReturn->getExpression());
        require(after.size() == before.size(), "checks changed rewritten AST shape");
        for (size_t i = 0; i < after.size(); ++i)
            require(after[i] == before[i].first && after[i]->semanticInfo() == before[i].second, "checks mutated a conversion node");
        require(types == analyzer.types().typeCount() && symbols == analyzer.symbols().symbolCount(), "checks mutated semantic arenas");
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
