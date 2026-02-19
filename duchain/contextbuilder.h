#ifndef JULIA_CONTEXTBUILDER_H
#define JULIA_CONTEXTBUILDER_H

#include <language/duchain/builders/abstractcontextbuilder.h>

#include <QVector>

#include "../parser/ast.h"
#include "astvisitor.h"
#include "juliaastdefaultvisitor.h"
#include "juliaducontext.h"

namespace Julia {

class JuliaEditorIntegrator;

// ContextBuilder: Builds the scope/context tree for the DUChain
// Note: We use AstNode for both template parameters (T and NameT).
// This works because:
// 1. identifierForNode() extracts text from any AstNode via node->text()
// 2. Virtual dispatch works correctly for visitNode() methods
// 3. The vtable allows derived classes to override specific visit methods
class ContextBuilder : public KDevelop::AbstractContextBuilder<Julia::AstNode, Julia::AstNode>, public JuliaAstDefaultVisitor
{
public:
    ContextBuilder();
    ~ContextBuilder() override;

    void setEditor(JuliaEditorIntegrator* editor);
    JuliaEditorIntegrator* editor() const;

protected:
    void startVisiting(AstNode* node) override;

    KDevelop::DUContext* contextFromNode(AstNode* node) override;
    virtual void setContextOnNode(AstNode* node, KDevelop::DUContext* context) override;
    KDevelop::RangeInRevision editorFindRange(AstNode* fromNode, AstNode* toNode) override;
    KDevelop::RangeInRevision editorFindRangeForContext(AstNode* fromNode, AstNode* toNode) override;
    KDevelop::QualifiedIdentifier identifierForNode(AstNode* node) override;

    // Container nodes - override to handle specially
    void visitTopLevel(AstNode* node) override;
    void visitFunction(FunctionNode* node) override;
    void visitStruct(StructNode* node) override;
    void visitModule(AstNode* node) override;
    void visitBlock(AstNode* node) override;

    // For function parameters and body
    void visitFunctionParameters(AstNode* node, FunctionNode* funcNode);
    void visitFunctionBody(AstNode* node, FunctionNode* funcNode);

    virtual void addImportedContexts();

    AstNode* extractFunctionNameNode(FunctionNode* funcNode);
    KDevelop::QualifiedIdentifier extractFunctionId(FunctionNode* funcNode);
    KDevelop::RangeInRevision rangeForArgumentsContext(FunctionNode* funcNode);

    QVector<KDevelop::DUContext*> m_importedParentContexts;

private:
    KDevelop::DUContext* newContext(const KDevelop::RangeInRevision& range) override;
    KDevelop::TopDUContext* newTopContext(const KDevelop::RangeInRevision& range,
                                          KDevelop::ParsingEnvironmentFile* file) override;
    
    JuliaEditorIntegrator* m_editor = nullptr;
};

}

#endif
