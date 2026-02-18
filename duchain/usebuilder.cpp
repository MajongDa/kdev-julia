#include "usebuilder.h"

#include <QDebug>

#include <language/duchain/duchain.h>

#include "../parser/ast.h"
#include "juliadebug.h"
#include "contextbuilder.h"

namespace Julia {

UseBuilder::UseBuilder() = default;

UseBuilder::~UseBuilder() = default;

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
