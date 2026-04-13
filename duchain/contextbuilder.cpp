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
    visitNode(node);
    qCDebug(KDEV_JULIA) << "<<< ContextBuilder::startVisiting DONE";
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
    
    qCDebug(KDEV_JULIA) << "ContextBuilder::visitFunctionDefinition:" << (node->name ? node->name->value : QStringLiteral("unknown"));
    
    // First, process function arguments - creates DUContext of type Function
    if (node->arguments) {
        visitFunctionArguments(node);
    }
    
    // Second, process function body - creates DUContext of type Other
    if (!node->body.isEmpty()) {
        visitFunctionBody(node);
    }
}

void ContextBuilder::visitFunctionArguments(FunctionDefinitionAst* node)
{
    if (!node || !node->arguments) return;
    
    qCDebug(KDEV_JULIA) << "ContextBuilder::visitFunctionArguments";
    
    // Calculate range for arguments context
    ArgumentsAst* args = node->arguments;
    KDevelop::RangeInRevision range = args->range();
    
    // Open context of type Function for parameters
    KDevelop::QualifiedIdentifier funcIdent;
    if (node->name) {
        funcIdent = KDevelop::QualifiedIdentifier(node->name->value);
    }
    
    openContext(args, range, KDevelop::DUContext::Function, funcIdent);
    
    // Visit argument nodes to create declarations for parameters
    visitNode(args);
    
    // Close the arguments context
    closeContext();
}

void ContextBuilder::visitFunctionBody(FunctionDefinitionAst* node)
{
    if (!node || node->body.isEmpty()) return;
    
    qCDebug(KDEV_JULIA) << "ContextBuilder::visitFunctionBody";
    
    // Calculate range for body context
    Ast* firstBody = node->body.first();
    Ast* lastBody = node->body.last();
    KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol, 
                                          lastBody->endLine, lastBody->endCol);
    
    // Open context of type Other for function body
    KDevelop::QualifiedIdentifier funcIdent;
    if (node->name) {
        funcIdent = KDevelop::QualifiedIdentifier(node->name->value);
    }
    
    openContext(node, bodyRange, KDevelop::DUContext::Other, funcIdent);
    
    // Import parent context (argument context) into body scope
    // This makes parameter names visible in the function body
    addImportedContexts();
    
    // Visit all statements in the body
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
    
    closeContext();
}

void ContextBuilder::visitAssignment(AssignmentAst* node)
{
    if (!node) return;
    for (auto* target : node->targets) {
        if (target) visitNode(target);
    }
    if (node->value) visitNode(node->value);
}

void ContextBuilder::visitIf(IfAst* node)
{
    if (!node) return;
    if (node->condition) visitNode(node->condition);
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
    for (auto* stmt : node->orelse) {
        if (stmt) visitNode(stmt);
    }
}

void ContextBuilder::visitFor(ForAst* node)
{
    if (!node) return;
    if (node->target) visitNode(node->target);
    if (node->iterator) visitNode(node->iterator);
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
}

void ContextBuilder::visitWhile(WhileAst* node)
{
    if (!node) return;
    if (node->condition) visitNode(node->condition);
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
}

void ContextBuilder::visitTry(TryAst* node)
{
    if (!node) return;
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
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
    
    if (!node->body.isEmpty()) {
        Ast* firstBody = node->body.first();
        Ast* lastBody = node->body.last();
        KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol, 
                                            lastBody->endLine, lastBody->endCol);
        openContext(node, bodyRange, KDevelop::DUContext::Namespace, moduleName);
        
        for (auto* stmt : node->body) {
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
    
    if (!node->body.isEmpty()) {
        Ast* firstBody = node->body.first();
        Ast* lastBody = node->body.last();
        KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol, 
                                            lastBody->endLine, lastBody->endCol);
        openContext(node, bodyRange, KDevelop::DUContext::Namespace, moduleName);
        
        for (auto* stmt : node->body) {
            if (stmt) visitNode(stmt);
        }
        
        closeContext();
    }
}

void ContextBuilder::visitStruct(StructAst* node)
{
    if (!node) return;
    
    KDevelop::QualifiedIdentifier typeName;
    if (node->name) {
        typeName = KDevelop::QualifiedIdentifier(node->name->value);
    }
    
    // Struct body creates a context
    if (!node->body.isEmpty()) {
        Ast* firstBody = node->body.first();
        Ast* lastBody = node->body.last();
        KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol,
                                            lastBody->endLine, lastBody->endCol);
        openContext(node, bodyRange, KDevelop::DUContext::Class, typeName);
        
        for (auto* stmt : node->body) {
            if (stmt) visitNode(stmt);
        }
        
        closeContext();
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
    if (node->name) {
        macroName = KDevelop::QualifiedIdentifier(node->name->value);
    }
    
    // Macro body creates context
    if (!node->body.isEmpty()) {
        Ast* firstBody = node->body.first();
        Ast* lastBody = node->body.last();
        KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol,
                                            lastBody->endLine, lastBody->endCol);
        openContext(node, bodyRange, KDevelop::DUContext::Other, macroName);
        
        for (auto* stmt : node->body) {
            if (stmt) visitNode(stmt);
        }
        
        closeContext();
    }
}

void ContextBuilder::visitLet(LetAst* node)
{
    if (!node) return;
    
    // Let creates a new scope for bindings
    if (!node->body.isEmpty()) {
        Ast* firstBody = node->body.first();
        Ast* lastBody = node->body.last();
        KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol,
                                            lastBody->endLine, lastBody->endCol);
        openContext(node, bodyRange, KDevelop::DUContext::Other, KDevelop::QualifiedIdentifier());
        
        // Visit bindings first (they create local variables)
        for (auto* binding : node->bindings) {
            if (binding) visitNode(binding);
        }
        
        // Then visit body
        for (auto* stmt : node->body) {
            if (stmt) visitNode(stmt);
        }
        
        closeContext();
    }
}

void ContextBuilder::visitDo(DoAst* node)
{
    if (!node) return;
    
    // Do block creates a context
    if (!node->body.isEmpty()) {
        Ast* firstBody = node->body.first();
        Ast* lastBody = node->body.last();
        KDevelop::RangeInRevision bodyRange(firstBody->startLine, firstBody->startCol,
                                            lastBody->endLine, lastBody->endCol);
        openContext(node, bodyRange, KDevelop::DUContext::Function, KDevelop::QualifiedIdentifier());
        
        for (auto* stmt : node->body) {
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
