#include "declarationbuilder.h"

#include <QDebug>

#include <language/duchain/duchain.h>
#include <language/duchain/declaration.h>
#include <language/duchain/functiondeclaration.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/types/functiontype.h>
#include <language/duchain/types/structuretype.h>
#include <language/duchain/types/integraltype.h>
#include <language/editor/documentrange.h>

#include "parser/ast.h"
#include "expressionvisitor.h"
#include "juliaeditorintegrator.h"
#include "juliadebug.h"

namespace Julia {

static IdentifierAst* extractNameFromSignature(Ast* sig);

DeclarationBuilder::DeclarationBuilder(JuliaEditorIntegrator* editor)
    : m_editor(editor)
{
}

DeclarationBuilder::~DeclarationBuilder() = default;

KDevelop::RangeInRevision DeclarationBuilder::editorFindRange(Ast* fromNode, Ast* toNode)
{
    if (!fromNode || !toNode) {
        return KDevelop::RangeInRevision(0, 0, 0, 0);
    }
    if (m_editor) {
        return m_editor->findRange(fromNode, toNode);
    }
    return fromNode->range();
}

void DeclarationBuilder::setContextOnNode(Ast* node, KDevelop::DUContext* context)
{
    if (!node) return;
    node->context = context;
}

KDevelop::DUContext* DeclarationBuilder::contextFromNode(Ast* node)
{
    if (!node) return nullptr;
    return node->context;
}

KDevelop::QualifiedIdentifier DeclarationBuilder::identifierForNode(IdentifierAst* node)
{
    if (!node) {
        return KDevelop::QualifiedIdentifier();
    }
    return KDevelop::QualifiedIdentifier(node->value);
}

void DeclarationBuilder::visitNode(Ast* node)
{
    if (!node) return;
    
    switch (node->astType) {
        case AstType::FunctionDefinitionAstType:
            visitFunctionDefinition(static_cast<FunctionDefinitionAst*>(node));
            break;
        case AstType::AssignmentAstType:
            visitAssignment(static_cast<AssignmentAst*>(node));
            break;
        case AstType::ReturnAstType:
            visitReturn(static_cast<ReturnAst*>(node));
            break;
        case AstType::ImportAstType:
            visitImport(static_cast<ImportAst*>(node));
            break;
        case AstType::ForAstType:
            visitFor(static_cast<ForAst*>(node));
            break;
        case AstType::WhileAstType:
            visitWhile(static_cast<WhileAst*>(node));
            break;
        case AstType::GlobalAstType:
            visitGlobal(static_cast<GlobalAst*>(node));
            break;
        case AstType::LocalAstType:
            visitLocal(static_cast<LocalAst*>(node));
            break;
        case AstType::ModuleAstType:
            visitModule(static_cast<ModuleAst*>(node));
            break;
        case AstType::BaremoduleAstType:
            visitBaremodule(static_cast<BaremoduleAst*>(node));
            break;
        case AstType::StructAstType:
            visitStruct(static_cast<StructAst*>(node));
            break;
        case AstType::AbstractAstType:
            visitAbstract(static_cast<AbstractAst*>(node));
            break;
        case AstType::PrimitiveAstType:
            visitPrimitive(static_cast<PrimitiveAst*>(node));
            break;
        case AstType::MacroAstType:
            visitMacro(static_cast<MacroAst*>(node));
            break;
        case AstType::ConstAstType:
            visitConst(static_cast<ConstAst*>(node));
            break;
        default:
            AstVisitor::visitNode(node);
            break;
    }
}

void DeclarationBuilder::visitFunctionDefinition(FunctionDefinitionAst* node)
{
    auto* sig = node->signature;
    if (!sig) return;
    
    KDevelop::FunctionType::Ptr funcType(new KDevelop::FunctionType());
    
    if (sig->name) {
        qCDebug(KDEV_JULIA) << "DeclarationBuilder::visitFunctionDefinition:" << sig->name->value;
        
        KDevelop::QualifiedIdentifier ident(sig->name->value);
        KDevelop::RangeInRevision range = editorFindRange(sig->name, sig->name);
        
        {
            KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());
            KDevelop::FunctionDeclaration* funcDecl = DeclarationBuilderBase::openDeclaration<KDevelop::FunctionDeclaration>(ident, range);
            funcDecl->setType(funcType);
            funcDecl->setInSymbolTable(true);
        }
    } else {
        // Anonymous function: create a context scope for parameter declarations
        KDevelop::RangeInRevision range = editorFindRange(node, node);
        openContext(node, range, KDevelop::DUContext::Function, KDevelop::QualifiedIdentifier());
    }
    
