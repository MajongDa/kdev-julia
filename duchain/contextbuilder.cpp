#include "contextbuilder.h"

#include <QDebug>

#include <language/duchain/duchain.h>
#include <language/duchain/ducontext.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/parsingenvironment.h>

#include "parser/ast.h"
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

void ContextBuilder::startVisiting(Ast* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::startVisiting:" << node->dump();
    qCDebug(KDEV_JULIA) << "  ContextBuilder: currentContext before visitNode:" << currentContext();
    visitNode(node);
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::startVisiting DONE, currentContext:" << currentContext();
}

KDevelop::DUContext* ContextBuilder::contextFromNode(Ast* node)
{
    if (!node) {
        return nullptr;
    }
    return node->context;
}

void ContextBuilder::setContextOnNode(Ast* node, KDevelop::DUContext* context)
{
    if (!node) {
        return;
    }
    node->context = context;
}

KDevelop::RangeInRevision ContextBuilder::editorFindRange(Ast* fromNode, Ast* toNode)
{
    if (!fromNode || !toNode) {
        return KDevelop::RangeInRevision(0, 0, 0, 0);
    }
    if (m_editor) {
        return m_editor->findRange(fromNode, toNode);
    }
    return fromNode->range();
}

KDevelop::RangeInRevision ContextBuilder::editorFindRangeForContext(Ast* fromNode, Ast* toNode)
{
    return editorFindRange(fromNode, toNode);
}

KDevelop::QualifiedIdentifier ContextBuilder::identifierForNode(IdentifierAst* node)
{
    if (!node) {
        return KDevelop::QualifiedIdentifier();
    }
    return KDevelop::QualifiedIdentifier(node->value);
}

void ContextBuilder::visitFunctionDefinition(FunctionDefinitionAst* node)
{
    if (!node) return;
    
    auto* sig = node->signature;
    qCDebug(KDEV_JULIA) << "ContextBuilder::visitFunctionDefinition:" << (sig ? sig->dump() : QStringLiteral("unknown"));

    if (!sig || !sig->rawSignature) return;

    // Visit rawSignature to process default value expressions for use tracking
    // Only for named functions — anonymous functions have TupleAst as rawSignature
    // whose elements are parameter identifiers, not use-tracking targets
    if (sig->name) {
        visitNode(sig->rawSignature);
    }

    // Second, process function body - creates DUContext of type Other
    if (node->block && !node->block->block.isEmpty()) {
        if (!node->block->context) {
            // Declaration phase didn't store body context
            JuliaAstDefaultVisitor::visitFunctionDefinition(node);
        } else {
            visitFunctionBody(node);
        }
    }
}

void ContextBuilder::visitCall(CallAst* node)
{
    if (!node) return;
    if (node->name) visitNode(node->name);
    for (auto* arg : node->arguments) {
        if (arg) visitNode(arg);
    }
}

void ContextBuilder::visitFunctionBody(FunctionDefinitionAst* node)
{
    if (!node || !node->block) return;
    
    qCDebug(KDEV_JULIA) << "ContextBuilder::visitFunctionBody";
    
    auto* sig = node->signature;
    KDevelop::QualifiedIdentifier funcIdent;
    if (sig && sig->name) {
        funcIdent = KDevelop::QualifiedIdentifier(sig->name->value);
    }
    
    auto& body = node->block->block;
    if (body.isEmpty()) return;

    Ast* firstBody = body.first();
    Ast* lastBody = body.last();
    KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol,
                                          lastBody->endLine, lastBody->endCol);
    
    openContext(node->block, bodyRange, KDevelop::DUContext::Other, funcIdent);
    addImportedContexts();
    JuliaAstDefaultVisitor::visitFunctionDefinition(node);
    closeContext();
}

void ContextBuilder::visitAssignment(AssignmentAst* node)
{
    if (!node) return;
    if (node->target) visitNode(node->target);
    if (node->value) visitNode(node->value);
}

void ContextBuilder::visitIf(IfAst* node)
{
    if (!node) return;
    if (node->condition) visitNode(node->condition);
    if (node->block) {
        for (auto* stmt : node->block->block) {
            if (stmt) visitNode(stmt);
        }
    }
    if (node->orelse) visitNode(node->orelse);
}

