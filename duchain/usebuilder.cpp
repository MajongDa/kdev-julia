#include "usebuilder.h"

#include <QDebug>

#include <language/duchain/duchain.h>

#include "../parser/ast.h"
#include "juliadebug.h"

namespace Julia {

UseBuilder::UseBuilder() = default;

UseBuilder::~UseBuilder() = default;

void UseBuilder::startVisiting(AstNode* node)
{
    if (!node) {
        return;
    }

    switch (node->kind()) {
        case NodeKind::TopLevel:
        case NodeKind::Block:
            for (AstNode* child : node->children()) {
                startVisiting(child);
            }
            break;

        case NodeKind::Function:
        case NodeKind::Struct:
        case NodeKind::Module:
        case NodeKind::Abstract:
        case NodeKind::Primitive:
        case NodeKind::Macro:
            for (AstNode* child : node->children()) {
                startVisiting(child);
            }
            break;

        case NodeKind::Identifier:
            visitIdentifier(node);
            break;

        case NodeKind::Call:
            visitCall(node);
            for (AstNode* child : node->children()) {
                startVisiting(child);
            }
            break;

        case NodeKind::Assignment:
        case NodeKind::Equals:
            if (AstNode* lhs = node->firstChild()) {
                if (lhs->kind() == NodeKind::TypeAnnotation) {
                    for (AstNode* child : lhs->children()) {
                        startVisiting(child);
                    }
                } else if (lhs->kind() == NodeKind::Identifier) {
                    startVisiting(lhs);
                } else {
                    for (AstNode* child : lhs->children()) {
                        startVisiting(child);
                    }
                }
            }
            if (AstNode* rhs = node->lastChild()) {
                startVisiting(rhs);
            }
            break;

        case NodeKind::TypeAnnotation:
            for (AstNode* child : node->children()) {
                if (child) {
                    startVisiting(child);
                }
            }
            break;

        default:
            for (AstNode* child : node->children()) {
                startVisiting(child);
            }
            break;
    }
}

void UseBuilder::visitIdentifier(AstNode* node)
{
    if (!node) {
        return;
    }

    if (AstNode* parent = node->parent()) {
        if (parent->kind() == NodeKind::Function ||
            parent->kind() == NodeKind::Struct ||
            parent->kind() == NodeKind::Module ||
            parent->kind() == NodeKind::Macro ||
            parent->kind() == NodeKind::Abstract ||
            parent->kind() == NodeKind::Primitive) {
            return;
        }
    }

    qCDebug(KDEV_JULIA) << "Creating use for identifier:" << node->text() << "range:" << node->range();
    
    KDevelop::QualifiedIdentifier id = identifierForNode(node);
    KDevelop::RangeInRevision range = editorFindRange(node, node);
    
    qCDebug(KDEV_JULIA) << "  editorFindRange returned:" << range;
    
    // Skip if parent is a declaration: Equals/Assignment with this as first child, or TypeAnnotation (variable name in type annotation)
    if (AstNode* parent = node->parent()) {
        if ((parent->kind() == NodeKind::Equals || parent->kind() == NodeKind::Assignment) && 
            parent->firstChild() == node) {
            qCDebug(KDEV_JULIA) << "  Skipping - declaration position (Equals/Assignment)";
            return;
        }
        if (parent->kind() == NodeKind::TypeAnnotation && parent->firstChild() == node) {
            qCDebug(KDEV_JULIA) << "  Skipping - variable in type annotation";
            return;
        }
    }
    
    {
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        
        // Search in current context
        auto* ctx = currentContext();
        
        if (ctx) {
            auto decls = ctx->findDeclarations(id, range.start);
            qCDebug(KDEV_JULIA) << "  Context:" << ctx << "findDeclarations for" << id << "at" << range.start << "found:" << decls.size();
            for (auto* d : decls) {
                qCDebug(KDEV_JULIA) << "    Declaration:" << d->identifier().toString() << "range:" << d->range();
            }
        } else {
            qCDebug(KDEV_JULIA) << "  No context!";
        }
    }
    
    newUse(node);
}

void UseBuilder::visitCall(AstNode* node)
{
    if (!node) {
        return;
    }

    if (AstNode* funcName = node->firstChild()) {
        if (funcName->kind() == NodeKind::Identifier) {
            qCDebug(KDEV_JULIA) << "Creating use for function call:" << funcName->text();
            newUse(funcName);
        }
    }
}

KDevelop::RangeInRevision UseBuilder::editorFindRange(AstNode* fromNode, AstNode* toNode)
{
    if (!fromNode || !toNode) {
        return KDevelop::RangeInRevision(0, 0, 0, 0);
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
