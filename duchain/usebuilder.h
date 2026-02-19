#ifndef JULIA_USEBUILDER_H
#define JULIA_USEBUILDER_H

#include <language/duchain/builders/abstractusebuilder.h>

#include "../parser/ast.h"
#include "contextbuilder.h"
#include "expressionvisitor.h"

namespace Julia {

class JuliaEditorIntegrator;

typedef KDevelop::AbstractUseBuilder<Julia::AstNode, Julia::AstNode, ContextBuilder> UseBuilderBase;

class UseBuilder : public UseBuilderBase
{
public:
    UseBuilder(JuliaEditorIntegrator* editor = nullptr);
    ~UseBuilder() override;

protected:
    KDevelop::RangeInRevision editorFindRange(AstNode* fromNode, AstNode* toNode) override;
    KDevelop::QualifiedIdentifier identifierForNode(AstNode* node) override;

    void visitIdentifier(AstNode* node) override;
    void visitCall(CallNode* node) override;
    void visitDot(AstNode* node) override;

private:
    JuliaEditorIntegrator* m_editor;
};

}

#endif
