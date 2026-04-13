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

void ExpressionVisitor::visitIdentifier(IdentifierAst* node)
{
    if (!node || node->astType != AstType::IdentifierAstType) {
        return;
    }

    if (!node) {
        encounterUnknown();
        return;
    }

    QString name = node->value;
    
    if (isOperatorSymbol(name)) {
        return;
    }
    
    if (name.isEmpty()) {
        encounterUnknown();
        return;
    }

    KDevelop::CursorInRevision searchPos = node->range().start;
    
    qCDebug(KDEV_JULIA) << "  ExpressionVisitor: Looking up identifier:" << name 
                         << "in context:" << context() << "at position:" << searchPos;
    
    KDevelop::Declaration* decl = Helper::declarationForName(name, searchPos, context());
    
    if (decl) {
        QString declType = decl->abstractType() ? decl->abstractType()->toString() : QStringLiteral("none");
        qCDebug(KDEV_JULIA) << "  ExpressionVisitor: Found declaration:" << decl->identifier().toString() 
                            << "type:" << declType << "range:" << decl->range();
        if (decl && decl->abstractType()) {
            encounter(decl->abstractType(), KDevelop::DeclarationPointer(decl));
            return;
        }
    }
    
    qCDebug(KDEV_JULIA) << "  ExpressionVisitor: No declaration found for:" << name;
    setConfident(false);
    encounterUnknown();
}

void ExpressionVisitor::visitCall(CallAst* node)
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

    Ast* funcNode = node->function;
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
            qCDebug(KDEV_JULIA) << "  Returning structure type (constructor):" << st->toString();
            encounter(st);
            return;
        }
    }

    qCDebug(KDEV_JULIA) << "  Returning unknown";
    encounterUnknown();
}

void ExpressionVisitor::visitAttribute(AttributeAst* node)
{
    if (!node || !node->value) {
        encounterUnknown();
        return;
    }
    
    ExpressionVisitor valueVisitor(this);
    valueVisitor.visitNode(node->value);
    
    if (!valueVisitor.lastType()) {
        encounterUnknown();
        return;
    }
    
    if (node->attribute) {
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        KDevelop::Declaration* decl = Helper::accessAttribute(valueVisitor.lastType(), node->attribute->value, topContext());
        
        if (decl && decl->abstractType()) {
            encounter(decl->abstractType(), KDevelop::DeclarationPointer(decl));
            return;
        }
    }
    
    setConfident(false);
    encounterUnknown();
}

static QString binaryOpToString(BinaryOperationAst::Operator op)
{
    switch (op) {
        case BinaryOperationAst::Operator::Add: return QStringLiteral("+");
        case BinaryOperationAst::Operator::Sub: return QStringLiteral("-");
        case BinaryOperationAst::Operator::Mul: return QStringLiteral("*");
        case BinaryOperationAst::Operator::Div: return QStringLiteral("/");
        case BinaryOperationAst::Operator::FloorDiv: return QStringLiteral("÷");
        case BinaryOperationAst::Operator::Mod: return QStringLiteral("%");
        case BinaryOperationAst::Operator::Pow: return QStringLiteral("^");
        case BinaryOperationAst::Operator::And: return QStringLiteral("&&");
        case BinaryOperationAst::Operator::Or: return QStringLiteral("||");
        case BinaryOperationAst::Operator::Eq: return QStringLiteral("==");
        case BinaryOperationAst::Operator::Ne: return QStringLiteral("!=");
        case BinaryOperationAst::Operator::Lt: return QStringLiteral("<");
        case BinaryOperationAst::Operator::Le: return QStringLiteral("<=");
        case BinaryOperationAst::Operator::Gt: return QStringLiteral(">");
        case BinaryOperationAst::Operator::Ge: return QStringLiteral(">=");
        default: return QString();
    }
}

static QString unaryOpToString(UnaryOperationAst::Operator op)
{
    switch (op) {
        case UnaryOperationAst::Operator::UAdd: return QStringLiteral("+");
        case UnaryOperationAst::Operator::USub: return QStringLiteral("-");
        case UnaryOperationAst::Operator::Not: return QStringLiteral("!");
        case UnaryOperationAst::Operator::Invert: return QStringLiteral("~");
        default: return QString();
    }
}

