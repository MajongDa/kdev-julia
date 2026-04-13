#ifndef JULIA_DECLARATIONBUILDER_H
#define JULIA_DECLARATIONBUILDER_H

#include <language/duchain/builders/abstractdeclarationbuilder.h>
#include <language/duchain/builders/abstracttypebuilder.h>

#include "parser/ast.h"
#include "types/types.h"
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

    // Declaration creation via virtual dispatch (Python-style)
    void visitNode(Ast* node) override;
    void visitCode(CodeAst* node) override;
    void visitFunctionDefinition(FunctionDefinitionAst* node) override;
    void visitArguments(ArgumentsAst* node) override;
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
    void processArg(ArgAst* node, KDevelop::FunctionType::Ptr funcType, 
                   int defaultValueIndex, ArgumentsAst* parentArgs);

private:
    JuliaEditorIntegrator* m_editor;
};

}

#endif