    openType(funcType);
    
    // Process return type annotation
    if (sig->returnType) {
        ExpressionVisitor exprVisitor(currentContext());
        exprVisitor.visitNode(sig->returnType);
        KDevelop::AbstractType::Ptr returnType = exprVisitor.lastType();
        if (returnType) {
            KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());
            funcType->setReturnType(returnType);
        }
    }
    
    // Create parameter declarations from positional args
    for (auto* arg : sig->positionalArgs) {
        processParameter(arg, funcType);
    }
    
    // Create parameter declarations from keyword args
    for (auto* kw : sig->keywordArgs) {
        if (kw->astType == AstType::ParameterAstType) {
            auto* params = static_cast<ParameterAst*>(kw);
            for (auto* kwarg : params->kwargs) {
                processParameter(kwarg, funcType);
            }
        }
    }
    
    closeType();

    // Create function body context and store on node->block so the
    // use phase can reuse it (crash guard: body context must exist)
    if (node->block && !node->block->block.isEmpty()) {
        ContextBuilder::visitFunctionBody(node);
    }

    if (sig->name) {
        DeclarationBuilderBase::closeDeclaration();
    } else {
        closeContext();
    }
}

void DeclarationBuilder::processParameter(Ast* arg, KDevelop::FunctionType::Ptr funcType)
{
    if (!arg) return;
    
    QString paramName;
    KDevelop::AbstractType::Ptr paramType;
    Ast* inner = arg;
    
    // Unwrap EllipsisAst (varargs: x...)
    if (inner->astType == AstType::EllipsisAstType) {
        inner = static_cast<EllipsisAst*>(inner)->name;
        if (!inner) return;
    }
    
    // Unwrap TypeAnnotationAst (x::T or x::T=1)
    if (inner->astType == AstType::TypeAnnotationAstType) {
        auto* typeAnn = static_cast<TypeAnnotationAst*>(inner);
        if (typeAnn->type) {
            ExpressionVisitor exprVisitor(currentContext());
            exprVisitor.visitNode(typeAnn->type);
            paramType = exprVisitor.lastType();
        }
        inner = typeAnn->value;
    }
    
    // Unwrap AssignmentAst (x=1 or x::T=1)
    if (inner && inner->astType == AstType::AssignmentAstType) {
        auto* assign = static_cast<AssignmentAst*>(inner);
        if (!paramType && assign->value) {
            ExpressionVisitor exprVisitor(currentContext());
            exprVisitor.visitNode(assign->value);
            paramType = exprVisitor.lastType();
        }
        inner = assign->target;
    }
    
    // Extract parameter name
    if (inner && inner->astType == AstType::IdentifierAstType) {
        paramName = static_cast<IdentifierAst*>(inner)->value;
    }
    
    if (paramName.isEmpty()) return;
    
    // Fallback type
    if (!paramType) {
        paramType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed));
    }
    
    // Add to FunctionType
    funcType->addArgument(paramType);
    
    // Create declaration
    KDevelop::QualifiedIdentifier qident(paramName);
    KDevelop::RangeInRevision range = editorFindRange(inner, inner);
    
    {
        KDevelop::DUChainWriteLocker lock;
        KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
        decl->setKind(KDevelop::Declaration::Instance);
        decl->setInSymbolTable(true);
        decl->setType(paramType);
        DeclarationBuilderBase::closeDeclaration();
    }

}

void DeclarationBuilder::declareIdentifier(IdentifierAst* ident, KDevelop::AbstractType::Ptr type)
{
    if (!ident) return;
    KDevelop::QualifiedIdentifier qident(ident->value);
    KDevelop::RangeInRevision range = editorFindRange(ident, ident);
    KDevelop::DUChainWriteLocker lock;
    auto* decl = openDeclaration<KDevelop::Declaration>(qident, range);
    decl->setKind(KDevelop::Declaration::Instance);
    decl->setInSymbolTable(true);
    if (type) decl->setType(type);
    closeDeclaration();
}

void DeclarationBuilder::visitAssignment(AssignmentAst* node)
{
    if (!node || !node->target) return;

    JuliaAstDefaultVisitor::visitAssignment(node);

    if (node->target->astType == AstType::IdentifierAstType) {
        auto* ident = static_cast<IdentifierAst*>(node->target);
        KDevelop::AbstractType::Ptr type;
        if (node->value) {
            ExpressionVisitor v(currentContext());
            v.visitNode(node->value);
            type = v.lastType();
        }
        declareIdentifier(ident, type);
    }
    else if (node->target->astType == AstType::TypeAnnotationAstType) {
        visitTypeAnnotation(static_cast<TypeAnnotationAst*>(node->target));
    }
}

