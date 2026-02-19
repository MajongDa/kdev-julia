#include "expressionvisitor.h"

#include <QString>

#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/types/integraltype.h>
#include <language/duchain/types/structuretype.h>
#include <language/duchain/types/functiontype.h>
#include <language/duchain/types/containertypes.h>
#include <language/duchain/types/arraytype.h>
#include <language/duchain/types/unsuretype.h>
#include <language/duchain/declaration.h>

#include "../parser/ast.h"
#include "../types/types.h"
#include "helpers.h"
#include "juliadebug.h"

namespace Julia {

static bool isOperatorSymbol(const QString& text)
{
    static const QSet<QString> operators = {
        QStringLiteral("+"), QStringLiteral("-"), QStringLiteral("*"), QStringLiteral("/"), QStringLiteral("^"), QStringLiteral("%"), 
        QStringLiteral("=="), QStringLiteral("!="), QStringLiteral("<"), QStringLiteral(">"), QStringLiteral("<="), QStringLiteral(">="),
        QStringLiteral("&&"), QStringLiteral("||"), 
        QStringLiteral(">>"), QStringLiteral("<<"), QStringLiteral(">>>"), QStringLiteral("<<<"),
        QStringLiteral("=>"), QStringLiteral("->"), 
        QStringLiteral("|"), QStringLiteral("&"), QStringLiteral("~"), QStringLiteral("$"),
        QStringLiteral("÷"), QStringLiteral("∈"), QStringLiteral("∉"), QStringLiteral("⊆"), QStringLiteral("⊂"), QStringLiteral("⊇"), QStringLiteral("⊃")
    };
    return operators.contains(text);
}

// TODO: NothingType for Julia
// Current: Using IntegralType::TypeVoid which doesn't properly represent Julia's semantics
// - `nothing`: Singleton representing absence of value (like Python's None)
// - `missing`: Singleton representing missing value (like Nullable in other languages)
// Need custom type to:
// 1. Override toString() to show "nothing" or "missing"
// 2. Enable proper type checking in code completion
// See Python's NoneType for reference (kdev-python/duchain/types/nonetype.h)

ExpressionVisitor::ExpressionVisitor(const KDevelop::DUContext* ctx)
    : KDevelop::DynamicLanguageExpressionVisitor(ctx)
{
}

ExpressionVisitor::ExpressionVisitor(ExpressionVisitor* parent, const KDevelop::DUContext* overrideContext)
    : KDevelop::DynamicLanguageExpressionVisitor(parent)
{
    if (overrideContext) {
        m_context = overrideContext;
    }
}

KDevelop::AbstractType::Ptr ExpressionVisitor::unknownType() const
{
    return KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed));
}

KDevelop::AbstractType::Ptr ExpressionVisitor::encounterPreprocess(KDevelop::AbstractType::Ptr type)
{
    return Helper::resolveAliasType(type);
}

void ExpressionVisitor::visitIdentifier(AstNode* node)
{
    if (!node || node->kind() != NodeKind::Identifier) {
        return;
    }

    QString name = node->text();
    
    // Skip operator symbols - JuliaSyntax parses operators like "+", "-", "*" as Identifier nodes
    // They should not trigger identifier resolution
    // TODO: Once parser/ast.cpp operator mapping is implemented, this check may no longer be needed
    // because operators will have NodeKind::Operator instead of NodeKind::Identifier
    if (isOperatorSymbol(name)) {
        return;
    }
    
    if (name.isEmpty()) {
        encounterUnknown();
        return;
    }

    KDevelop::QualifiedIdentifier id(name);
    
    KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
    
    QList<KDevelop::Declaration*> declarations = context()->findDeclarations(id);
    
    qCDebug(KDEV_JULIA) << "  ExpressionVisitor: Looking up identifier:" << id.toString() << "found" << declarations.size() << "declarations";
    
    if (!declarations.isEmpty()) {
        KDevelop::Declaration* decl = declarations.first();
        QString declType = decl->abstractType() ? decl->abstractType()->toString() : QStringLiteral("none");
        qCDebug(KDEV_JULIA) << "  ExpressionVisitor: Found declaration:" << decl->identifier().toString() << "type:" << declType;
        if (decl && decl->abstractType()) {
            encounter(decl->abstractType(), KDevelop::DeclarationPointer(decl));
            return;
        }
    }
    
    qCDebug(KDEV_JULIA) << "  ExpressionVisitor: No declaration found for:" << id.toString();
    setConfident(false);
    encounterUnknown();
}

