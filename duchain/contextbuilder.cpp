#include "contextbuilder.h"

#include <QDebug>

#include <language/duchain/duchain.h>
#include <language/duchain/ducontext.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/parsingenvironment.h>

#include "../parser/ast.h"
#include "juliaeditorintegrator.h"
#include "juliadebug.h"

namespace Julia {

ContextBuilder::ContextBuilder() = default;

ContextBuilder::~ContextBuilder() = default;

void ContextBuilder::setEditor(JuliaEditorIntegrator* editor)
{
    m_editor = editor;
}

JuliaEditorIntegrator* ContextBuilder::editor() const
{
    return m_editor;
}

KDevelop::DUContext* ContextBuilder::newContext(const KDevelop::RangeInRevision& range)
{
    return new JuliaNormalDUContext(range, currentContext());
}

KDevelop::TopDUContext* ContextBuilder::newTopContext(const KDevelop::RangeInRevision& range,
                                                       KDevelop::ParsingEnvironmentFile* file)
{
    if (!file) {
        file = new KDevelop::ParsingEnvironmentFile(document());
        file->setLanguage(KDevelop::IndexedString("julia"));
    }
    return new JuliaTopDUContext(document(), range, file);
}

void ContextBuilder::startVisiting(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::startVisiting:" << nodeKindToString(node->kind());
    qCDebug(KDEV_JULIA) << "  Calling visitNode...";
    visitNode(node);
    qCDebug(KDEV_JULIA) << "  visitNode returned";
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::startVisiting DONE";
}

void ContextBuilder::visitTopLevel(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitTopLevel";
    JuliaAstDefaultVisitor::visitTopLevel(node);
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitTopLevel DONE";
}

void ContextBuilder::visitFunction(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitFunction";
    
    FunctionNode* funcNode = dynamic_cast<FunctionNode*>(node);
    if (!funcNode) {
        JuliaAstDefaultVisitor::visitFunction(node);
        return;
    }
    
    qCDebug(KDEV_JULIA) << "  Function name:" << funcNode->functionName() << "range:" << node->range();
    
    // Step 1: Parse parameters into parameter context
    visitFunctionParameters(node, funcNode);
    
    // Step 2: Parse body into body context (imports parameter context)
    visitFunctionBody(node, funcNode);
    
    // Step 3: Continue traversal for remaining children (type parameters, etc.)
    JuliaAstDefaultVisitor::visitFunction(node);
    
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitFunction DONE";
}

void ContextBuilder::visitFunctionParameters(AstNode* node, FunctionNode* funcNode)
{
    KDevelop::QualifiedIdentifier funcId = extractFunctionId(funcNode);
    qCDebug(KDEV_JULIA) << "ContextBuilder::visitFunctionParameters:" << funcId.toString();
    
    AstNode* argsNode = funcNode->arguments();
    if (!argsNode) {
        qCDebug(KDEV_JULIA) << "  No arguments node!";
        return;
    }
    
    KDevelop::RangeInRevision range = rangeForArgumentsContext(funcNode);
    qCDebug(KDEV_JULIA) << "  Opening Function context, range=" << range;
    
    openContext(argsNode, range, KDevelop::DUContext::Function, funcId);
    
    for (AstNode* child : node->children()) {
        if (!child) continue;
        if (child == funcNode->body()) {
            continue;
        }
        startVisiting(child);
    }
    
    m_importedParentContexts.append(currentContext());
    qCDebug(KDEV_JULIA) << "  Closing Function context, will import to body";
    
    closeContext();
}

void ContextBuilder::visitFunctionBody(AstNode* /*node*/, FunctionNode* funcNode)
{
    KDevelop::QualifiedIdentifier funcId = extractFunctionId(funcNode);
    qCDebug(KDEV_JULIA) << "ContextBuilder::visitFunctionBody:" << funcId.toString();
    
    AstNode* bodyNode = funcNode->body();
    if (!bodyNode) {
        qCDebug(KDEV_JULIA) << "  No body node!";
        return;
    }
    
    KDevelop::RangeInRevision bodyRange = bodyNode->range();
    qCDebug(KDEV_JULIA) << "  Opening Other context for body, range=" << bodyRange;
    openContext(bodyNode, bodyRange, KDevelop::DUContext::Other, funcId);
    
    // Set local scope identifier to help findDeclarations resolve identifiers
    {
        KDevelop::DUChainWriteLocker lock;
        currentContext()->setLocalScopeIdentifier(funcId);
    }
    
    qCDebug(KDEV_JULIA) << "  Body context=" << currentContext() << "will import" << m_importedParentContexts.size() << "parent contexts";
    addImportedContexts();
    
    startVisiting(bodyNode);
    
    closeContext();
}

void ContextBuilder::addImportedContexts()
{
    qCDebug(KDEV_JULIA) << "ContextBuilder::addImportedContexts: importing" << m_importedParentContexts.size() << "contexts";
    if (compilingContexts() && !m_importedParentContexts.isEmpty()) {
        for (KDevelop::DUContext* imported : m_importedParentContexts) {
            qCDebug(KDEV_JULIA) << "    Importing context:" << imported << "type=" << imported->type();
            currentContext()->addImportedParentContext(imported);
        }
        m_importedParentContexts.clear();
    }
}

AstNode* ContextBuilder::extractFunctionNameNode(FunctionNode* funcNode)
{
    if (!funcNode) return nullptr;
    
    AstNode* nameNode = funcNode->firstChild();
    if (!nameNode) return nullptr;
    
    if (nameNode->kind() == NodeKind::Where) {
        AstNode* inner = nameNode->firstChild();
        if (inner && inner->kind() == NodeKind::TypeAnnotation) {
            inner = inner->firstChild();
        }
        if (inner && inner->kind() == NodeKind::Call) {
            return inner->firstChild();
        }
        return nullptr;
    } else if (nameNode->kind() == NodeKind::TypeAnnotation) {
        AstNode* callNode = nameNode->firstChild();
        if (callNode && callNode->kind() == NodeKind::Call) {
            return callNode->firstChild();
        }
    } else if (nameNode->kind() == NodeKind::Call) {
        return nameNode->firstChild();
    }
    
    return nameNode;
}

KDevelop::QualifiedIdentifier ContextBuilder::extractFunctionId(FunctionNode* funcNode)
{
    AstNode* nameNode = extractFunctionNameNode(funcNode);
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        return KDevelop::QualifiedIdentifier(nameNode->text());
    }
    return KDevelop::QualifiedIdentifier();
}

