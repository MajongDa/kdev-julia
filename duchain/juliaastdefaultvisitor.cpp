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
    visitNode(node->arguments());
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

void JuliaAstDefaultVisitor::visitAssignment(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitReturn(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
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

void JuliaAstDefaultVisitor::visitWhile(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitFor(AstNode* node)
{
    if (!node) return;
    for (AstNode* child : node->children()) {
        if (child) {
            visitNode(child);
        }
    }
}

void JuliaAstDefaultVisitor::visitTry(AstNode* node)
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

void JuliaAstDefaultVisitor::visitParameters(AstNode* node)
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

void JuliaAstDefaultVisitor::visitIdentifier(AstNode* node) {}
void JuliaAstDefaultVisitor::visitString(AstNode* node) {}
void JuliaAstDefaultVisitor::visitFloat(AstNode* node) {}
void JuliaAstDefaultVisitor::visitInteger(AstNode* node) {}
void JuliaAstDefaultVisitor::visitBool(AstNode* node) {}
void JuliaAstDefaultVisitor::visitOperator(AstNode* node) {}
void JuliaAstDefaultVisitor::visitComment(AstNode* node) {}
void JuliaAstDefaultVisitor::visitWhitespace(AstNode* node) {}
void JuliaAstDefaultVisitor::visitNewline(AstNode* node) {}
void JuliaAstDefaultVisitor::visitSemicolon(AstNode* node) {}
void JuliaAstDefaultVisitor::visitComma(AstNode* node) {}
void JuliaAstDefaultVisitor::visitEquals(AstNode* node) {}
void JuliaAstDefaultVisitor::visitColonEquals(AstNode* node) {}
void JuliaAstDefaultVisitor::visitBreak(AstNode* node) {}
void JuliaAstDefaultVisitor::visitContinue(AstNode* node) {}
void JuliaAstDefaultVisitor::visitError(AstNode* node) {}
void JuliaAstDefaultVisitor::visitUnknown(AstNode* node) {}

}
