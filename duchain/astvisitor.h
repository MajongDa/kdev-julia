#ifndef JULIA_ASTVISITOR_H
#define JULIA_ASTVISITOR_H

#include "../parser/ast.h"

namespace Julia {

class AstVisitor {
public:
    virtual ~AstVisitor() = default;
    
    void visit(AstNode* node);
    
    virtual void visitNode(AstNode* node);
    
    // Container nodes - using specific types (Python-style)
    virtual void visitTopLevel(AstNode*) {}
    virtual void visitFunction(FunctionNode*) {}
    virtual void visitStruct(StructNode*) {}
    virtual void visitModule(AstNode*) {}
    virtual void visitBaremodule(BaremoduleNode*) {}
    virtual void visitBlock(AstNode*) {}
    
    // Statements
    virtual void visitReturn(AstNode*) {}
    virtual void visitIf(AstNode*) {}
    virtual void visitElseIf(AstNode*) {}
    virtual void visitElse(AstNode*) {}
    virtual void visitWhile(AstNode*) {}
    virtual void visitFor(AstNode*) {}
    virtual void visitTry(TryNode*) {}
    virtual void visitCatch(AstNode*) {}
    virtual void visitFinally(AstNode*) {}
    virtual void visitAssignment(AssignmentNode*) {}
    virtual void visitBreak(AstNode*) {}
    virtual void visitContinue(AstNode*) {}
    
    // Variable declarations
    virtual void visitConst(ConstNode*) {}
    virtual void visitGlobal(GlobalNode*) {}
    virtual void visitLocal(LocalNode*) {}
    virtual void visitLet(LetNode*) {}
    virtual void visitDo(DoNode*) {}
    virtual void visitQuote(QuoteNode*) {}
    
    // Other keywords
    virtual void visitEnd(AstNode*) {}
    
    // Expressions - using specific types where available
    virtual void visitIdentifier(AstNode*) {}
    virtual void visitCall(CallNode*) {}
    virtual void visitOperator(AstNode*) {}
    virtual void visitTypeAnnotation(AstNode*) {}
    virtual void visitCurly(CurlyNode*) {}
    virtual void visitWhere(AstNode*) {}
    virtual void visitParameters(AstNode*) {}
    virtual void visitString(AstNode*) {}
    virtual void visitFloat(AstNode*) {}
    virtual void visitInteger(AstNode*) {}
    virtual void visitBool(AstNode*) {}
    virtual void visitTuple(AstNode*) {}
    virtual void visitArray(AstNode*) {}
    virtual void visitDict(AstNode*) {}
    virtual void visitRef(AstNode*) {}
    virtual void visitVect(AstNode*) {}
    virtual void visitGenerator(AstNode*) {}
    virtual void visitImportPath(AstNode*) {}
    virtual void visitMacroName(AstNode*) {}
    virtual void visitMacroCall(AstNode*) {}
    
    // Import/Export
    virtual void visitUsing(AstNode*) {}
    virtual void visitImport(AstNode*) {}
    virtual void visitExport(AstNode*) {}
    virtual void visitMacro(AstNode*) {}
    
    // Other
    virtual void visitColonEquals(AstNode*) {}
    virtual void visitDot(AstNode*) {}
    virtual void visitColon(AstNode*) {}
    virtual void visitSemicolon(AstNode*) {}
    virtual void visitComma(AstNode*) {}
    virtual void visitComment(AstNode*) {}
    virtual void visitWhitespace(AstNode*) {}
    virtual void visitNewline(AstNode*) {}
    virtual void visitError(AstNode*) {}
    virtual void visitUnknown(AstNode*) {}
    
    // Abstract types
    virtual void visitAbstract(AstNode*) {}
    virtual void visitPrimitive(AstNode*) {}
};

} // namespace Julia

#endif
