#ifndef JULIA_CONTEXTBUILDER_H
#define JULIA_CONTEXTBUILDER_H

#include <language/duchain/builders/abstractcontextbuilder.h>

#include "../parser/ast.h"

namespace Julia {

class ContextBuilder : public KDevelop::AbstractContextBuilder<Julia::AstNode, Julia::AstNode>
{
public:
    ContextBuilder();
    ~ContextBuilder() override;

protected:
    void startVisiting(AstNode* node) override;

    KDevelop::DUContext* contextFromNode(AstNode* node) override;
    void setContextOnNode(AstNode* node, KDevelop::DUContext* context) override;
    KDevelop::RangeInRevision editorFindRange(AstNode* fromNode, AstNode* toNode) override;
    KDevelop::RangeInRevision editorFindRangeForContext(AstNode* fromNode, AstNode* toNode) override;
    KDevelop::QualifiedIdentifier identifierForNode(AstNode* node) override;

private:
    KDevelop::DUContext* newContext(const KDevelop::RangeInRevision& range) override;
    KDevelop::TopDUContext* newTopContext(const KDevelop::RangeInRevision& range,
                                          KDevelop::ParsingEnvironmentFile* file) override;

    void visitFunction(AstNode* node);
    void visitStruct(AstNode* node);
    void visitModule(AstNode* node);
    void visitBlock(AstNode* node);
    void visitFor(AstNode* node);
    void visitWhile(AstNode* node);
    void visitIf(AstNode* node);
};

}

#endif