void ExpressionVisitor::visitBinaryOperation(BinaryOperationAst* node)
{
    if (!node || !node->left || !node->right) {
        encounterUnknown();
        return;
    }
    
    ExpressionVisitor lhsVisitor(this);
    ExpressionVisitor rhsVisitor(this);
    
    lhsVisitor.visitNode(node->left);
    rhsVisitor.visitNode(node->right);
    
    QString opStr = binaryOpToString(node->op);
    
    if (!opStr.isEmpty()) {
        processBinaryOperator(lhsVisitor.lastType(), rhsVisitor.lastType(), opStr);
    } else {
        encounterUnknown();
    }
}

void ExpressionVisitor::visitUnaryOperation(UnaryOperationAst* node)
{
    if (!node || !node->operand) {
        encounterUnknown();
        return;
    }
    
    ExpressionVisitor operandVisitor(this);
    operandVisitor.visitNode(node->operand);
    
    QString opStr = unaryOpToString(node->op);
    
    if (opStr == QStringLiteral("-") || opStr == QStringLiteral("+")) {
        encounter(operandVisitor.lastType());
        return;
    }
    if (opStr == QStringLiteral("!")) {
        auto boolType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeBoolean));
        encounter(boolType);
        return;
    }
    
    encounterUnknown();
}

void ExpressionVisitor::visitString(StringAst* node)
{
    auto stringType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeString));
    encounter(stringType);
}

void ExpressionVisitor::visitNumber(NumberAst* node)
{
    auto numType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed));
    encounter(numType);
}

void ExpressionVisitor::visitList(ListAst* node)
{
    if (!node || node->elements.isEmpty()) {
        encounterUnknown();
        return;
    }
    
    QList<KDevelop::AbstractType::Ptr> elementTypes;
    for (auto* elem : node->elements) {
        if (!elem) continue;
        ExpressionVisitor elemVisitor(this);
        elemVisitor.visitNode(elem);
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

void ExpressionVisitor::visitTuple(TupleAst* node)
{
    if (!node || node->elements.isEmpty()) {
        encounterUnknown();
        return;
    }
    
    QList<KDevelop::AbstractType::Ptr> elementTypes;
    for (auto* elem : node->elements) {
        if (!elem) continue;
        ExpressionVisitor elemVisitor(this);
        elemVisitor.visitNode(elem);
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

void ExpressionVisitor::visitDict(DictAst* node)
{
    if (!node || node->keys.isEmpty()) {
        encounterUnknown();
        return;
    }
    
    setConfident(false);
    encounterUnknown();
}

void ExpressionVisitor::visitSubscript(SubscriptAst* node)
{
    if (!node || !node->value) {
        encounterUnknown();
        return;
    }
    
    ExpressionVisitor valueVisitor(this);
    valueVisitor.visitNode(node->value);
    
    auto baseType = valueVisitor.lastType();
    if (!baseType) {
        encounterUnknown();
        return;
    }
    
    if (auto arrayType = baseType.dynamicCast<KDevelop::ArrayType>()) {
        encounter(arrayType->elementType());
        return;
    }
    
    setConfident(false);
    encounterUnknown();
}

void ExpressionVisitor::processBinaryOperator(KDevelop::AbstractType::Ptr lhs, KDevelop::AbstractType::Ptr rhs, const QString& op)
{
    if (op == QStringLiteral("+") || op == QStringLiteral("-") || 
        op == QStringLiteral("*") || op == QStringLiteral("/") ||
        op == QStringLiteral("^") || op == QStringLiteral("%")) {
        
        if (lhs && rhs) {
            encounter(lhs);
            return;
        }
        auto mixedType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed));
        encounter(mixedType);
        return;
    }
    
    if (op == QStringLiteral("==") || op == QStringLiteral("!=") ||
        op == QStringLiteral("<") || op == QStringLiteral(">") ||
        op == QStringLiteral("<=") || op == QStringLiteral(">=")) {
        auto boolType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeBoolean));
        encounter(boolType);
        return;
    }
    
    encounterUnknown();
}

}