void ExpressionVisitor::visitCall(AstNode* node)
{
    if (!node) {
        return;
    }

    qCDebug(KDEV_JULIA) << ">>> ExpressionVisitor::visitCall:" << node->range();
    qCDebug(KDEV_JULIA) << "  Context:" << m_context;

    if (!m_context) {
        qCDebug(KDEV_JULIA) << "  ERROR: No context, returning unknown";
        encounterUnknown();
        return;
    }

    AstNode* funcNode = node->firstChild();
    if (!funcNode) {
        qCDebug(KDEV_JULIA) << "  No function node, returning unknown";
        encounterUnknown();
        return;
    }

    ExpressionVisitor funcVisitor(this);
    funcVisitor.visitNode(funcNode);

    auto funcType = funcVisitor.lastType();
    auto functionDecl = funcVisitor.lastDeclaration();
    
    QString funcTypeStr = funcType ? funcType->toString() : QStringLiteral("none");
    QString funcDeclStr = functionDecl ? functionDecl->identifier().toString() : QStringLiteral("none");
    qCDebug(KDEV_JULIA) << "  Function type:" << funcTypeStr;
    qCDebug(KDEV_JULIA) << "  Function declaration:" << funcDeclStr;

    if (funcType) {
        if (auto ft = funcType.dynamicCast<KDevelop::FunctionType>()) {
            QString retTypeStr = ft->returnType() ? ft->returnType()->toString() : QStringLiteral("none");
            qCDebug(KDEV_JULIA) << "  Returning function type, return type:" << retTypeStr;
            encounter(ft->returnType());
            return;
        }
        if (auto st = funcType.dynamicCast<KDevelop::StructureType>()) {
            // Constructor call like Point(1, 2) - return the struct type (instance of this type)
            qCDebug(KDEV_JULIA) << "  Returning structure type (constructor):" << st->toString();
            encounter(st);
            return;
        }
    }

    qCDebug(KDEV_JULIA) << "  Returning unknown";
    encounterUnknown();
}

void ExpressionVisitor::visitOperator(AstNode* node)
{
    if (!node) {
        return;
    }
    
    QString op = node->text().trimmed();
    
    if (op == QLatin1String("+") || op == QLatin1String("-") || 
        op == QLatin1String("*") || op == QLatin1String("/") ||
        op == QLatin1String("^") || op == QLatin1String("%")) {
        auto intType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed));
        encounter(intType);
        return;
    }
    
    if (op == QLatin1String("==") || op == QLatin1String("!=") ||
        op == QLatin1String("<") || op == QLatin1String(">") ||
        op == QLatin1String("<=") || op == QLatin1String(">=") ||
        op == QLatin1String("&&") || op == QLatin1String("||")) {
        auto boolType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeBoolean));
        encounter(boolType);
        return;
    }
    
    encounterUnknown();
}

void ExpressionVisitor::visitString(AstNode* node)
{
    // TODO: String interpolation support
    // Current: Returns String type for all strings
    // Problem: "$x and $y" should infer type from interpolated variables
    // - Parse string content to find $variable or $(expression) patterns
    // - Visit each interpolated expression to get its type
    // - If mixed types, return UnsureType; if all same, return that type
    // See Python's string formatting handling for reference
    
    if (!node) {
        return;
    }
    
    auto stringType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeString));
    encounter(stringType);
}

void ExpressionVisitor::visitFloat(AstNode* node)
{
    if (!node) {
        return;
    }
    
    auto floatType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed));
    encounter(floatType);
}

void ExpressionVisitor::visitInteger(AstNode* node)
{
    if (!node) {
        return;
    }
    
    auto intType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeInt));
    encounter(intType);
}

void ExpressionVisitor::visitBool(AstNode* node)
{
    if (!node) {
        return;
    }
    
    auto boolType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeBoolean));
    encounter(boolType);
}

void ExpressionVisitor::visitTuple(AstNode* node)
{
    // TODO: TupleType for Julia
    // Current: Using UnsureType for heterogeneous tuples
    // Problem: UnsureType doesn't preserve order of element types
    // - (Int, String, Float64) should be distinguishable from (String, Int, Float64)
    // Custom TupleType would preserve ordered list of element types
    
    if (!node) {
        encounterUnknown();
        return;
    }
    
    QList<AstNode*> children = node->children();
    if (children.isEmpty()) {
        encounterUnknown();
        return;
    }
    
    QList<KDevelop::AbstractType::Ptr> elementTypes;
    for (AstNode* child : children) {
        if (!child) continue;
        ExpressionVisitor elemVisitor(this);
        elemVisitor.visitNode(child);
        if (elemVisitor.lastType()) {
            elementTypes.append(elemVisitor.lastType());
        }
    }
    
    if (elementTypes.isEmpty()) {
        encounterUnknown();
        return;
    }
    
    if (elementTypes.size() == 1) {
        encounter(elementTypes.first());
        return;
    }
    
    auto* unsure = new KDevelop::UnsureType();
    for (const auto& t : elementTypes) {
        unsure->addType(t->indexed());
    }
    encounter(KDevelop::AbstractType::Ptr(unsure));
}