KDevelop::RangeInRevision ContextBuilder::rangeForArgumentsContext(FunctionNode* funcNode)
{
    AstNode* argsNode = funcNode->arguments();
    if (!argsNode) {
        return KDevelop::RangeInRevision();
    }
    
    QList<AstNode*> params = funcNode->parameters();
    if (params.isEmpty()) {
        return argsNode->range();
    }
    
    AstNode* lastParam = params.last();
    return lastParam->range();
}

void ContextBuilder::visitStruct(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitStruct";
    
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
    
    qCDebug(KDEV_JULIA) << "  Struct name:" << structId.toString() << "range:" << node->range();
    
    KDevelop::RangeInRevision range = node->range();
    
    openContext(node, range, KDevelop::DUContext::Class, structId);
    
    // Traverse children using base class
    JuliaAstDefaultVisitor::visitStruct(node);
    
    closeContext();
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitStruct DONE";
}

void ContextBuilder::visitModule(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitModule";
    
    AstNode* nameNode = node->firstChild();
    
    KDevelop::QualifiedIdentifier moduleId;
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        moduleId = KDevelop::QualifiedIdentifier(nameNode->text());
    }
    qCDebug(KDEV_JULIA) << "  Module name:" << moduleId.toString() << "range:" << node->range();
    
    KDevelop::RangeInRevision range = node->range();
    
    openContext(node, range, KDevelop::DUContext::Namespace, moduleId);
    
    // Traverse children using base class
    JuliaAstDefaultVisitor::visitModule(node);
    
    closeContext();
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitModule DONE";
}

void ContextBuilder::visitBlock(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitBlock:" << node->range();
    
    KDevelop::RangeInRevision range = node->range();
    
    openContext(node, range, KDevelop::DUContext::Other, KDevelop::QualifiedIdentifier());
    
    // Traverse children using base class
    JuliaAstDefaultVisitor::visitBlock(node);
    
    closeContext();
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitBlock DONE";
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
    
    if (m_editor) {
        return m_editor->findRange(fromNode, toNode);
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
