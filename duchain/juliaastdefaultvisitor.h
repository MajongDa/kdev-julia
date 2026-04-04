#ifndef JULIA_ASTDEFAULTVISITOR_H
#define JULIA_ASTDEFAULTVISITOR_H

#include "astvisitor.h"

namespace Julia {

class JuliaAstDefaultVisitor : public AstVisitor
{
public:
    JuliaAstDefaultVisitor() = default;
    ~JuliaAstDefaultVisitor() override = default;

    // Container nodes - define scopes
    void visitTopLevel(AstNode* node) override;
    void visitBlock(AstNode* node) override;
    void visitFunction(FunctionNode* node) override;
    void visitStruct(StructNode* node) override;
    void visitModule(AstNode* node) override;
    void visitBaremodule(BaremoduleNode* node) override;
    void visitAbstract(AstNode* node) override;
    void visitPrimitive(AstNode* node) override;
    void visitMacro(AstNode* node) override;

    // Statements
    void visitAssignment(AssignmentNode* node) override;
    void visitReturn(ReturnNode* node) override;
    void visitIf(AstNode* node) override;
    void visitElseIf(AstNode* node) override;
    void visitElse(AstNode* node) override;
    void visitWhile(WhileNode* node) override;
    void visitFor(ForNode* node) override;
    void visitTry(TryNode* node) override;
    void visitCatch(AstNode* node) override;
    void visitFinally(AstNode* node) override;

    // Variable declarations
    void visitConst(ConstNode* node) override;
    void visitGlobal(GlobalNode* node) override;
    void visitLocal(LocalNode* node) override;
    void visitLet(LetNode* node) override;
    void visitDo(DoNode* node) override;
    void visitQuote(QuoteNode* node) override;

    // Other keywords
    void visitEnd(AstNode* node) override;

    // Expressions
    void visitCall(CallNode* node) override;
    void visitFunctionSignature(CallNode* node) override;
    void visitCurly(CurlyNode* node) override;
    void visitWhere(AstNode* node) override;
    void visitTypeAnnotation(AstNode* node) override;
    void visitTuple(AstNode* node) override;
    void visitArray(AstNode* node) override;
    void visitDict(AstNode* node) override;
    void visitVect(AstNode* node) override;
    void visitRef(AstNode* node) override;
    void visitGenerator(AstNode* node) override;

    // Import/Export
    void visitUsing(AstNode* node) override;
    void visitImport(AstNode* node) override;
    void visitExport(AstNode* node) override;

    // Other
    void visitDot(AstNode* node) override;
    void visitColon(AstNode* node) override;
    void visitImportPath(AstNode* node) override;
    void visitMacroName(AstNode* node) override;
    void visitMacroCall(AstNode* node) override;

    // Leaf nodes - do nothing
    void visitIdentifier(AstNode* node) override;
    void visitString(AstNode* node) override;
    void visitFloat(AstNode* node) override;
    void visitInteger(AstNode* node) override;
    void visitBool(AstNode* node) override;
    void visitOperator(AstNode* node) override;
    void visitComment(AstNode* node) override;
    void visitWhitespace(AstNode* node) override;
    void visitNewline(AstNode* node) override;
    void visitSemicolon(AstNode* node) override;
    void visitComma(AstNode* node) override;
    void visitColonEquals(AstNode* node) override;
    void visitBreak(AstNode* node) override;
    void visitContinue(AstNode* node) override;
    void visitError(AstNode* node) override;
    void visitUnknown(AstNode* node) override;
};

}

#endif
