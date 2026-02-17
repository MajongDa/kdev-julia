#include "contextbuilder.h"

#include <QDebug>

#include <language/duchain/duchain.h>
#include <language/duchain/ducontext.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/parsingenvironment.h>

#include "../parser/ast.h"
#include "juliadebug.h"

namespace Julia {

ContextBuilder::ContextBuilder() = default;

ContextBuilder::~ContextBuilder() = default;

KDevelop::DUContext* ContextBuilder::newContext(const KDevelop::RangeInRevision& range)
{
    return new KDevelop::DUContext(range, currentContext());
}

KDevelop::TopDUContext* ContextBuilder::newTopContext(const KDevelop::RangeInRevision& range,
                                                       KDevelop::ParsingEnvironmentFile* file)
{
    if (!file) {
        file = new KDevelop::ParsingEnvironmentFile(document());
        file->setLanguage(KDevelop::IndexedString("julia"));
    }
    return new KDevelop::TopDUContext(document(), range, file);
}

void ContextBuilder::startVisiting(AstNode* node)
{
    if (!node) {
        return;
    }

    switch (node->kind()) {
        case NodeKind::Function:
            visitFunction(node);
            break;
        case NodeKind::Struct:
            visitStruct(node);
            break;
        case NodeKind::Module:
            visitModule(node);
            break;
        case NodeKind::Block:
            visitBlock(node);
            break;
        case NodeKind::For:
            visitFor(node);
            break;
        case NodeKind::While:
            visitWhile(node);
            break;
        case NodeKind::If:
            visitIf(node);
            break;
        default:
            for (AstNode* child : node->children()) {
                startVisiting(child);
            }
            break;
    }
}

void ContextBuilder::visitFunction(AstNode* node)
{
    if (!node) {
        return;
    }
    
    AstNode* nameNode = node->firstChild();
    if (nameNode && nameNode->kind() == NodeKind::Call) {
        nameNode = nameNode->firstChild();
    }
    
    KDevelop::QualifiedIdentifier funcId;
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        funcId = KDevelop::QualifiedIdentifier(nameNode->text());
    }
    
    KDevelop::RangeInRevision range = node->range();
    
    openContext(node, range, KDevelop::DUContext::Function, funcId);
    
    for (AstNode* child : node->children()) {
        if (child && child != nameNode) {
            startVisiting(child);
        }
    }
    
    closeContext();
}

void ContextBuilder::visitStruct(AstNode* node)
{
    if (!node) {
        return;
    }
    
    AstNode* nameNode = node->firstChild();
    
    // Handle parametric struct: struct Foo{T} ...
    // First child is Curly node, not Identifier
    if (nameNode && nameNode->kind() == NodeKind::Curly) {
        if (CurlyNode* curly = dynamic_cast<CurlyNode*>(nameNode)) {
            nameNode = curly->firstChild();
        }
    }
    
    KDevelop::QualifiedIdentifier structId;
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        structId = KDevelop::QualifiedIdentifier(nameNode->text());
    }
    
    KDevelop::RangeInRevision range = node->range();
    
    openContext(node, range, KDevelop::DUContext::Class, structId);
    
    for (AstNode* child : node->children()) {
        if (child && child != nameNode) {
            startVisiting(child);
        }
    }
    
    closeContext();
}

void ContextBuilder::visitModule(AstNode* node)
{
    if (!node) {
        return;
    }
    
    AstNode* nameNode = node->firstChild();
    
    KDevelop::QualifiedIdentifier moduleId;
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        moduleId = KDevelop::QualifiedIdentifier(nameNode->text());
    }
    
    KDevelop::RangeInRevision range = node->range();
    
    openContext(node, range, KDevelop::DUContext::Namespace, moduleId);
    
    for (AstNode* child : node->children()) {
        if (child && child != nameNode) {
            startVisiting(child);
        }
    }
    
    closeContext();
}

void ContextBuilder::visitBlock(AstNode* node)
{
    if (!node) {
        return;
    }
    
    KDevelop::RangeInRevision range = node->range();
    
    openContext(node, range, KDevelop::DUContext::Other, KDevelop::QualifiedIdentifier());
    
    for (AstNode* child : node->children()) {
        startVisiting(child);
    }
    
    closeContext();
}

void ContextBuilder::visitFor(AstNode* node)
{
    if (!node) {
        return;
    }
    
    KDevelop::RangeInRevision range = node->range();
    
    openContext(node, range, KDevelop::DUContext::Other, KDevelop::QualifiedIdentifier());
    
    for (AstNode* child : node->children()) {
        startVisiting(child);
    }
    
    closeContext();
}

void ContextBuilder::visitWhile(AstNode* node)
{
    if (!node) {
        return;
    }
    
    KDevelop::RangeInRevision range = node->range();
    
    openContext(node, range, KDevelop::DUContext::Other, KDevelop::QualifiedIdentifier());
    
    for (AstNode* child : node->children()) {
        startVisiting(child);
    }
    
    closeContext();
}

void ContextBuilder::visitIf(AstNode* node)
{
    if (!node) {
        return;
    }
    
    KDevelop::RangeInRevision range = node->range();
    
    openContext(node, range, KDevelop::DUContext::Other, KDevelop::QualifiedIdentifier());
    
    for (AstNode* child : node->children()) {
        startVisiting(child);
    }
    
    closeContext();
}

KDevelop::DUContext* ContextBuilder::contextFromNode(AstNode* node)
{
    if (!node) {
        return nullptr;
    }
    return node->context;
}

void ContextBuilder::setContextOnNode(AstNode* node, KDevelop::DUContext* context)
{
    if (!node) {
        return;
    }
    node->context = context;
}

KDevelop::RangeInRevision ContextBuilder::editorFindRange(AstNode* fromNode, AstNode* toNode)
{
    if (!fromNode || !toNode) {
        return KDevelop::RangeInRevision(0, 0, 0, 0);
    }
    return fromNode->range();
}

KDevelop::RangeInRevision ContextBuilder::editorFindRangeForContext(AstNode* fromNode, AstNode* toNode)
{
    return editorFindRange(fromNode, toNode);
}

KDevelop::QualifiedIdentifier ContextBuilder::identifierForNode(AstNode* node)
{
    if (!node) {
        return KDevelop::QualifiedIdentifier();
    }
    return KDevelop::QualifiedIdentifier(node->text());
}

}
