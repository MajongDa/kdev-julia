#ifndef JULIA_USEBUILDER_H
#define JULIA_USEBUILDER_H

#include <language/duchain/builders/abstractusebuilder.h>

#include "../parser/ast.h"
#include "contextbuilder.h"

namespace Julia {

class UseBuilder : public KDevelop::AbstractUseBuilder<Julia::AstNode, Julia::AstNode, ContextBuilder>
{
public:
    UseBuilder();
    ~UseBuilder() override;

protected:
    void startVisiting(AstNode* node) override;

    KDevelop::RangeInRevision editorFindRange(AstNode* fromNode, AstNode* toNode) override;
    KDevelop::QualifiedIdentifier identifierForNode(AstNode* node) override;

    void visitIdentifier(AstNode* node);
    void visitCall(AstNode* node);
};

}

#endif