void DeclarationBuilder::visitReturn(ReturnAst* node)
{
    if (!node || !node->value) return;
    
    KDevelop::FunctionType::Ptr funcType = currentType<KDevelop::FunctionType>();
    if (!funcType) return;
    
    ExpressionVisitor exprVisitor(currentContext());
    exprVisitor.visitNode(node->value);
    KDevelop::AbstractType::Ptr returnType = exprVisitor.lastType();
    
    if (returnType) {
        KDevelop::DUChainWriteLocker lock;
        funcType->setReturnType(returnType);
    }
}

void DeclarationBuilder::visitImport(ImportAst* node)
{
    // Import handling - create module declarations
}

void DeclarationBuilder::visitFor(ForAst* node)
{
    if (!node || !node->iterator) return;
    
    // Handle loop variable declaration — unwrap InAst -> target
    Ast* iter = node->iterator;
    if (iter->astType == AstType::InAstType) {
        auto* inNode = static_cast<InAst*>(iter);
        if (inNode->target && inNode->target->astType == AstType::IdentifierAstType) {
            IdentifierAst* ident = static_cast<IdentifierAst*>(inNode->target);
            if (ident) {
                KDevelop::QualifiedIdentifier qident(ident->value);
                KDevelop::RangeInRevision range = editorFindRange(ident, ident);
                
                KDevelop::DUChainWriteLocker lock;
                KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
                decl->setKind(KDevelop::Declaration::Instance);
                decl->setInSymbolTable(true);
                DeclarationBuilderBase::closeDeclaration();
            }
        }
    }
}

void DeclarationBuilder::visitWhile(WhileAst* node)
{
    AstVisitor::visitWhile(node);
}

void DeclarationBuilder::visitGlobal(GlobalAst* node)
{
    if (!node) return;
    
    for (auto* name : node->names) {
        if (!name || name->astType != AstType::IdentifierAstType) continue;
        IdentifierAst* ident = static_cast<IdentifierAst*>(name);
        
        KDevelop::QualifiedIdentifier qident(ident->value);
        KDevelop::RangeInRevision range = editorFindRange(ident, ident);
        
        KDevelop::DUChainWriteLocker lock;
        KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
        decl->setKind(KDevelop::Declaration::Instance);
        decl->setInSymbolTable(true);
        DeclarationBuilderBase::closeDeclaration();
    }
}

void DeclarationBuilder::visitLocal(LocalAst* node)
{
    if (!node) return;
    
    for (auto* ident : node->names) {
        if (!ident) continue;
        
        KDevelop::QualifiedIdentifier qident(ident->value);
        KDevelop::RangeInRevision range = editorFindRange(ident, ident);
        
        KDevelop::DUChainWriteLocker lock;
        KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
        decl->setKind(KDevelop::Declaration::Instance);
        decl->setInSymbolTable(true);
        DeclarationBuilderBase::closeDeclaration();
    }
}

void DeclarationBuilder::visitModule(ModuleAst* node)
{
    if (!node || !node->name) return;
    
    KDevelop::QualifiedIdentifier qident(node->name->value);
    KDevelop::RangeInRevision range = editorFindRange(node->name, node->name);
    
    KDevelop::DUChainWriteLocker lock;
    KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
    decl->setKind(KDevelop::Declaration::Namespace);
    decl->setInSymbolTable(true);
    DeclarationBuilderBase::closeDeclaration();

    ContextBuilder::visitModule(node);
}

void DeclarationBuilder::visitBaremodule(BaremoduleAst* node)
{
    if (!node || !node->name) return;
    
    KDevelop::QualifiedIdentifier qident(node->name->value);
    KDevelop::RangeInRevision range = editorFindRange(node->name, node->name);
    
    KDevelop::DUChainWriteLocker lock;
    KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
    decl->setKind(KDevelop::Declaration::Namespace);
    decl->setInSymbolTable(true);
    DeclarationBuilderBase::closeDeclaration();

    ContextBuilder::visitBaremodule(node);
}