void ContextBuilder::visitFor(ForAst* node)
{
    if (!node) return;
    if (node->iterator) visitNode(node->iterator);
    if (node->block) {
        for (auto* stmt : node->block->block) {
            if (stmt) visitNode(stmt);
        }
    }
}

void ContextBuilder::visitWhile(WhileAst* node)
{
    if (!node) return;
    if (node->condition) visitNode(node->condition);
    if (node->block) {
        for (auto* stmt : node->block->block) {
            if (stmt) visitNode(stmt);
        }
    }
}

void ContextBuilder::visitTry(TryAst* node)
{
    if (!node) return;
    if (node->block) {
        for (auto* stmt : node->block->block) {
            if (stmt) visitNode(stmt);
        }
    }
}

void ContextBuilder::visitImport(ImportAst* node)
{
    // Import statements don't create their own context
    // Just visit children normally
    JuliaAstDefaultVisitor::visitImport(node);
}

void ContextBuilder::visitModule(ModuleAst* node)
{
    if (!node) return;
    
    KDevelop::QualifiedIdentifier moduleName;
    if (node->name) {
        moduleName = KDevelop::QualifiedIdentifier(node->name->value);
    }
    
    if (node->block && !node->block->block.isEmpty()) {
        Ast* firstBody = node->block->block.first();
        Ast* lastBody = node->block->block.last();
        KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol, 
                                            lastBody->endLine, lastBody->endCol);
        openContext(node, bodyRange, KDevelop::DUContext::Namespace, moduleName);
        
        for (auto* stmt : node->block->block) {
            if (stmt) visitNode(stmt);
        }
        
        closeContext();
    }
}

void ContextBuilder::visitBaremodule(BaremoduleAst* node)
{
    if (!node) return;
    
    KDevelop::QualifiedIdentifier moduleName;
    if (node->name) {
        moduleName = KDevelop::QualifiedIdentifier(node->name->value);
    }
    
    if (node->block && !node->block->block.isEmpty()) {
        Ast* firstBody = node->block->block.first();
        Ast* lastBody = node->block->block.last();
        KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol, 
                                            lastBody->endLine, lastBody->endCol);
        openContext(node, bodyRange, KDevelop::DUContext::Namespace, moduleName);
        
        for (auto* stmt : node->block->block) {
            if (stmt) visitNode(stmt);
        }
        
        closeContext();
    }
}

static IdentifierAst* extractNameFromSignature(Ast* sig)
{
    if (!sig) return nullptr;
    Ast* inner = sig;
    while (inner) {
        if (inner->astType == AstType::IdentifierAstType)
            return static_cast<IdentifierAst*>(inner);
        if (inner->astType == AstType::CallAstType)
            inner = static_cast<CallAst*>(inner)->name;
        else if (inner->astType == AstType::CurlyAstType)
            inner = static_cast<CurlyAst*>(inner)->name;
        else if (inner->astType == AstType::WhereAstType)
            inner = static_cast<WhereAst*>(inner)->signature;
        else
            break;
    }
    return nullptr;
}

void ContextBuilder::visitStruct(StructAst* node)
{
    if (!node) return;
    
    qCDebug(KDEV_JULIA) << ">>> ContextBuilder::visitStruct, currentContext before:" << currentContext();
    
    KDevelop::QualifiedIdentifier typeName;
    IdentifierAst* nameIdent = extractNameFromSignature(node->signature);
    if (nameIdent) {
        typeName = KDevelop::QualifiedIdentifier(nameIdent->value);
    }
    
    // Struct body creates a context
    if (node->block && !node->block->block.isEmpty()) {
        Ast* firstBody = node->block->block.first();
        Ast* lastBody = node->block->block.last();
        KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol,
                                            lastBody->endLine, lastBody->endCol);
        openContext(node, bodyRange, KDevelop::DUContext::Class, typeName);
        
        qCDebug(KDEV_JULIA) << "  ContextBuilder: openContext done, currentContext:" << currentContext();
        
        // Use base class visitor to visit children
        JuliaAstDefaultVisitor::visitStruct(node);
        
        // Only close if context is valid (prevent crash if cleared)
        if (currentContext()) {
            closeContext();
        }
    }
}

