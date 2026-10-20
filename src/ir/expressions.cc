#include "lowering.hh"
#include "types/parser/ast/castFactorNode.hh"
#include "types/parser/ast/ternary.hh"
#include "types/parser/ast/sizeof.hh"
#include <algorithm>

namespace baka::ir {
    Operand Lowerer::constant(const types::ConstantNode* node, TypeId target) {
        if (auto* literal = dynamic_cast<const types::ConstantTNode<int>*>(node)) {
            return integer(target, static_cast<uint64_t>(literal->getValue()));
        }
        if (auto* literal = dynamic_cast<const types::ConstantTNode<char>*>(node)) {
            return integer(target, static_cast<uint64_t>(literal->getValue()));
        }
        if (auto* literal = dynamic_cast<const types::ConstantTNode<long long>*>(node)) {
            return integer(target, static_cast<uint64_t>(literal->getValue()));
        }
        if (auto* literal = dynamic_cast<const types::ConstantTNode<unsigned int>*>(node)) {
            return integer(target, static_cast<uint64_t>(literal->getValue()));
        }
        if (auto* literal = dynamic_cast<const types::ConstantTNode<unsigned long long>*>(node)) {
            return integer(target, static_cast<uint64_t>(literal->getValue()));
        }
        if (auto* literal = dynamic_cast<const types::ConstantTNode<bool>*>(node)) {
            return integer(target, static_cast<uint64_t>(literal->getValue()));
        }

        if (auto* literal = dynamic_cast<const types::ConstantTNode<float>*>(node)) {
            return {OperandKind::Floating, target, 0, 0, literal->getValue()};
        }
        if (auto* literal = dynamic_cast<const types::ConstantTNode<double>*>(node)) {
            return {OperandKind::Floating, target, 0, 0, literal->getValue()};
        }
        if (dynamic_cast<const types::ConstantTNode<std::nullptr_t>*>(node)) {
            return integer(target, 0);
        }
        std::string bytes;
        if (auto* literal = dynamic_cast<const types::ConstantTNode<base::SLString>*>(node)) {
            bytes = literal->getValue();
        } else if (auto* literal = dynamic_cast<const types::ConstantTNode<std::string>*>(node)) {
            bytes = literal->getValue();
        } else {
            fail(node, "unsupported literal in IR lowering");
        }
        bytes.push_back('\0');
        Object string;
        string.id = NextObject++;
        string.name = ".string";
        string.type = target;
        string.global = string.internal = string.readOnly = true;
        string.bytes = std::move(bytes);
        auto id = string.id;
        Result.globals.push_back(std::move(string));
        return {OperandKind::Object, Result.pointer(target), id};
    }

    Operand Lowerer::self() {
        if (SelfOverride) {
            return SelfOverride;
        }
        if (!Self) {
            fail(nullptr, "bound member expression has no active receiver");
        }
        auto location = object(Self);
        return load(location, Result.type(location.type).element);
    }

    Operand Lowerer::memberAddress(const types::ASTNode* node, Operand receiver, semantic::TypeId receiverType) {
        const auto& metadata = node->semanticInfo();
        if (!metadata.MemberOwner || !metadata.MemberIndex) {
            fail(node, "member expression lacks selected-member metadata");
        }
        const auto& owner = Analyzer.types().get({metadata.MemberOwner});
        const auto& member = owner.members.at(metadata.MemberIndex - 1);
        if (member.isStatic) {
            return object(member.symbol.value);
        }
        auto source = Analyzer.types().unConst(receiverType);
        if (Analyzer.types().get(source).kind == semantic::TypeKind::Pointer) {
            source = Analyzer.types().unConst(Analyzer.types().get(source).element);
        }
        uint64_t offset = 0;
        while (source.value != metadata.MemberOwner) {
            const auto& current = Analyzer.types().get(source);
            auto layout = Analyzer.types().layout(source);
            if (!current.base.value || !layout || !layout.layout.baseOffset) {
                fail(node, "cannot compute inherited member receiver adjustment");
            }
            offset += *layout.layout.baseOffset;
            source = Analyzer.types().unConst(current.base);
        }
        auto layout = Analyzer.types().layout({metadata.MemberOwner});
        if (!layout || !layout.layout.offsets.at(metadata.MemberIndex - 1)) {
            fail(node, "selected member has no object offset");
        }
        offset += *layout.layout.offsets[metadata.MemberIndex - 1];
        return emitValue(Opcode::AddressOffset, Result.pointer(importType(member.type)), receiver,
                         integer(integerType(), offset));
    }

