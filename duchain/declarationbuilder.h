#ifndef JULIA_DECLARATIONBUILDER_H
#define JULIA_DECLARATIONBUILDER_H

#include <language/duchain/builders/abstractdeclarationbuilder.h>
#include <language/duchain/builders/abstracttypebuilder.h>

#include "../parser/ast.h"
#include "../types/types.h"
#include "contextbuilder.h"

namespace Julia {

typedef KDevelop::AbstractTypeBuilder<Julia::AstNode, Julia::AstNode, ContextBuilder> TypeBuilderBase;
typedef KDevelop::AbstractDeclarationBuilder<Julia::AstNode, Julia::AstNode, TypeBuilderBase> DeclarationBuilderBase;

class DeclarationBuilder : public DeclarationBuilderBase
{
public:
    DeclarationBuilder();
    ~DeclarationBuilder() override;

protected:
    void startVisiting(AstNode* node) override;

    KDevelop::RangeInRevision editorFindRange(AstNode* fromNode, AstNode* toNode) override;
    void setContextOnNode(AstNode* node, KDevelop::DUContext* context) override;
    KDevelop::DUContext* contextFromNode(AstNode* node) override;
};

}

#endif
