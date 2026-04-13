#ifndef JULIA_USEBUILDER_H
#define JULIA_USEBUILDER_H

#include <language/duchain/builders/abstractusebuilder.h>

#include "parser/ast.h"
#include "contextbuilder.h"
#include "expressionvisitor.h"

namespace Julia {

class JuliaEditorIntegrator;

typedef KDevelop::AbstractUseBuilder<Julia::Ast, Julia::IdentifierAst, ContextBuilder> UseBuilderBase;

class UseBuilder : public UseBuilderBase
{
public:
    UseBuilder(JuliaEditorIntegrator* editor = nullptr);
    ~UseBuilder() override;

protected:
    KDevelop::RangeInRevision editorFindRange(Ast* fromNode, Ast* toNode) override;
    KDevelop::QualifiedIdentifier identifierForNode(IdentifierAst* node) override;

    void visitIdentifier(IdentifierAst* node) override;
    void visitCall(CallAst* node) override;
    void visitAttribute(AttributeAst* node) override;

private:
    JuliaEditorIntegrator* m_editor;
};

}

#endif