void ExpressionVisitor::visitArray(AstNode* node)
{
    if (!node) {
        encounterUnknown();
        return;
    }
    
    QList<AstNode*> children = node->children();
    if (children.isEmpty()) {
        encounterUnknown();
        return;
    }
    
    QList<KDevelop::AbstractType::Ptr> elementTypes;
    for (AstNode* child : children) {
        if (!child) continue;
        ExpressionVisitor elemVisitor(this);
        elemVisitor.visitNode(child);
        if (elemVisitor.lastType()) {
            elementTypes.append(elemVisitor.lastType());
        }
    }
    
    if (elementTypes.isEmpty()) {
        encounterUnknown();
        return;
    }
    
    KDevelop::AbstractType::Ptr commonType = elementTypes.first();
    bool allSame = true;
    for (const auto& t : elementTypes) {
        if (!t || !t->equals(commonType.data())) {
            allSame = false;
            break;
        }
    }
    
    if (allSame) {
        auto* arrayType = new KDevelop::ArrayType();
        arrayType->setElementType(commonType);
        encounter(KDevelop::AbstractType::Ptr(arrayType));
    } else {
        auto* unsure = new KDevelop::UnsureType();
        for (const auto& t : elementTypes) {
            unsure->addType(t->indexed());
        }
        auto* arrayType = new KDevelop::ArrayType();
        arrayType->setElementType(KDevelop::AbstractType::Ptr(unsure));
        encounter(KDevelop::AbstractType::Ptr(arrayType));
    }
}

void ExpressionVisitor::visitDict(AstNode* node)
{
    // TODO: Full Dict/Map type support
    // Current: Falling back to UnsureType for dict literals
    // Problem: Dict(:a => 1, :b => 2) should show key/value types
    
    if (!node) {
        encounterUnknown();
        return;
    }
    
    QList<AstNode*> children = node->children();
    if (children.isEmpty()) {
        encounterUnknown();
        return;
    }
    
    setConfident(false);
    encounterUnknown();
}

void ExpressionVisitor::visitTypeAnnotation(AstNode* node)
{
    // TypeAnnotation node: "x::Type" 
    // Children: [identifier, type]
    // Last child is the type node
    
    if (!node) {
        encounterUnknown();
        return;
    }
    
    AstNode* typeNode = node->lastChild();
    if (!typeNode) {
        encounterUnknown();
        return;
    }
    
    // Delegate based on node kind
    switch (typeNode->kind()) {
        case NodeKind::Identifier: {
            // x::Int - look up in context (try user type first, then builtin)
            QString typeName = typeNode->text();
            
            // Try to find user-defined type in context first
            KDevelop::Declaration* decl = Helper::declarationForName(typeName, typeNode->range().start, context());
            if (decl && decl->abstractType()) {
                encounter(decl->abstractType(), KDevelop::DeclarationPointer(decl));
                return;
            }
            
            // Fall back to TypeMapper for builtins
            // Note: TypeMapper uses context for user type lookup, which won't work for const context
            // But for builtins it doesn't need the context
            KDevelop::AbstractType* type = TypeMapper::typeFromString(typeName, nullptr);
            if (type) {
                encounter(KDevelop::AbstractType::Ptr(type));
                return;
            }
            
            encounterUnknown();
            break;
        }
        
        case NodeKind::Curly:
            // x::Array{Int} or x::Dict{String, Float64}
            // Delegate to visitCurly which handles parametric types
            visitCurly(typeNode);
            break;
        
        case NodeKind::Dot:
            // x::Base.Vector - module qualified type
            // TODO: Handle module-qualified types like Base.Vector
            setConfident(false);
            encounterUnknown();
            break;
        
        default:
            setConfident(false);
            encounterUnknown();
            break;
    }
}

void ExpressionVisitor::visitDot(AstNode* node)
{
    if (!node) {
        encounterUnknown();
        return;
    }
    
    QList<AstNode*> children = node->children();
    if (children.size() < 2) {
        encounterUnknown();
        return;
    }
    
    AstNode* lhs = children.first();
    AstNode* attr = children.last();
    if (!lhs || !attr) {
        encounterUnknown();
        return;
    }
    
    ExpressionVisitor lhsVisitor(this);
    lhsVisitor.visitNode(lhs);
    
    if (!lhsVisitor.lastType()) {
        encounterUnknown();
        return;
    }
    
    KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
    KDevelop::Declaration* decl = Helper::accessAttribute(lhsVisitor.lastType(), attr->text(), topContext());
    
    if (decl && decl->abstractType()) {
        encounter(decl->abstractType(), KDevelop::DeclarationPointer(decl));
        return;
    }
    
    setConfident(false);
    encounterUnknown();
}

