#include "juliaastdefaultvisitor.h"
#include "../parser/ast.h"
#include "juliadebug.h"

namespace Julia {


void JuliaAstDefaultVisitor::visitTopLevel(AstNode* node)
{
    if (!node) return;
    qCDebug(KDEV_JULIA) << ">>> JuliaAstDefaultVisitor::visitTopLevel";
    // Visit children - NOT the node itself (that would cause infinite recursion)
    for (AstNode* child : node->children()) {
        if (child) {
            AstVisitor::visitNode(child);
        }
    }
    qCDebug(KDEV_JULIA) << "<<< JuliaAstDefaultVisitor::visitTopLevel DONE";
}

void JuliaAstDefaultVisitor::visitBlock(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitFunction(FunctionNode* node)
{
    if (!node) return;
    visitNode(node->callNode());
    visitNode(node->body());
}

void JuliaAstDefaultVisitor::visitStruct(StructNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitModule(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitBaremodule(BaremoduleNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitAbstract(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitPrimitive(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitMacro(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitCall(CallNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitAssignment(AssignmentNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitReturn(ReturnNode* node)
{
    if (!node) return;
    if (node->value()) {
        visitNode(node->value());
    }
}

void JuliaAstDefaultVisitor::visitIf(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitWhile(WhileNode* node)
{
    if (!node) return;
    visitNode(node->condition());
    visitNode(node->body());
}

void JuliaAstDefaultVisitor::visitFor(ForNode* node)
{
    if (!node) return;
    visitNode(node->iterator());
    visitNode(node->body());
}

void JuliaAstDefaultVisitor::visitTry(TryNode* node)
{
    if (!node) return;
    visitNode(node->tryBody());
    visitNode(node->catchVariable());
    visitNode(node->catchBody());
    visitNode(node->finallyBody());
}

void JuliaAstDefaultVisitor::visitCatch(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitFinally(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitElseIf(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitElse(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitConst(ConstNode* node)
{
    if (!node) return;
    visitNode(node->target());
    visitNode(node->value());
}

void JuliaAstDefaultVisitor::visitGlobal(GlobalNode* node)
{
    if (!node) return;
    for (AstNode* child : node->identifiers()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitLocal(LocalNode* node)
{
    if (!node) return;
    for (AstNode* child : node->identifiers()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitLet(LetNode* node)
{
    if (!node) return;
    for (AstNode* child : node->bindings()) {
        if (child) {
            visitNode(child);
        }
    }
    visitNode(node->body());
}

void JuliaAstDefaultVisitor::visitDo(DoNode* node)
{
    if (!node) return;
    for (AstNode* child : node->arguments()) {
        if (child) {
            visitNode(child);
        }
    }
    visitNode(node->body());
}

void JuliaAstDefaultVisitor::visitQuote(QuoteNode* node)
{
    if (!node) return;
    for (AstNode* child : node->body()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitEnd(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitUsing(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitImport(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitExport(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitCurly(CurlyNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitWhere(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitTypeAnnotation(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

// visitFunctionSignature traverses the Call node that's part of a function definition
// It visits all children (the function name + parameters) for traversal
void JuliaAstDefaultVisitor::visitFunctionSignature(CallNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitTuple(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitArray(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitDict(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitVect(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitRef(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitGenerator(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitDot(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitColon(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitImportPath(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitMacroName(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitMacroCall(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitIdentifier(AstNode*) {}
void JuliaAstDefaultVisitor::visitString(AstNode*) {}
void JuliaAstDefaultVisitor::visitFloat(AstNode*) {}
void JuliaAstDefaultVisitor::visitInteger(AstNode*) {}
void JuliaAstDefaultVisitor::visitBool(AstNode*) {}
void JuliaAstDefaultVisitor::visitOperator(AstNode*) {}
void JuliaAstDefaultVisitor::visitComment(AstNode*) {}
void JuliaAstDefaultVisitor::visitWhitespace(AstNode*) {}
void JuliaAstDefaultVisitor::visitNewline(AstNode*) {}
void JuliaAstDefaultVisitor::visitSemicolon(AstNode*) {}
void JuliaAstDefaultVisitor::visitComma(AstNode*) {}
void JuliaAstDefaultVisitor::visitColonEquals(AstNode*) {}
void JuliaAstDefaultVisitor::visitBreak(AstNode*) {}
void JuliaAstDefaultVisitor::visitContinue(AstNode*) {}
void JuliaAstDefaultVisitor::visitError(AstNode*) {}
void JuliaAstDefaultVisitor::visitUnknown(AstNode*) {}

}
