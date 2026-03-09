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

void ContextBuilder::visitFunction(FunctionNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitFunction";
    
    qCDebug(KDEV_JULIA) << "  Function name:" << node->functionName() << "range:" << node->range();
    
    // Step 1: Parse parameters into parameter context
    visitFunctionParameters(node, node);
    
    // Step 2: Parse body into body context (imports parameter context)
    visitFunctionBody(node, node);
    
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitFunction DONE";
}

void ContextBuilder::visitFunctionParameters(AstNode* node, FunctionNode* funcNode)
{
    KDevelop::QualifiedIdentifier funcId = extractFunctionId(funcNode);
    qCDebug(KDEV_JULIA) << "ContextBuilder::visitFunctionParameters:" << funcId.toString()
                         << "compilingContexts=" << compilingContexts();
    
    AstNode* argsNode = funcNode->arguments();
    if (!argsNode) {
        qCDebug(KDEV_JULIA) << "  No arguments node!";
        return;
    }
    
    // Check if context already exists on the argsNode (for second pass)
    if (argsNode->context) {
        qCDebug(KDEV_JULIA) << "  Reusing existing Function context from argsNode";
        openContext(argsNode->context);
    } else {
        KDevelop::RangeInRevision range = rangeForArgumentsContext(funcNode);
        qCDebug(KDEV_JULIA) << "  Opening Function context, range=" << range;
        openContext(argsNode, range, KDevelop::DUContext::Function, funcId);
    }
    
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
    qCDebug(KDEV_JULIA) << "ContextBuilder::visitFunctionBody:" << funcId.toString()
                         << "compilingContexts=" << compilingContexts();
    
    AstNode* bodyNode = funcNode->body();
    if (!bodyNode) {
        qCDebug(KDEV_JULIA) << "  No body node!";
        return;
    }
    
    // Check if context already exists on the bodyNode (for second pass)
    if (bodyNode->context) {
        qCDebug(KDEV_JULIA) << "  Reusing existing Other context from bodyNode";
        openContext(bodyNode->context);
    } else {
        KDevelop::RangeInRevision bodyRange = bodyNode->range();
        qCDebug(KDEV_JULIA) << "  Opening Other context for body, range=" << bodyRange;
        openContext(bodyNode, bodyRange, KDevelop::DUContext::Other, funcId);
    }
    
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
    
    AstNode* nameNode = funcNode->functionNameNode();
    KDevelop::CursorInRevision start = nameNode ? nameNode->range().start : params.first()->range().start;
    
    AstNode* lastParam = params.last();
    AstNode* lastId = lastParam->kind() == NodeKind::TypeAnnotation 
        ? lastParam->firstChild() : lastParam;
    KDevelop::CursorInRevision end = lastId ? lastId->range().end : lastParam->range().end;
    
    return KDevelop::RangeInRevision(start, end);
}

void ContextBuilder::visitStruct(StructNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitStruct (traverse only)";
    
    // Just traverse children - context is created by visitStructBody
    JuliaAstDefaultVisitor::visitStruct(node);
    
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitStruct DONE";
}

void ContextBuilder::visitStructBody(StructNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitStructBody (create context)";
    
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
    
    // Check if context already exists on the node (for second pass)
    if (node->context) {
        qCDebug(KDEV_JULIA) << "  Reusing existing Class context";
        openContext(node->context);
    } else {
        KDevelop::RangeInRevision range = node->range();
        openContext(node, range, KDevelop::DUContext::Class, structId);
    }
    
    // Traverse children using base class
    JuliaAstDefaultVisitor::visitStruct(node);
    
    closeContext();
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitStructBody DONE";
}

void ContextBuilder::visitModule(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitModule (traverse only)";
    
    // Just traverse children - context is created by visitModuleBody
    JuliaAstDefaultVisitor::visitModule(node);
    
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitModule DONE";
}

void ContextBuilder::visitBaremodule(BaremoduleNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitBaremodule (traverse only)";
    JuliaAstDefaultVisitor::visitBaremodule(node);
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitBaremodule DONE";
}

void ContextBuilder::visitBaremoduleBody(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitBaremoduleBody (create context)";
    
    KDevelop::QualifiedIdentifier moduleId;
    if (node->parent()) {
        AstNode* nameNode = node->parent()->firstChild();
        if (nameNode && nameNode->kind() == NodeKind::Identifier) {
            moduleId = KDevelop::QualifiedIdentifier(nameNode->text());
        }
    }
    
    if (node->context) {
        openContext(node->context);
    } else {
        openContext(node, node->range(), KDevelop::DUContext::Namespace, moduleId);
    }
    
    JuliaAstDefaultVisitor::visitBaremodule(static_cast<BaremoduleNode*>(node->parent()));
    closeContext();
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitBaremoduleBody DONE";
}

