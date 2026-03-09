#ifndef JULIA_DECLARATIONBUILDER_H
#define JULIA_DECLARATIONBUILDER_H

#include <language/duchain/builders/abstractdeclarationbuilder.h>
#include <language/duchain/builders/abstracttypebuilder.h>

#include "../parser/ast.h"
#include "../types/types.h"
#include "contextbuilder.h"

namespace Julia {

class JuliaEditorIntegrator;

typedef KDevelop::AbstractTypeBuilder<Julia::AstNode, Julia::AstNode, ContextBuilder> TypeBuilderBase;
typedef KDevelop::AbstractDeclarationBuilder<Julia::AstNode, Julia::AstNode, TypeBuilderBase> DeclarationBuilderBase;

class DeclarationBuilder : public DeclarationBuilderBase
{
public:
    DeclarationBuilder(JuliaEditorIntegrator* editor = nullptr);
    ~DeclarationBuilder() override;

protected:
    // Required overrides from base
    KDevelop::RangeInRevision editorFindRange(AstNode* fromNode, AstNode* toNode) override;
    void setContextOnNode(AstNode* node, KDevelop::DUContext* context) override;
    KDevelop::DUContext* contextFromNode(AstNode* node) override;
    KDevelop::QualifiedIdentifier identifierForNode(AstNode* node) override;

    // Declaration creation via virtual dispatch (Python-style)
    void visitNode(AstNode* node) override;
    void visitFunction(FunctionNode* node) override;
    void visitStruct(StructNode* node) override;
    void visitModule(AstNode* node) override;
    void visitAbstract(AstNode* node) override;
    void visitPrimitive(AstNode* node) override;
    void visitUsing(AstNode* node) override;
    void visitImport(AstNode* node) override;
    void visitExport(AstNode* node) override;
    void visitAssignment(AssignmentNode* node) override;
    void visitReturn(AstNode* node) override;
    void visitStructBody(StructNode* node);
    void visitMacro(AstNode* node) override;
    void visitMacroCall(AstNode* node) override;
    void visitImportPath(AstNode* node) override;

    // Note: visitFunctionParameters and visitFunctionBody are inherited from ContextBuilder
    // They create contexts and should be called after creating declarations

private:
    JuliaEditorIntegrator* m_editor;
};

}

#endif
