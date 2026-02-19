#include "usebuilder.h"

#include <QDebug>

#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>

#include "../parser/ast.h"
#include "contextbuilder.h"
#include "expressionvisitor.h"
#include "juliaeditorintegrator.h"
#include "juliadebug.h"

namespace Julia {

UseBuilder::UseBuilder(JuliaEditorIntegrator* editor)
    : m_editor(editor)
{
}

UseBuilder::~UseBuilder() = default;

void UseBuilder::visitIdentifier(AstNode* node)
{
    if (!node) {
        return;
    }

    qCDebug(KDEV_JULIA) << ">>> UseBuilder::visitIdentifier:" << node->text() << "range:" << node->range();

    if (AstNode* parent = node->parent()) {
        if (parent->kind() == NodeKind::Function ||
            parent->kind() == NodeKind::Struct ||
            parent->kind() == NodeKind::Module ||
            parent->kind() == NodeKind::Macro ||
            parent->kind() == NodeKind::Abstract ||
            parent->kind() == NodeKind::Primitive) {
            qCDebug(KDEV_JULIA) << "  Skipping - is declaration site";
            return;
        }
    }
    
    ExpressionVisitor v(currentContext());
    v.visitNode(node);
    
    auto type = v.lastType();
    if (type) {
        qCDebug(KDEV_JULIA) << "  Inferred type:" << type->toString();
    }
    
    KDevelop::RangeInRevision useRange = editorFindRange(node, node);
    
    auto decl = v.lastDeclaration();
    if (decl) {
        qCDebug(KDEV_JULIA) << "  Found declaration:" << decl->identifier().toString() << "range:" << decl->range();
        if (decl->range() == useRange) {
            qCDebug(KDEV_JULIA) << "  Skipping - is declaration itself";
            return;
        }
        UseBuilderBase::newUse(useRange, KDevelop::DeclarationPointer(decl));
        qCDebug(KDEV_JULIA) << "  Created use for declaration";
        return;
    }
    
    qCDebug(KDEV_JULIA) << "  No declaration found - creating empty use";
    UseBuilderBase::newUse(useRange, KDevelop::DeclarationPointer());
    qCDebug(KDEV_JULIA) << "<<< UseBuilder::visitIdentifier DONE";
}

void UseBuilder::visitCall(AstNode* node)
{
    if (!node) {
        return;
    }

    qCDebug(KDEV_JULIA) << ">>> UseBuilder::visitCall:" << node->range();

    // Call base class first to properly set up context
    UseBuilderBase::visitCall(node);

    KDevelop::DUContext* ctx = currentContext();
    qCDebug(KDEV_JULIA) << "  UseBuilder::visitCall currentContext:" << ctx;
    
    if (!ctx) {
        qCDebug(KDEV_JULIA) << "  No context, skipping ExpressionVisitor";
        return;
    }

    ExpressionVisitor v(ctx);
    v.visitNode(node);
    
    auto type = v.lastType();
    if (type) {
        qCDebug(KDEV_JULIA) << "  Inferred type:" << type->toString();
    }
    
    auto decl = v.lastDeclaration();
    if (decl) {
        qCDebug(KDEV_JULIA) << "  Found declaration:" << decl->identifier().toString();
        AstNode* funcName = node->firstChild();
        if (funcName && funcName->kind() == NodeKind::Identifier) {
            KDevelop::RangeInRevision useRange = editorFindRange(funcName, funcName);
            UseBuilderBase::newUse(useRange, KDevelop::DeclarationPointer(decl));
            qCDebug(KDEV_JULIA) << "  Created use for function call";
        }
    } else {
        qCDebug(KDEV_JULIA) << "  No declaration found";
    }
    qCDebug(KDEV_JULIA) << "<<< UseBuilder::visitCall DONE";
}

void UseBuilder::visitDot(AstNode* node)
{
    if (!node) {
        return;
    }

    QList<AstNode*> children = node->children();
    if (children.size() < 2) {
        return;
    }

    AstNode* lhs = children.first();
    AstNode* attr = children.last();
    if (!lhs || !attr) {
        return;
    }

    ExpressionVisitor v(currentContext());
    v.visitNode(node);

    KDevelop::DeclarationPointer decl = v.lastDeclaration();
    if (decl) {
        KDevelop::RangeInRevision useRange = editorFindRange(attr, attr);
        if (decl->range() != useRange) {
            UseBuilderBase::newUse(useRange, decl);
        }
    }
}

KDevelop::RangeInRevision UseBuilder::editorFindRange(AstNode* fromNode, AstNode* toNode)
{
    if (!fromNode || !toNode) {
        return KDevelop::RangeInRevision(0, 0, 0, 0);
    }

    if (m_editor) {
        return m_editor->findRange(fromNode, toNode);
    }

    return fromNode->range();
}

KDevelop::QualifiedIdentifier UseBuilder::identifierForNode(AstNode* node)
{
    if (!node) {
        return KDevelop::QualifiedIdentifier();
    }

    if (node->kind() == NodeKind::Identifier) {
        return KDevelop::QualifiedIdentifier(node->text());
    }

    return KDevelop::QualifiedIdentifier();
}

}
