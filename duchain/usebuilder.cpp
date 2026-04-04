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
    qCDebug(KDEV_JULIA) << "  currentContext:" << currentContext() 
                         << "type:" << (currentContext() ? currentContext()->type() : -1);

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
    
    // Find context at identifier position from the built DUChain (Python-style)
    qCDebug(KDEV_JULIA) << "  UseBuilder::visitIdentifier: topContext=" << topContext() 
                         << "currentContext=" << currentContext();
    KDevelop::DUContext* ctx = nullptr;
    {
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        ctx = topContext()->findContextAt(node->range().start, true);
        
        // Debug: show context hierarchy
        if (ctx) {
            qCDebug(KDEV_JULIA) << "  Context hierarchy:";
            KDevelop::DUContext* parent = ctx->parentContext();
            int depth = 0;
            while (parent) {
                qCDebug(KDEV_JULIA) << "    Parent[" << depth << "]:" << parent 
                                     << "type:" << parent->type() 
                                     << "range:" << parent->range();
                parent = parent->parentContext();
                depth++;
            }
            
            // Show declarations in this context
            qCDebug(KDEV_JULIA) << "  Local declarations in ctx:" << ctx->localDeclarations().size();
            for (auto* d : ctx->localDeclarations()) {
                qCDebug(KDEV_JULIA) << "    Decl:" << d->identifier().toString() 
                                     << "range:" << d->range();
            }
        }
    }
    qCDebug(KDEV_JULIA) << "  findContextAt returned:" << ctx 
                         << "type:" << (ctx ? ctx->type() : -1);
    if (!ctx) {
        ctx = currentContext();  // fallback
    }
    
    if (!ctx) {
        qCDebug(KDEV_JULIA) << "  ERROR: No context available, skipping";
        return;
    }
    
    qCDebug(KDEV_JULIA) << "  Using context:" << ctx << "for identifier";
    
    // Use ExpressionVisitor ONLY for type inference, don't visit children (causes recursion!)
    ExpressionVisitor v(ctx);
    v.visitIdentifier(node);  // Only visit this specific node, not children
    
    auto type = v.lastType();
    if (type) {
        qCDebug(KDEV_JULIA) << "  Inferred type:" << type->toString();
    }
    
    KDevelop::RangeInRevision useRange = editorFindRange(node, node);
    qCDebug(KDEV_JULIA) << "  Use range:" << useRange;
    
    auto decl = v.lastDeclaration();
    if (decl) {
        qCDebug(KDEV_JULIA) << "  Found declaration:" << decl->identifier().toString() 
                             << "range:" << decl->range() 
                             << "in context:" << decl->context();
        if (decl->range() == useRange) {
            qCDebug(KDEV_JULIA) << "  Skipping - is declaration itself";
            return;
        }
        
        // Log what context we're adding the use to
        qCDebug(KDEV_JULIA) << "  Adding use to context:" << currentContext() 
                             << "type:" << (currentContext() ? currentContext()->type() : -1);
        
        UseBuilderBase::newUse(useRange, KDevelop::DeclarationPointer(decl));
        qCDebug(KDEV_JULIA) << "  Created use for declaration";
        
        // Verify use was added - check all child contexts too
        {
            KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
            int totalUses = currentContext() ? currentContext()->usesCount() : 0;
            qCDebug(KDEV_JULIA) << "  Uses in currentContext after add:" << totalUses;
            
            // Also check if there are uses in parent context
            KDevelop::DUContext* parent = currentContext() ? currentContext()->parentContext() : nullptr;
            while (parent) {
                qCDebug(KDEV_JULIA) << "  Uses in parent context:" << parent->usesCount();
                parent = parent->parentContext();
            }
        }
        return;
    }
    
    qCDebug(KDEV_JULIA) << "  No declaration found - creating empty use";
    UseBuilderBase::newUse(useRange, KDevelop::DeclarationPointer());
    qCDebug(KDEV_JULIA) << "<<< UseBuilder::visitIdentifier DONE";
}

void UseBuilder::visitCall(CallNode* node)
{
    if (!node) {
        return;
    }

    qCDebug(KDEV_JULIA) << ">>> UseBuilder::visitCall:" << node->range();

    // Call base class first to properly set up context
    UseBuilderBase::visitCall(node);

    // Find context at call position from the built DUChain (Python-style)
    KDevelop::DUContext* ctx = nullptr;
    {
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        ctx = topContext()->findContextAt(node->range().start, true);
    }
    if (!ctx) {
        ctx = currentContext();  // fallback
    }
    
    qCDebug(KDEV_JULIA) << "  UseBuilder::visitCall context:" << ctx;
    
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

    // Find context at dot position from the built DUChain (Python-style)
    KDevelop::DUContext* ctx = nullptr;
    {
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        ctx = topContext()->findContextAt(node->range().start, true);
    }
    if (!ctx) {
        ctx = currentContext();  // fallback
    }
    
    if (!ctx) {
        return;
    }

    ExpressionVisitor v(ctx);
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
