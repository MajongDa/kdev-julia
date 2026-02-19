#ifndef JULIA_EXPRESSIONVISITOR_H
#define JULIA_EXPRESSIONVISITOR_H

#include <language/duchain/builders/dynamiclanguageexpressionvisitor.h>

#include "../parser/ast.h"
#include "astvisitor.h"

namespace Julia {

class ExpressionVisitor : public AstVisitor, public KDevelop::DynamicLanguageExpressionVisitor
{
public:
    explicit ExpressionVisitor(const KDevelop::DUContext* ctx);
    ExpressionVisitor(ExpressionVisitor* parent, const KDevelop::DUContext* overrideContext = nullptr);

    void visitIdentifier(AstNode* node) override;
    void visitCall(CallNode* node) override;
    void visitOperator(AstNode* node) override;
    void visitString(AstNode* node) override;
    void visitFloat(AstNode* node) override;
    void visitInteger(AstNode* node) override;
    void visitBool(AstNode* node) override;
    void visitTuple(AstNode* node) override;
    void visitArray(AstNode* node) override;
    void visitDict(AstNode* node) override;
    void visitTypeAnnotation(AstNode* node) override;
    void visitDot(AstNode* node) override;
    void visitCurly(CurlyNode* node) override;
    void visitBinaryOperation(AstNode* node);
    void visitUnaryOperation(AstNode* node);

protected:
    KDevelop::AbstractType::Ptr unknownType() const override;
    KDevelop::AbstractType::Ptr encounterPreprocess(KDevelop::AbstractType::Ptr type) override;

    void processBinaryOperator(KDevelop::AbstractType::Ptr lhs, KDevelop::AbstractType::Ptr rhs, const QString& op);
};

}

#endif