void ContextBuilder::visitModuleBody(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitModuleBody (create context)";
    
    AstNode* nameNode = node->firstChild();
    
    KDevelop::QualifiedIdentifier moduleId;
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        moduleId = KDevelop::QualifiedIdentifier(nameNode->text());
    }
    qCDebug(KDEV_JULIA) << "  Module name:" << moduleId.toString() << "range:" << node->range();
    
    // Check if context already exists on the node (for second pass)
    if (node->context) {
        qCDebug(KDEV_JULIA) << "  Reusing existing Namespace context";
        openContext(node->context);
    } else {
        KDevelop::RangeInRevision range = node->range();
        openContext(node, range, KDevelop::DUContext::Namespace, moduleId);
    }
    
    // Traverse children using base class
    JuliaAstDefaultVisitor::visitModule(node);
    
    closeContext();
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitModuleBody DONE";
}

void ContextBuilder::visitAbstract(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitAbstract (traverse only)";
    
    // Just traverse children - context is created by visitAbstractBody
    JuliaAstDefaultVisitor::visitAbstract(node);
    
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitAbstract DONE";
}

void ContextBuilder::visitAbstractBody(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitAbstractBody (create context)";
    
    AstNode* nameNode = node->firstChild();
    KDevelop::QualifiedIdentifier abstractId;
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        abstractId = KDevelop::QualifiedIdentifier(nameNode->text());
    }
    qCDebug(KDEV_JULIA) << "  Abstract name:" << abstractId.toString();
    
    // Check if context already exists on the node (for second pass)
    if (node->context) {
        qCDebug(KDEV_JULIA) << "  Reusing existing Class context";
        openContext(node->context);
    } else {
        KDevelop::RangeInRevision range = node->range();
        openContext(node, range, KDevelop::DUContext::Class, abstractId);
    }
    
    JuliaAstDefaultVisitor::visitAbstract(node);
    
    closeContext();
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitAbstractBody DONE";
}

void ContextBuilder::visitPrimitive(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitPrimitive (traverse only)";
    
    // Just traverse children - context is created by visitPrimitiveBody
    JuliaAstDefaultVisitor::visitPrimitive(node);
    
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitPrimitive DONE";
}

void ContextBuilder::visitPrimitiveBody(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitPrimitiveBody (create context)";
    
    AstNode* nameNode = node->firstChild();
    KDevelop::QualifiedIdentifier primitiveId;
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        primitiveId = KDevelop::QualifiedIdentifier(nameNode->text());
    }
    qCDebug(KDEV_JULIA) << "  Primitive name:" << primitiveId.toString();
    
    // Check if context already exists on the node (for second pass)
    if (node->context) {
        qCDebug(KDEV_JULIA) << "  Reusing existing Class context";
        openContext(node->context);
    } else {
        KDevelop::RangeInRevision range = node->range();
        openContext(node, range, KDevelop::DUContext::Class, primitiveId);
    }
    
    JuliaAstDefaultVisitor::visitPrimitive(node);
    
    closeContext();
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitPrimitiveBody DONE";
}

void ContextBuilder::visitBlock(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitBlock:" << node->range();
    
    // Check if context already exists on the node (for second pass)
    if (node->context) {
        qCDebug(KDEV_JULIA) << "  Reusing existing Other context";
        openContext(node->context);
    } else {
        KDevelop::RangeInRevision range = node->range();
        openContext(node, range, KDevelop::DUContext::Other, KDevelop::QualifiedIdentifier());
    }
    
    // Traverse children using base class
    JuliaAstDefaultVisitor::visitBlock(node);
    
    closeContext();
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitBlock DONE";
}

KDevelop::DUContext* ContextBuilder::contextFromNode(AstNode* node)
{
    if (!node) {
        qCDebug(KDEV_JULIA) << "contextFromNode: node is null";
        return nullptr;
    }
    qCDebug(KDEV_JULIA) << "contextFromNode: node kind=" << nodeKindToString(node->kind()) 
                         << "has context=" << (node->context != nullptr)
                         << "compilingContexts=" << compilingContexts();
    return node->context;
}

void ContextBuilder::setContextOnNode(AstNode* node, KDevelop::DUContext* context)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << "setContextOnNode: node kind=" << nodeKindToString(node->kind()) 
                         << "context=" << context << "compilingContexts=" << compilingContexts();
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

void ContextBuilder::visitTry(TryNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitTry";
    JuliaAstDefaultVisitor::visitTry(node);
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitTry DONE";
}

void ContextBuilder::visitLet(LetNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitLet";
    JuliaAstDefaultVisitor::visitLet(node);
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitLet DONE";
}

void ContextBuilder::visitDo(DoNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitDo";
    JuliaAstDefaultVisitor::visitDo(node);
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::visitDo DONE";
}

}