void DeclarationBuilder::visitStruct(StructAst* node)
{
    if (!node || !node->signature) return;
    
    IdentifierAst* nameIdent = extractNameFromSignature(node->signature);
    if (!nameIdent) return;
    
    KDevelop::StructureType::Ptr structType(new KDevelop::StructureType());
    
    KDevelop::QualifiedIdentifier qident(nameIdent->value);
    KDevelop::RangeInRevision range = editorFindRange(nameIdent, nameIdent);

    KDevelop::DUChainWriteLocker lock;
    KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
    decl->setKind(KDevelop::Declaration::Type);
    decl->setInSymbolTable(true);
    decl->setType(structType);
    DeclarationBuilderBase::closeDeclaration();

    ContextBuilder::visitStruct(node);

}

void DeclarationBuilder::visitAbstract(AbstractAst* node)
{
    if (!node || !node->signature) return;
    
    IdentifierAst* nameIdent = extractNameFromSignature(node->signature);
    if (!nameIdent) return;
    
    KDevelop::QualifiedIdentifier qident(nameIdent->value);
    KDevelop::RangeInRevision range = editorFindRange(nameIdent, nameIdent);
    
    KDevelop::DUChainWriteLocker lock;
    KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
    decl->setKind(KDevelop::Declaration::Type);
    decl->setInSymbolTable(true);
    DeclarationBuilderBase::closeDeclaration();
}

void DeclarationBuilder::visitPrimitive(PrimitiveAst* node)
{
    if (!node || !node->signature) return;
    
    IdentifierAst* nameIdent = extractNameFromSignature(node->signature);
    if (!nameIdent) return;
    
    KDevelop::QualifiedIdentifier qident(nameIdent->value);
    KDevelop::RangeInRevision range = editorFindRange(nameIdent, nameIdent);
    
    KDevelop::DUChainWriteLocker lock;
    KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
    decl->setKind(KDevelop::Declaration::Type);
    decl->setInSymbolTable(true);
    DeclarationBuilderBase::closeDeclaration();
}

void DeclarationBuilder::visitMacro(MacroAst* node)
{
    if (!node || !node->signature) return;
    
    IdentifierAst* nameIdent = extractNameFromSignature(node->signature);
    if (!nameIdent) return;
    
    KDevelop::QualifiedIdentifier qident(nameIdent->value);
    KDevelop::RangeInRevision range = editorFindRange(nameIdent, nameIdent);
    
    KDevelop::DUChainWriteLocker lock;
    KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
    decl->setKind(KDevelop::Declaration::Instance);
    decl->setInSymbolTable(true);
    DeclarationBuilderBase::closeDeclaration();

    ContextBuilder::visitMacro(node);
}

void DeclarationBuilder::visitConst(ConstAst* node)
{
    if (!node || !node->target) return;
    
    if (node->target->astType == AstType::IdentifierAstType) {
        IdentifierAst* ident = static_cast<IdentifierAst*>(node->target);
        if (ident) {
            KDevelop::QualifiedIdentifier qident(ident->value);
            KDevelop::RangeInRevision range = editorFindRange(ident, ident);
            
            KDevelop::DUChainWriteLocker lock;
            KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
            decl->setKind(KDevelop::Declaration::Instance);
            decl->setInSymbolTable(true);
            
            if (node->value) {
                lock.unlock();
                ExpressionVisitor exprVisitor(currentContext());
                exprVisitor.visitNode(node->value);
                KDevelop::AbstractType::Ptr type = exprVisitor.lastType();
                if (type) {
                    lock.lock();
                    decl->setType(type);
                }
                lock.unlock();
            } else {
                lock.unlock();
            }
            
            DeclarationBuilderBase::closeDeclaration();
        }
        
        // Visit RHS through declaration chain for nested declarations
        if (node->value) {
            visitNode(node->value);
        }
    }
}

void DeclarationBuilder::visitTypeAnnotation(TypeAnnotationAst* node)
{
    if (!node) return;
    if (!node->value || node->value->astType != AstType::IdentifierAstType) {
        JuliaAstDefaultVisitor::visitTypeAnnotation(node);
        return;
    }

    auto* ident = static_cast<IdentifierAst*>(node->value);
    KDevelop::QualifiedIdentifier qident(ident->value);
    KDevelop::RangeInRevision range = editorFindRange(ident, ident);

    KDevelop::DUChainWriteLocker lock;
    auto* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
    decl->setKind(KDevelop::Declaration::Instance);
    decl->setInSymbolTable(true);

    if (node->type) {
        lock.unlock();
        ExpressionVisitor exprVisitor(currentContext());
        exprVisitor.visitNode(node->type);
        KDevelop::AbstractType::Ptr type = exprVisitor.lastType();
        if (type) {
            lock.lock();
            decl->setType(type);
        }
        lock.unlock();
    } else {
        lock.unlock();
    }

    DeclarationBuilderBase::closeDeclaration();
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

}
