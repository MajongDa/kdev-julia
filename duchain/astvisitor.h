#ifndef JULIA_ASTVISITOR_H
#define JULIA_ASTVISITOR_H

#include "../parser/ast.h"

namespace Julia {

class AstVisitor {
public:
    virtual ~AstVisitor() = default;
    
    void visit(AstNode* node);
    
    virtual void visitNode(AstNode* node);
    
    // Container nodes
    virtual void visitTopLevel(AstNode* node) {}
    virtual void visitFunction(AstNode* node) {}
    virtual void visitStruct(AstNode* node) {}
    virtual void visitModule(AstNode* node) {}
    virtual void visitBlock(AstNode* node) {}
    
    // Statements
    virtual void visitReturn(AstNode* node) {}
    virtual void visitIf(AstNode* node) {}
    virtual void visitWhile(AstNode* node) {}
    virtual void visitFor(AstNode* node) {}
    virtual void visitTry(AstNode* node) {}
    virtual void visitAssignment(AstNode* node) {}
    virtual void visitBreak(AstNode* node) {}
    virtual void visitContinue(AstNode* node) {}
    
    // Expressions
    virtual void visitIdentifier(AstNode* node) {}
    virtual void visitCall(AstNode* node) {}
    virtual void visitOperator(AstNode* node) {}
    virtual void visitTypeAnnotation(AstNode* node) {}
    virtual void visitCurly(AstNode* node) {}
    virtual void visitWhere(AstNode* node) {}
    virtual void visitParameters(AstNode* node) {}
    virtual void visitString(AstNode* node) {}
    virtual void visitFloat(AstNode* node) {}
    virtual void visitInteger(AstNode* node) {}
    virtual void visitBool(AstNode* node) {}
    virtual void visitTuple(AstNode* node) {}
    virtual void visitArray(AstNode* node) {}
    virtual void visitDict(AstNode* node) {}
    virtual void visitRef(AstNode* node) {}
    virtual void visitVect(AstNode* node) {}
    virtual void visitGenerator(AstNode* node) {}
    virtual void visitImportPath(AstNode* node) {}
    virtual void visitMacroName(AstNode* node) {}
    virtual void visitMacroCall(AstNode* node) {}
    
    // Import/Export
    virtual void visitUsing(AstNode* node) {}
    virtual void visitImport(AstNode* node) {}
    virtual void visitExport(AstNode* node) {}
    virtual void visitMacro(AstNode* node) {}
    
    // Other
    virtual void visitEquals(AstNode* node) {}
    virtual void visitColonEquals(AstNode* node) {}
    virtual void visitDot(AstNode* node) {}
    virtual void visitColon(AstNode* node) {}
    virtual void visitSemicolon(AstNode* node) {}
    virtual void visitComma(AstNode* node) {}
    virtual void visitComment(AstNode* node) {}
    virtual void visitWhitespace(AstNode* node) {}
    virtual void visitNewline(AstNode* node) {}
    virtual void visitError(AstNode* node) {}
    virtual void visitUnknown(AstNode* node) {}
    
    // Abstract types
    virtual void visitAbstract(AstNode* node) {}
    virtual void visitPrimitive(AstNode* node) {}
};

} // namespace Julia

#endif