void ContextBuilder::visitAbstract(AbstractAst* node)
{
    if (!node) return;
    // Abstract types don't have body, no context needed
    JuliaAstDefaultVisitor::visitAbstract(node);
}

void ContextBuilder::visitPrimitive(PrimitiveAst* node)
{
    if (!node) return;
    // Primitive types don't have body, no context needed
    JuliaAstDefaultVisitor::visitPrimitive(node);
}

void ContextBuilder::visitMacro(MacroAst* node)
{
    if (!node) return;
    
    KDevelop::QualifiedIdentifier macroName;
    IdentifierAst* nameIdent = extractNameFromSignature(node->signature);
    if (nameIdent) {
        macroName = KDevelop::QualifiedIdentifier(nameIdent->value);
    }
    
    // Macro body creates context
    if (node->block && !node->block->block.isEmpty()) {
        Ast* firstBody = node->block->block.first();
        Ast* lastBody = node->block->block.last();
        KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol,
                                            lastBody->endLine, lastBody->endCol);
        openContext(node, bodyRange, KDevelop::DUContext::Other, macroName);
        
        for (auto* stmt : node->block->block) {
            if (stmt) visitNode(stmt);
        }
        
        closeContext();
    }
}

void ContextBuilder::visitLet(LetAst* node)
{
    if (!node) return;
    
    // Let creates a new scope for bindings
    if (!node->block.isEmpty()) {
        Ast* firstBody = node->block.first();
        Ast* lastBody = node->block.last();
        KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol,
                                            lastBody->endLine, lastBody->endCol);
        openContext(node, bodyRange, KDevelop::DUContext::Other, KDevelop::QualifiedIdentifier());
        
        // Visit bindings first (they create local variables)
        for (auto* binding : node->bindings) {
            if (binding) visitNode(binding);
        }
        
        // Then visit body
        for (auto* stmt : node->block) {
            if (stmt) visitNode(stmt);
        }
        
        closeContext();
    }
}

void ContextBuilder::visitDo(DoAst* node)
{
    if (!node) return;
    
    // Do block creates a context
    if (!node->block.isEmpty()) {
        Ast* firstBody = node->block.first();
        Ast* lastBody = node->block.last();
        KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol,
                                            lastBody->endLine, lastBody->endCol);
        openContext(node, bodyRange, KDevelop::DUContext::Function, KDevelop::QualifiedIdentifier());
        
        for (auto* stmt : node->block) {
            if (stmt) visitNode(stmt);
        }
        
        closeContext();
    }
}

void ContextBuilder::visitLambda(LambdaAst* node)
{
    if (!node) return;
    
    // Lambda creates a new context for its body
    // Currently we don't store body as separate - need to handle in expression visitor
    JuliaAstDefaultVisitor::visitLambda(node);
}

void ContextBuilder::visitGenerator(GeneratorAst* node)
{
    if (!node) return;
    
    // Generator creates a context for its filter expressions
    if (!node->filters.isEmpty()) {
        Ast* firstFilter = node->filters.first();
        Ast* lastFilter = node->filters.last();
        KDevelop::RangeInRevision filterRange(firstFilter->startLine, firstFilter->startCol,
                                              lastFilter->endLine, lastFilter->endCol);
        openContext(node, filterRange, KDevelop::DUContext::Other, KDevelop::QualifiedIdentifier());
        
        for (auto* filter : node->filters) {
            if (filter) visitNode(filter);
        }
        
        closeContext();
    }
}

void ContextBuilder::visitComprehension(ComprehensionAst* node)
{
    if (!node) return;
    
    // Comprehension creates a context
    openContext(node, node->range(), KDevelop::DUContext::Other, KDevelop::QualifiedIdentifier());
    
    JuliaAstDefaultVisitor::visitComprehension(node);
    
    closeContext();
}

void ContextBuilder::visitTypeAnnotation(TypeAnnotationAst* node)
{
    if (!node) return;
    qCDebug(KDEV_JULIA) << "ContextBuilder::visitTypeAnnotation";
    JuliaAstDefaultVisitor::visitTypeAnnotation(node);
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

}