void ExpressionVisitor::visitCurly(AstNode* node)
{
    if (!node) {
        encounterUnknown();
        return;
    }
    
    QList<AstNode*> children = node->children();
    if (children.isEmpty()) {
        encounterUnknown();
        return;
    }
    
    AstNode* baseType = children.first();
    if (!baseType) {
        encounterUnknown();
        return;
    }
    
    QString baseName = baseType->text();
    
    QList<KDevelop::AbstractType::Ptr> typeParams;
    for (int i = 1; i < children.size(); i++) {
        if (!children[i]) continue;
        ExpressionVisitor paramVisitor(this);
        paramVisitor.visitNode(children[i]);
        if (paramVisitor.lastType()) {
            typeParams.append(paramVisitor.lastType());
        }
    }
    
    if (baseName == QLatin1String("Array") && !typeParams.isEmpty()) {
        auto* arrayType = new KDevelop::ArrayType();
        arrayType->setElementType(typeParams.first());
        encounter(KDevelop::AbstractType::Ptr(arrayType));
        return;
    }
    
    if (baseName == QLatin1String("Dict") && typeParams.size() >= 2) {
        auto* mapType = new KDevelop::MapType();
        mapType->replaceKeyType(typeParams[0]);
        mapType->replaceContentType(typeParams[1]);
        encounter(KDevelop::AbstractType::Ptr(mapType));
        return;
    }
    
    if (baseName == QLatin1String("Tuple") && !typeParams.isEmpty()) {
        auto* unsure = new KDevelop::UnsureType();
        for (const auto& t : typeParams) {
            unsure->addType(t->indexed());
        }
        encounter(KDevelop::AbstractType::Ptr(unsure));
        return;
    }
    
    KDevelop::StructureType::Ptr structType(new KDevelop::StructureType());
    encounter(structType);
}

void ExpressionVisitor::visitBinaryOperation(AstNode* node)
{
    if (!node) {
        encounterUnknown();
        return;
    }
    
    QList<AstNode*> children = node->children();
    if (children.size() < 3) {
        encounterUnknown();
        return;
    }
    
    AstNode* lhs = children[0];
    AstNode* op = children[1];
    AstNode* rhs = children[2];
    
    ExpressionVisitor lhsVisitor(this);
    ExpressionVisitor rhsVisitor(this);
    
    lhsVisitor.visitNode(lhs);
    rhsVisitor.visitNode(rhs);
    
    if (op) {
        processBinaryOperator(lhsVisitor.lastType(), rhsVisitor.lastType(), op->text());
    } else {
        encounterUnknown();
    }
}

void ExpressionVisitor::visitUnaryOperation(AstNode* node)
{
    if (!node) {
        encounterUnknown();
        return;
    }
    
    QList<AstNode*> children = node->children();
    if (children.size() < 2) {
        encounterUnknown();
        return;
    }
    
    AstNode* operand = children[1];
    ExpressionVisitor operandVisitor(this);
    operandVisitor.visitNode(operand);
    
    QString op = children.size() > 0 ? children[0]->text() : QString();
    
    if (op == QLatin1String("-") || op == QLatin1String("+")) {
        encounter(operandVisitor.lastType());
        return;
    }
    if (op == QLatin1String("!")) {
        auto boolType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeBoolean));
        encounter(boolType);
        return;
    }
    
    encounterUnknown();
}

void ExpressionVisitor::processBinaryOperator(KDevelop::AbstractType::Ptr lhs, KDevelop::AbstractType::Ptr rhs, const QString& op)
{
    if (op == QLatin1String("+") || op == QLatin1String("-") || 
        op == QLatin1String("*") || op == QLatin1String("/") ||
        op == QLatin1String("^") || op == QLatin1String("%")) {
        
        if (lhs && rhs) {
            encounter(lhs);
            return;
        }
        auto mixedType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed));
        encounter(mixedType);
        return;
    }
    
    if (op == QLatin1String("==") || op == QLatin1String("!=") ||
        op == QLatin1String("<") || op == QLatin1String(">") ||
        op == QLatin1String("<=") || op == QLatin1String(">=")) {
        auto boolType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeBoolean));
        encounter(boolType);
        return;
    }
    
    encounterUnknown();
}

}