    Operand Lowerer::address(const types::ExpressionNode* expression) {
        if (!expression) {
            fail(expression, "missing lvalue in IR lowering");
        }
        const auto& info = expression->semanticInfo();
        if (!info.LValue && Result.type(type(expression)).kind == TypeKind::Record) {
            // Member access on a returned record needs addressable storage. This
            // materialization is local IR storage, never a hidden call argument.
            auto captured = value(expression);
            if (auto found = ValueLocations.find(captured.id);
                captured.kind == OperandKind::Temporary && found != ValueLocations.end()) {
                return found->second;
            }
            Object storage;
            storage.id = NextObject++;
            storage.name = ".materialized";
            storage.type = captured.type;
            auto id = storage.id;
            function().objects.push_back(std::move(storage));
            auto location = object(id);
            emit(Opcode::Store, location, captured);
            return location;
        }
        // Assignment and prefix mutation are lvalues in baka. Their outer
        // LValueToRValue conversion must evaluate the write once, then load the
        // saved location, rather than trying to recover an address from a value.
        if (auto* conversion = dynamic_cast<const types::ImplicitCastNode*>(expression);
            conversion && conversion->getConversionKind() == types::ImplicitConversionKind::LValueToRValue) {
            return address(conversion->getOperand());
        }
        if (auto* assignment = dynamic_cast<const types::BinaryExpressionNode*>(expression);
            assignment && assignment->getBinaryOp() == types::OP_ASSIGN) {
            auto location = address(assignment->getLeft());
            auto result = value(assignment->getRight());
            emit(Opcode::Store, location, result);
            return location;
        }
        if (auto* assignment = dynamic_cast<const types::CompoundAssignmentNode*>(expression)) {
            auto location = address(assignment->getTarget());
            SavedTargets.push_back(location);
            auto result = value(assignment->getValue());
            SavedTargets.pop_back();
            emit(Opcode::Store, location, result);
            return location;
        }
        if (auto* prefix = dynamic_cast<const types::FactorNode*>(expression);
            prefix &&
            (prefix->getUnaryOp() == types::ASTUnaryOp::OP_INC || prefix->getUnaryOp() == types::ASTUnaryOp::OP_DEC)) {
            return increment(prefix->getOperand(), prefix->getUnaryOp() == types::ASTUnaryOp::OP_DEC, false, true);
        }
        if (auto* ternary = dynamic_cast<const types::TernaryOpNode*>(expression)) {
            auto condition = value(ternary->getCondition());
            auto yes = newBlock(), no = newBlock(), join = newBlock();
            auto pointer = Result.pointer(type(expression));
            auto result = temporary(pointer);
            branch(condition, yes, no);
            select(yes);
            auto left = address(ternary->getThen());
            emit(Opcode::Copy, result, cast(left, pointer));
            jump(join);
            select(no);
            auto right = address(ternary->getElse());
            emit(Opcode::Copy, result, cast(right, pointer));
            jump(join);
            select(join);
            return result;
        }
        if (auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression)) {
            const auto& payload = primary->getExpression();
            if (auto* name = std::get_if<types::IdentifierNode*>(&payload)) {
                auto symbol = (*name)->semanticInfo().Symbol;
                if (!symbol) {
                    symbol = info.Symbol;
                }
                if (auto member = Members.find(symbol); member != Members.end() && !member->second.isStatic) {
                    const auto& selected =
                        Analyzer.types().get({member->second.owner}).members[member->second.index - 1];
                    if (Analyzer.types().get(Analyzer.types().unConst(selected.type)).kind ==
                        semantic::TypeKind::Function) {
                        fail(expression, "an instance method cannot escape as an ordinary function pointer");
                    }
                    auto receiver = self();
                    return memberAddress(expression, receiver, CurrentOwner);
                }
                const auto& selected = Analyzer.symbols().get({symbol});
                if (selected.kind == semantic::SymbolKind::Function ||
                    selected.kind == semantic::SymbolKind::BuiltinFunction) {
                    return callable(symbol);
                }
                return object(symbol);
            }
            if (auto* literal = std::get_if<types::ConstantNode*>(&payload)) {
                auto result = constant(*literal, type(expression));
                if (result.kind == OperandKind::Object) {
                    return result;
                }
            }
        }
        if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression)) {
            const auto& expressions = comma->getExpressions();
            if (expressions.empty()) {
                fail(expression, "empty comma expression has no address");
            }
            for (size_t i = 0; i + 1 < expressions.size(); ++i) {
                value(expressions[i]);
            }
            return address(expressions.back());
        }
        if (auto* target = dynamic_cast<const types::CompoundTargetNode*>(expression)) {
            if (SavedTargets.empty()) {
                fail(target, "compound target used outside its assignment");
            }
            return SavedTargets.back();
        }
        if (auto* unary = dynamic_cast<const types::FactorNode*>(expression);
            unary && unary->getUnaryOp() == types::ASTUnaryOp::OP_DEREF) {
            return value(unary->getOperand());
        }
        if (auto* index = dynamic_cast<const types::IndexPostfixExpr*>(expression)) {
            auto base = value(index->getBase());
            auto offset = value(index->getIndex());
            const auto& element = Result.type(type(expression));
            offset = cast(offset, integerType());
            offset = emitValue(Opcode::Multiply, integerType(), offset, integer(integerType(), element.size));
            return emitValue(Opcode::AddressOffset, Result.pointer(type(expression)), base, offset);
        }
        if (auto* member = dynamic_cast<const types::MemberPostfixExpr*>(expression)) {
            auto base = address(member->getBase());
            return memberAddress(expression, base, {member->getBase()->semanticInfo().Type});
        }
        if (auto* member = dynamic_cast<const types::ArrowPostfixExpr*>(expression)) {
            auto base = value(member->getBase());
            return memberAddress(expression, base, {member->getBase()->semanticInfo().Type});
        }
        fail(expression, "expression has no supported IR address");
    }

    Operand Lowerer::binary(types::ASTBinaryOp opcode, Operand left, Operand right, TypeId result) {
        auto leftType = Result.type(left.type), rightType = Result.type(right.type);
        if ((opcode == types::OP_ADD || opcode == types::OP_SUB) &&
            (leftType.kind == TypeKind::Pointer || rightType.kind == TypeKind::Pointer)) {
            if (leftType.kind == TypeKind::Pointer && rightType.kind == TypeKind::Pointer) {
                auto lhs = cast(left, integerType()), rhs = cast(right, integerType());
                auto bytes = emitValue(Opcode::Subtract, integerType(), lhs, rhs);
                auto size = Result.type(leftType.element).size;
                if (!size) {
                    fail(nullptr, "pointer difference has an unsized pointee");
                }
                return cast(emitValue(Opcode::Divide, integerType(), bytes, integer(integerType(), size)), result);
            }
            auto pointer = leftType.kind == TypeKind::Pointer ? left : right;
            auto index = leftType.kind == TypeKind::Pointer ? right : left;
            auto element = Result.type(Result.type(pointer.type).element);
            if (!element.size) {
                fail(nullptr, "pointer arithmetic has an unsized pointee");
            }
            auto bytes = emitValue(Opcode::Multiply, integerType(), cast(index, integerType()),
                                   integer(integerType(), element.size));
            if (opcode == types::OP_SUB) {
                bytes = emitValue(Opcode::Negate, integerType(), bytes);
            }
            return emitValue(Opcode::AddressOffset, result, pointer, bytes);
        }
        Opcode operation;
        switch (opcode) {
        case types::OP_ADD:
            operation = Opcode::Add;
            break;
        case types::OP_SUB:
            operation = Opcode::Subtract;
            break;
        case types::OP_MUL:
            operation = Opcode::Multiply;
            break;
        case types::OP_DIV:
            operation = Opcode::Divide;
            break;
        case types::OP_MOD:
            operation = Opcode::Remainder;
            break;
        case types::OP_LSHIFT:
            operation = Opcode::ShiftLeft;
            break;
        case types::OP_RSHIFT:
            operation = Opcode::ShiftRight;
            break;
        case types::OP_BIT_AND:
            operation = Opcode::BitAnd;
            break;
        case types::OP_BIT_OR:
            operation = Opcode::BitOr;
            break;
        case types::OP_BIT_XOR:
            operation = Opcode::BitXor;
            break;
        case types::OP_EQ:
            operation = Opcode::Equal;
            break;
        case types::OP_NEQ:
            operation = Opcode::NotEqual;
            break;
        case types::OP_LT:
            operation = Opcode::Less;
            break;
        case types::OP_GT:
            operation = Opcode::Greater;
            break;
        case types::OP_LE:
            operation = Opcode::LessEqual;
            break;
        case types::OP_GE:
            operation = Opcode::GreaterEqual;
            break;

        default:
            fail(nullptr, "unmaterialized or unsupported binary operator in IR");
        }
        return emitValue(operation, result, left, right);
    }

    Operand Lowerer::increment(const types::ExpressionNode* target, bool decrement, bool postfix, bool returnAddress) {
        auto location = address(target);
        auto targetType = type(target);
        auto old = load(location, targetType);
        auto info = Result.type(targetType);
        Operand next;
        if (info.kind == TypeKind::Pointer) {
            auto delta = Result.type(info.element).size;
            if (!delta) {
                fail(target, "pointer increment has an unsized pointee");
            }
            next = emitValue(Opcode::AddressOffset, targetType, old,
                             integer(integerType(), decrement ? uint64_t{0} - delta : delta));
        } else {
            // Use the frontend's integral-promotion rule before the writeback.
            // For bool this is essential: converting 2 to bool yields true,
            // whereas doing addition directly in a u1 temporary would wrap to 0.
            auto computation = info.kind == TypeKind::Integer && info.bits < 32
                                   ? importType(Analyzer.types().builtin(semantic::Builtin::Int))
                                   : targetType;
            auto promoted = cast(old, computation);
            auto one = info.kind == TypeKind::Floating ? Operand{OperandKind::Floating, computation, 0, 0, 1}
                                                       : integer(computation, 1);
            next = cast(emitValue(decrement ? Opcode::Subtract : Opcode::Add, computation, promoted, one), targetType);
        }
        emit(Opcode::Store, location, next);
        return returnAddress ? location : postfix ? old : next;
    }

    Operand Lowerer::receiver(const types::ExpressionNode* callee, uint32_t owner) {
        if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(callee)) {
            const auto& expressions = comma->getExpressions();
            if (expressions.empty()) {
                fail(callee, "missing bound callee");
            }
            for (size_t i = 0; i + 1 < expressions.size(); ++i) {
                value(expressions[i]);
            }
            return receiver(expressions.back(), owner);
        }
        Operand result;
        semantic::TypeId source;
        if (auto* dot = dynamic_cast<const types::MemberPostfixExpr*>(callee)) {
            result = address(dot->getBase());
            source = {dot->getBase()->semanticInfo().Type};
        } else if (auto* arrow = dynamic_cast<const types::ArrowPostfixExpr*>(callee)) {
            result = value(arrow->getBase());
            source = {arrow->getBase()->semanticInfo().Type};
            source = Analyzer.types().get(Analyzer.types().unConst(source)).element;
        } else {
            result = self();
            source = CurrentOwner;
        }
        source = Analyzer.types().unConst(source);
        uint64_t offset = 0;
        while (source.value != owner) {
            auto layout = Analyzer.types().layout(source);
            const auto& current = Analyzer.types().get(source);
            if (!current.base.value || !layout || !layout.layout.baseOffset) {
                fail(callee, "cannot adjust bound method receiver");
            }
            offset += *layout.layout.baseOffset;
            source = Analyzer.types().unConst(current.base);
        }
        auto target = Result.pointer(importType({owner}));
        if (offset) {
            return emitValue(Opcode::AddressOffset, target, result, integer(integerType(), offset));
        }
        return cast(result, target);
    }

    Operand Lowerer::call(const types::FunctionCallPostfixExpr* node) {
        const auto& metadata = node->semanticInfo();
        if (metadata.Construction) {
            auto target = type(node);
            auto location = destination(node);
            const bool direct = bool(location);
            if (!location) {
                location = storage(target, ".constructed");
            }
            std::vector<const types::ExpressionNode*> arguments;
            if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(node->getArguments())) {
                arguments.assign(comma->getExpressions().begin(), comma->getExpressions().end());
            } else if (node->getArguments()) {
                arguments.push_back(node->getArguments());
            }
            auto selected = metadata.MemberIndex
                                ? SpecialFunctions.at(
                                      {Analyzer.types().unConst({metadata.MemberOwner}).value, metadata.MemberIndex})
                                : 0;
            // Value-initialization through an implicit default constructor zeros
            // scalar subobjects before applying default member initializers.
            if (!selected) {
                zeroObject(location, target);
            }
            if (selected || needsConstruction(target)) {
                construct(location, target, selected, arguments);
            }
            auto captured = load(location, target);
            ValueLocations[captured.id] = location;
            return direct ? captured : ownTemporary(captured, location);
        }
        if (!metadata.CallSignature) {
            fail(node, "call lacks semantic signature metadata");
        }
        std::vector<const types::ExpressionNode*> expressions;
        if (auto* arguments = dynamic_cast<const types::CommaExpressionNode*>(node->getArguments())) {
            expressions.assign(arguments->getExpressions().begin(), arguments->getExpressions().end());
        } else if (node->getArguments()) {
            expressions.push_back(node->getArguments());
        }
        const auto& semanticSignature = Analyzer.types().get({metadata.CallSignature});
        // Defaults retain declaration-point bindings on their checked AST.
        // Evaluate them at this call site after explicit arguments, without
        // cloning or mutating either tree or semantic symbol tables.
        while (expressions.size() < semanticSignature.parameters.size()) {
            const types::ExpressionNode* argument = nullptr;
            if (metadata.Symbol) {
                const auto& defaults = Analyzer.symbols().get({metadata.Symbol}).defaultArguments;
                if (expressions.size() < defaults.size()) {
                    argument = static_cast<const types::ExpressionNode*>(defaults[expressions.size()]);
                }
            }
            if (!argument) {
                fail(node, "selected call has no checked default argument for an omitted parameter");
            }
            expressions.push_back(argument);
        }
        Operand callee;
        std::vector<Operand> arguments;
        if (metadata.MemberOwner && metadata.MemberIndex) {
            const auto& selected = Analyzer.types().get({metadata.MemberOwner}).members[metadata.MemberIndex - 1];
            if (!selected.isStatic &&
                Analyzer.types().get(Analyzer.types().unConst(selected.type)).kind == semantic::TypeKind::Function) {
                auto instance = receiver(node->getCallee(), metadata.MemberOwner);
                arguments.push_back(instance);
                callee = callable(metadata.Symbol);
            }
        }
        if (!callee) {
            callee = value(node->getCallee());
        }
        // Capture values before the Argument sequence: nested calls cannot steal
        // an outer call's arguments, and later side effects cannot reload them.
        for (size_t index = 0; index < expressions.size(); ++index) {
            auto argument = value(expressions[index]);
            if (index < semanticSignature.parameters.size()) {
                transfer(argument);
            }
            arguments.push_back(argument);
        }
        for (size_t i = 0; i < arguments.size(); ++i) {
            emit(Opcode::Argument, {}, arguments[i], integer(integerType(), i));
        }
        auto resultType = importType(semanticSignature.element);
        auto result = Result.type(resultType).kind == TypeKind::Void ? Operand{} : temporary(resultType);
        emit(Opcode::Call, result, callee, integer(integerType(), arguments.size()));
        if (result && needsDestruction(result.type)) {
            if (auto location = destination(node)) {
                emit(Opcode::Store, location, result);
            } else {
                result = ownTemporary(result);
            }
        }
        return result;
    }

    Operand Lowerer::value(const types::ExpressionNode* expression) {
        if (!expression) {
            return {};
        }
        if (!BuildingExpression) {
            auto found = ExpressionRegions.find(expression);
            if (found == ExpressionRegions.end()) {
                fail(expression, "expression was not collected by the expression lowering pass");
            }
            auto& cursor = RegionCursor[expression];
            if (cursor >= found->second.size()) {
                fail(expression, "expression region has no remaining evaluation");
            }
            auto& region = found->second[cursor++];
            if (region.connected) {
                fail(expression, "expression region was connected more than once");
            }
            link(region.entry);
            select(region.exit);
            region.connected = true;
            PendingTemps.insert(PendingTemps.end(), region.cleanups.begin(), region.cleanups.end());
            return region.value;
        }
        auto span = expression->getSourceSpan();
        if (span.Valid) {
            Source = {span.Line, span.Col};
        }
        const auto resultType = type(expression);
        if (auto* castNode = dynamic_cast<const types::ImplicitCastNode*>(expression)) {
            using C = types::ImplicitConversionKind;
            switch (castNode->getConversionKind()) {
            case C::LValueToRValue:
                return load(address(castNode->getOperand()), resultType);
            case C::ArrayToPointer:
                return cast(address(castNode->getOperand()), resultType);
            case C::FunctionToPointer:
                return cast(value(castNode->getOperand()), resultType);
            default:
                return cast(value(castNode->getOperand()), resultType);
            }
        }
        if (auto* compound = dynamic_cast<const types::CompoundAssignmentNode*>(expression)) {
            auto target = address(compound->getTarget());
            SavedTargets.push_back(target);
            auto result = value(compound->getValue());
            SavedTargets.pop_back();
            emit(Opcode::Store, target, result);
            return result;
        }
        if (dynamic_cast<const types::CompoundTargetNode*>(expression)) {
            return load(address(expression), resultType);
        }
        if (auto* comma = dynamic_cast<const types::CommaExpressionNode*>(expression)) {
            Operand result;
            for (auto* child : comma->getExpressions()) {
                result = value(child);
            }
            return result;
        }
        if (auto* initializer = dynamic_cast<const types::InitializerNode*>(expression)) {
            if (initializer->expressions().empty()) {
                return integer(resultType, 0);
            }
            if (initializer->expressions().size() != 1) {
                fail(expression, "aggregate initializer used as a runtime value");
            }
            return value(initializer->expressions().front());
        }
        if (auto* primary = dynamic_cast<const types::PrimaryExpressionNode*>(expression)) {
            const auto& payload = primary->getExpression();
            if (auto* literal = std::get_if<types::ConstantNode*>(&payload)) {
                auto result = constant(*literal, resultType);
                if (result.kind == OperandKind::Object) {
                    return cast(result, Result.pointer(Result.type(resultType).element));
                }
                return result;
            }
            if (std::holds_alternative<types::ThisNode*>(payload)) {
                return self();
            }
            auto symbol = std::get<types::IdentifierNode*>(payload)->semanticInfo().Symbol;
            if (!symbol) {
                symbol = expression->semanticInfo().Symbol;
            }
            const auto& selected = Analyzer.symbols().get({symbol});
            if (selected.kind == semantic::SymbolKind::Enumerator) {
                return integer(resultType, static_cast<uint64_t>(*selected.constant));
            }
            if (selected.kind == semantic::SymbolKind::Function ||
                selected.kind == semantic::SymbolKind::BuiltinFunction) {
                if (auto member = Members.find(symbol); member != Members.end() && !member->second.isStatic) {
                    fail(expression, "bound instance methods can only be lowered in call context");
                }
                return callable(symbol);
            }
            return load(address(expression), resultType);
        }
        if (auto* binaryNode = dynamic_cast<const types::BinaryExpressionNode*>(expression)) {
            auto opcode = binaryNode->getBinaryOp();
            if (opcode == types::OP_ASSIGN) {
                auto location = address(binaryNode->getLeft());
                auto result = value(binaryNode->getRight());
                emit(Opcode::Store, location, result);
                return result;
            }
            auto left = value(binaryNode->getLeft());
            if (opcode == types::OP_LOGICAL_AND || opcode == types::OP_LOGICAL_OR) {
                auto rightBlock = newBlock(), shortBlock = newBlock(), join = newBlock();
                auto result = temporary(resultType);
                bool conjunction = opcode == types::OP_LOGICAL_AND;
                branch(left, conjunction ? rightBlock : shortBlock, conjunction ? shortBlock : rightBlock);
                select(shortBlock);
                emit(Opcode::Copy, result, integer(resultType, conjunction ? 0 : 1));
                jump(join);
                select(rightBlock);
                auto right = value(binaryNode->getRight());
                emit(Opcode::Copy, result, cast(right, resultType));
                jump(join);
                select(join);
                return result;
            }
            auto right = value(binaryNode->getRight());
            if (opcode == types::OP_COMMA) {
                return right;
            }
            return binary(opcode, left, right, resultType);
        }
        if (auto* unary = dynamic_cast<const types::FactorNode*>(expression)) {
            using U = types::ASTUnaryOp;
            auto opcode = unary->getUnaryOp();
            if (opcode == U::OP_ADDR_OF) {
                return cast(address(unary->getOperand()), resultType);
            }
            if (opcode == U::OP_DEREF) {
                auto pointer = value(unary->getOperand());
                return Result.type(resultType).kind == TypeKind::Function ? pointer : load(pointer, resultType);
            }
            if (opcode == U::OP_INC || opcode == U::OP_DEC) {
                return increment(unary->getOperand(), opcode == U::OP_DEC, false);
            }
            auto operand = value(unary->getOperand());
            if (opcode == U::OP_UNARY_PLUS || opcode == U::OP_NOOP) {
                return operand;
            }
            if (opcode == U::OP_UNARY_MINUS) {
                return emitValue(Opcode::Negate, resultType, operand);
            }
            if (opcode == U::OP_TILDE) {
                return emitValue(Opcode::Complement, resultType, operand);
            }
            if (opcode == U::OP_LOGICAL_NOT) {
                return emitValue(Opcode::LogicalNot, resultType, operand);
            }
            fail(expression, "unsupported unary operator in IR");
        }
        if (auto* postfix = dynamic_cast<const types::UnaryPostfixExpr*>(expression)) {
            return increment(postfix->getOperand(), postfix->getOp() == types::ASTUnaryOp::OP_DEC, true);
        }
        if (auto* ternary = dynamic_cast<const types::TernaryOpNode*>(expression)) {
            Operand direct;
            if (Result.type(resultType).kind == TypeKind::Record) {
                if (auto location = destination(ternary)) {
                    direct = location;
                    DirectDestinations[valueRoot(ternary->getThen())] = location;
                    DirectDestinations[valueRoot(ternary->getElse())] = location;
                }
            }
            auto condition = value(ternary->getCondition());
            auto yes = newBlock(), no = newBlock(), join = newBlock();
            auto result = Result.type(resultType).kind == TypeKind::Void ? Operand{} : temporary(resultType);
            auto ownedAddress =
                needsDestruction(resultType) && !direct ? temporary(Result.pointer(resultType)) : Operand{};
            branch(condition, yes, no);
            select(yes);
            auto left = value(ternary->getThen());
            if (ownedAddress) {
                if (ValueOwners[left.id].empty()) {
                    left = ownTemporary(left);
                }
                emit(Opcode::Copy, ownedAddress, ValueLocations.at(left.id));
            }
            if (result) {
                emit(Opcode::Copy, result, left);
            }
            jump(join);
            select(no);
            auto right = value(ternary->getElse());
            if (ownedAddress) {
                if (ValueOwners[right.id].empty()) {
                    right = ownTemporary(right);
                }
                emit(Opcode::Copy, ownedAddress, ValueLocations.at(right.id));
            }
            if (result) {
                emit(Opcode::Copy, result, right);
            }
            jump(join);
            select(join);
            if (result) {
                if (ownedAddress) {
                    ValueLocations[result.id] = ownedAddress;
                } else if (direct) {
                    ValueLocations[result.id] = direct;
                }
                auto& owners = ValueOwners[result.id];
                if (left.kind == OperandKind::Temporary) {
                    owners = ValueOwners[left.id];
                }
                if (right.kind == OperandKind::Temporary) {
                    const auto other = ValueOwners[right.id];
                    owners.insert(owners.end(), other.begin(), other.end());
                }
            }
            return result;
        }
        if (auto* explicitCast = dynamic_cast<const types::CastFactorNode*>(expression)) {
            auto operand = value(explicitCast->getExpression());
            auto result = cast(operand, resultType);
            if (Result.type(resultType).kind == TypeKind::Record) {
                if (auto location = destination(explicitCast)) {
                    emit(Opcode::Store, location, result);
                    transfer(result);
                    ValueLocations[result.id] = location;
                } else if (needsDestruction(resultType) && ValueOwners[result.id].empty()) {
                    result = ownTemporary(result);
                }
            }
            return result;
        }
        if (auto* size = dynamic_cast<const types::SizeofNode*>(expression)) {
            if (!size->semanticInfo().HasConstant) {
                fail(size, "sizeof lacks a semantic constant size");
            }
            return integer(resultType, static_cast<uint64_t>(size->semanticInfo().Constant));
        }
        if (auto* callNode = dynamic_cast<const types::FunctionCallPostfixExpr*>(expression)) {
            return call(callNode);
        }
        if (dynamic_cast<const types::MemberPostfixExpr*>(expression) ||
            dynamic_cast<const types::ArrowPostfixExpr*>(expression)) {
            auto symbol = expression->semanticInfo().Symbol;
            if (symbol && Result.functionById(symbol)) {
                if (auto* dot = dynamic_cast<const types::MemberPostfixExpr*>(expression)) {
                    address(dot->getBase());
                }
                if (auto* arrow = dynamic_cast<const types::ArrowPostfixExpr*>(expression)) {
                    value(arrow->getBase());
                }
                auto binding = Members.at(symbol);
                if (!binding.isStatic) {
                    fail(expression, "bound instance methods require a call context");
                }
                return callable(symbol);
            }
            return load(address(expression), resultType);
        }
        if (dynamic_cast<const types::IndexPostfixExpr*>(expression)) {
            return load(address(expression), resultType);
        }
        fail(expression, "unsupported expression node in IR lowering");
    }
}
