#ifndef JULIA_CONTEXTBUILDER_H
#define JULIA_CONTEXTBUILDER_H

#include <language/duchain/builders/abstractcontextbuilder.h>

#include <QVector>

#include "parser/juliaastdefaultvisitor.h"
#include "juliaducontext.h"

namespace Julia {

class JuliaEditorIntegrator;

// ContextBuilder: Builds the scope/context tree for the DUChain
// Uses new Python-style AST with Ast and Identifier classes
class ContextBuilder : public KDevelop::AbstractContextBuilder<Ast, IdentifierAst>, public JuliaAstDefaultVisitor
{
public:
    ContextBuilder();
    ~ContextBuilder() override;

    void setEditor(JuliaEditorIntegrator* editor);
    JuliaEditorIntegrator* editor() const;

protected:
    void startVisiting(Ast* node) override;

    KDevelop::DUContext* contextFromNode(Ast* node) override;
    virtual void setContextOnNode(Ast* node, KDevelop::DUContext* context) override;
    KDevelop::RangeInRevision editorFindRange(Ast* fromNode, Ast* toNode) override;
    KDevelop::RangeInRevision editorFindRangeForContext(Ast* fromNode, Ast* toNode) override;
    KDevelop::QualifiedIdentifier identifierForNode(IdentifierAst* node) override;

    // Statements - override to create contexts
    void visitFunctionDefinition(FunctionDefinitionAst* node) override;
    void visitFunctionArguments(FunctionDefinitionAst* node);
    void visitFunctionBody(FunctionDefinitionAst* node);
    void visitModule(ModuleAst* node) override;
    void visitBaremodule(BaremoduleAst* node) override;
    void visitStruct(StructAst* node) override;
    void visitAbstract(AbstractAst* node) override;
    void visitPrimitive(PrimitiveAst* node) override;
    void visitMacro(MacroAst* node) override;
    void visitFor(ForAst* node) override;
    void visitWhile(WhileAst* node) override;
    void visitIf(IfAst* node) override;
    void visitTry(TryAst* node) override;
    void visitLet(LetAst* node) override;
    void visitDo(DoAst* node) override;
    
    // Expressions - override to handle specially
    void visitAssignment(AssignmentAst* node) override;
    void visitLambda(LambdaAst* node) override;
    void visitImport(ImportAst* node) override;
    
    // Generators/Comprehensions
    void visitGenerator(GeneratorAst* node) override;
    void visitComprehension(ComprehensionAst* node) override;

    virtual void addImportedContexts();

    QVector<KDevelop::DUContext*> m_importedParentContexts;

private:
    KDevelop::DUContext* newContext(const KDevelop::RangeInRevision& range) override;
    KDevelop::TopDUContext* newTopContext(const KDevelop::RangeInRevision& range,
                                          KDevelop::ParsingEnvironmentFile* file) override;
    
    JuliaEditorIntegrator* m_editor = nullptr;
};

}

#endif
