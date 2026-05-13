#ifndef JULIA_DECLARATIONBUILDER_H
#define JULIA_DECLARATIONBUILDER_H

#include <language/duchain/builders/abstractdeclarationbuilder.h>
#include <language/duchain/builders/abstracttypebuilder.h>

#include <language/duchain/types/integraltype.h>
#include <language/duchain/types/functiontype.h>
#include <language/duchain/types/pointertype.h>
#include <language/duchain/types/arraytype.h>
#include <language/duchain/types/containertypes.h>
#include <language/duchain/types/unsuretype.h>

#include "parser/ast.h"
#include "contextbuilder.h"

namespace Julia {

class JuliaEditorIntegrator;

typedef KDevelop::AbstractTypeBuilder<Ast, IdentifierAst, ContextBuilder> TypeBuilderBase;
typedef KDevelop::AbstractDeclarationBuilder<Ast, IdentifierAst, TypeBuilderBase> DeclarationBuilderBase;

class DeclarationBuilder : public DeclarationBuilderBase
{
public:
    DeclarationBuilder(JuliaEditorIntegrator* editor = nullptr);
    ~DeclarationBuilder() override;

protected:
    // Required overrides from base
    KDevelop::RangeInRevision editorFindRange(Ast* fromNode, Ast* toNode) override;
    void setContextOnNode(Ast* node, KDevelop::DUContext* context) override;
    KDevelop::DUContext* contextFromNode(Ast* node) override;
    KDevelop::QualifiedIdentifier identifierForNode(IdentifierAst* node) override;

    void visitNode(Ast* node) override;
    void visitFunctionDefinition(FunctionDefinitionAst* node) override;
    void visitAssignment(AssignmentAst* node) override;
    void visitReturn(ReturnAst* node) override;
    void visitImport(ImportAst* node) override;
    void visitFor(ForAst* node) override;
    void visitWhile(WhileAst* node) override;
    void visitGlobal(GlobalAst* node) override;
    void visitLocal(LocalAst* node) override;
    void visitModule(ModuleAst* node) override;
    void visitBaremodule(BaremoduleAst* node) override;
    void visitStruct(StructAst* node) override;
    void visitAbstract(AbstractAst* node) override;
    void visitPrimitive(PrimitiveAst* node) override;
    void visitMacro(MacroAst* node) override;
    void visitConst(ConstAst* node) override;
    void visitTypeAnnotation(TypeAnnotationAst* node) override;

private:
    JuliaEditorIntegrator* m_editor;
    void processParameter(Ast* arg, KDevelop::FunctionType::Ptr funcType);
    void declareIdentifier(IdentifierAst* ident, KDevelop::AbstractType::Ptr type = {});
};

}

#endif
