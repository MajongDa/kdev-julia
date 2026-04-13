#ifndef JULIA_EXPRESSIONVISITOR_H
#define JULIA_EXPRESSIONVISITOR_H

#include <language/duchain/builders/dynamiclanguageexpressionvisitor.h>

#include "parser/ast.h"
#include "parser/astvisitor.h"

namespace Julia {

class ExpressionVisitor : public AstVisitor, public KDevelop::DynamicLanguageExpressionVisitor
{
public:
    explicit ExpressionVisitor(const KDevelop::DUContext* ctx);
    ExpressionVisitor(ExpressionVisitor* parent, const KDevelop::DUContext* overrideContext = nullptr);

    void visitIdentifier(IdentifierAst* node) override;
    void visitCall(CallAst* node) override;
    void visitAttribute(AttributeAst* node) override;
    void visitBinaryOperation(BinaryOperationAst* node) override;
    void visitUnaryOperation(UnaryOperationAst* node) override;
    void visitString(StringAst* node) override;
    void visitNumber(NumberAst* node) override;
    void visitList(ListAst* node) override;
    void visitTuple(TupleAst* node) override;
    void visitDict(DictAst* node) override;
    void visitSubscript(SubscriptAst* node) override;

protected:
    KDevelop::AbstractType::Ptr unknownType() const override;
    KDevelop::AbstractType::Ptr encounterPreprocess(KDevelop::AbstractType::Ptr type) override;

    void processBinaryOperator(KDevelop::AbstractType::Ptr lhs, KDevelop::AbstractType::Ptr rhs, const QString& op);
};

}

#endif
