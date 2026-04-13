#include "usebuilder.h"

#include <QDebug>

#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>

#include "parser/ast.h"
#include "contextbuilder.h"
#include "expressionvisitor.h"
#include "juliaeditorintegrator.h"
#include "juliadebug.h"

namespace Julia {

UseBuilder::UseBuilder(JuliaEditorIntegrator* editor)
    : UseBuilderBase()
    , m_editor(editor)
{
}

UseBuilder::~UseBuilder() = default;

void UseBuilder::visitIdentifier(IdentifierAst* node)
{
    if (!node) {
        return;
    }

    qCDebug(KDEV_JULIA) << ">>> UseBuilder::visitIdentifier:" << node->range();
    qCDebug(KDEV_JULIA) << "  currentContext:" << currentContext();

    QString name = node->value;
    if (name.isEmpty()) {
        return;
    }

    // Find context at identifier position
    KDevelop::DUContext* ctx = nullptr;
    {
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        ctx = topContext()->findContextAt(node->range().start, true);
    }
    if (!ctx) {
        ctx = currentContext();
    }
    
    if (!ctx) {
        qCDebug(KDEV_JULIA) << "  ERROR: No context available";
        return;
    }

    // Use ExpressionVisitor for type inference
    ExpressionVisitor v(ctx);
    v.visitIdentifier(node);
    
    auto decl = v.lastDeclaration();
    if (decl) {
        qCDebug(KDEV_JULIA) << "  Found declaration:" << decl->identifier().toString();
        if (decl->range() != node->range()) {
            KDevelop::RangeInRevision useRange = editorFindRange(node, node);
            UseBuilderBase::newUse(useRange, KDevelop::DeclarationPointer(decl));
            qCDebug(KDEV_JULIA) << "  Created use for declaration";
        }
        return;
    }
    
    qCDebug(KDEV_JULIA) << "  No declaration found";
}

void UseBuilder::visitCall(CallAst* node)
{
    if (!node) {
        return;
    }

    qCDebug(KDEV_JULIA) << ">>> UseBuilder::visitCall:" << node->range();

    // Visit function to find its declaration
    if (node->function) {
        visitNode(node->function);
    }

    qCDebug(KDEV_JULIA) << "<<< UseBuilder::visitCall DONE";
}

void UseBuilder::visitAttribute(AttributeAst* node)
{
    if (!node) {
        return;
    }

    // Visit the value (the thing before the dot)
    if (node->value) {
        visitNode(node->value);
    }
}

KDevelop::RangeInRevision UseBuilder::editorFindRange(Ast* fromNode, Ast* toNode)
{
    if (!fromNode || !toNode) {
        return KDevelop::RangeInRevision(0, 0, 0, 0);
    }

    if (m_editor) {
        return m_editor->findRange(fromNode, toNode);
    }

    return fromNode->range();
}

KDevelop::QualifiedIdentifier UseBuilder::identifierForNode(IdentifierAst* node)
{
    if (!node) {
        return KDevelop::QualifiedIdentifier();
    }

    return KDevelop::QualifiedIdentifier(node->value);
}

}
