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
    if (!node || !node->name) return;
    
    qCDebug(KDEV_JULIA) << "DeclarationBuilder::visitFunctionDefinition:" << node->name->value;
    
    KDevelop::FunctionType::Ptr funcType(new KDevelop::FunctionType());
    
    KDevelop::QualifiedIdentifier ident(node->name->value);
    KDevelop::RangeInRevision range = editorFindRange(node->name, node->name);
    
    {
        KDevelop::DUChainWriteLocker lock;
        // Use FunctionDeclaration instead of generic Declaration
        KDevelop::FunctionDeclaration* funcDecl = DeclarationBuilderBase::openDeclaration<KDevelop::FunctionDeclaration>(ident, range);
        funcDecl->setType(funcType);
        funcDecl->setInSymbolTable(true);
    }
    
    openType(funcType);
    
    // Process return type annotation FIRST (so it's available for type inference)
    if (node->returns) {
        ExpressionVisitor exprVisitor(currentContext());
        exprVisitor.visitNode(node->returns);
        KDevelop::AbstractType::Ptr returnType = exprVisitor.lastType();
        if (returnType) {
            KDevelop::DUChainWriteLocker lock;
            funcType->setReturnType(returnType);
        }
    }
    
    // Visit arguments to create parameter declarations AND add to FunctionType
    if (node->arguments) {
        visitArguments(static_cast<ArgumentsAst*>(node->arguments));
    }
    
    closeType();
    DeclarationBuilderBase::closeDeclaration();
}

void DeclarationBuilder::visitAssignment(AssignmentAst* node)
{
    if (!node) return;
    
    for (auto* target : node->targets) {
        if (!target) continue;
        
        // Handle simple identifier assignment
        if (target->astType == AstType::IdentifierAstType) {
            IdentifierAst* nameAst = static_cast<IdentifierAst*>(target);
            if (!nameAst || nameAst->context != ExpressionAst::Context::Store) {
                continue;
            }
            
            IdentifierAst* ident = nameAst;
            KDevelop::QualifiedIdentifier qident(ident->value);
            KDevelop::RangeInRevision range = editorFindRange(ident, ident);
            
            KDevelop::DUChainWriteLocker lock;
            KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
            decl->setKind(KDevelop::Declaration::Instance);
            decl->setInSymbolTable(true);
            
            // If there's a value, try to infer type
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
        // Handle type annotation: x::T
        else if (target->astType == AstType::TypeAnnotationAstType) {
            TypeAnnotationAst* typeAnn = static_cast<TypeAnnotationAst*>(target);
            if (typeAnn->value && typeAnn->value->astType == AstType::IdentifierAstType) {
                IdentifierAst* ident = static_cast<IdentifierAst*>(typeAnn->value);
                if (ident) {
                    KDevelop::QualifiedIdentifier qident(ident->value);
                    KDevelop::RangeInRevision range = editorFindRange(ident, ident);
                    
                    KDevelop::DUChainWriteLocker lock;
                    KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
                    decl->setKind(KDevelop::Declaration::Instance);
                    decl->setInSymbolTable(true);
                    
                    if (typeAnn->type) {
                        lock.unlock();
                        ExpressionVisitor exprVisitor(currentContext());
                        exprVisitor.visitNode(typeAnn->type);
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
            }
        }
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
    if (!node || !node->target) return;
    
    // Handle loop variable declaration
    if (node->target->astType == AstType::IdentifierAstType) {
        IdentifierAst* ident = static_cast<IdentifierAst*>(node->target);
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

void DeclarationBuilder::visitWhile(WhileAst* node)
{
    AstVisitor::visitWhile(node);
}

void DeclarationBuilder::visitGlobal(GlobalAst* node)
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
}

void DeclarationBuilder::visitStruct(StructAst* node)
{
    if (!node || !node->name) return;
    
    KDevelop::StructureType::Ptr structType(new KDevelop::StructureType());
    
    KDevelop::QualifiedIdentifier qident(node->name->value);
    KDevelop::RangeInRevision range = editorFindRange(node->name, node->name);
    
    KDevelop::DUChainWriteLocker lock;
    KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
    decl->setKind(KDevelop::Declaration::Type);
    decl->setInSymbolTable(true);
    decl->setType(structType);
    DeclarationBuilderBase::closeDeclaration();
    
    // Handle field declarations in body
    for (auto* stmt : node->body) {
        if (!stmt) continue;
        if (stmt->astType == AstType::AssignmentAstType) {
            visitNode(stmt);
        }
    }
}

void DeclarationBuilder::visitAbstract(AbstractAst* node)
{
    if (!node || !node->name) return;
    
    KDevelop::QualifiedIdentifier qident(node->name->value);
    KDevelop::RangeInRevision range = editorFindRange(node->name, node->name);
    
    KDevelop::DUChainWriteLocker lock;
    KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
    decl->setKind(KDevelop::Declaration::Type);
    decl->setInSymbolTable(true);
    DeclarationBuilderBase::closeDeclaration();
}

void DeclarationBuilder::visitPrimitive(PrimitiveAst* node)
{
    if (!node || !node->name) return;
    
    KDevelop::QualifiedIdentifier qident(node->name->value);
    KDevelop::RangeInRevision range = editorFindRange(node->name, node->name);
    
    KDevelop::DUChainWriteLocker lock;
    KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
    decl->setKind(KDevelop::Declaration::Type);
    decl->setInSymbolTable(true);
    DeclarationBuilderBase::closeDeclaration();
}

void DeclarationBuilder::visitMacro(MacroAst* node)
{
    if (!node || !node->name) return;
    
    KDevelop::QualifiedIdentifier qident(node->name->value);
    KDevelop::RangeInRevision range = editorFindRange(node->name, node->name);
    
    KDevelop::DUChainWriteLocker lock;
    KDevelop::Declaration* decl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(qident, range);
    decl->setKind(KDevelop::Declaration::Instance);
    decl->setInSymbolTable(true);
    DeclarationBuilderBase::closeDeclaration();
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
    }
}

void DeclarationBuilder::visitArguments(ArgumentsAst* node)
{
    if (!node) return;
    
    qCDebug(KDEV_JULIA) << "DeclarationBuilder::visitArguments";
    
    // Get the FunctionType we're building
    KDevelop::FunctionType::Ptr funcType = currentType<KDevelop::FunctionType>();
    if (!funcType) {
        qCDebug(KDEV_JULIA) << "No FunctionType available for arguments";
    }
    
    // Default values are already in order matching positional arguments
    // No calculation needed - just iterate in natural order
    int defaultIndex = 0;
    
    // Process positional-only arguments
    for (auto* arg : node->posonlyargs) {
        if (arg && arg->astType == AstType::ArgAstType) {
            processArg(static_cast<ArgAst*>(arg), funcType, defaultIndex, node);
            defaultIndex++;
        }
    }
    
    // Process regular positional arguments
    for (auto* arg : node->arguments) {
        if (arg && arg->astType == AstType::ArgAstType) {
            processArg(static_cast<ArgAst*>(arg), funcType, defaultIndex, node);
            defaultIndex++;
        }
    }
    
    // Process keyword-only arguments (no default values in defaultValues list)
    for (auto* arg : node->kwonlyargs) {
        if (arg && arg->astType == AstType::ArgAstType) {
            processArg(static_cast<ArgAst*>(arg), funcType, -1, node);
        }
    }
    
    // Process *args (vararg)
    if (node->vararg && node->vararg->astType == AstType::ArgAstType) {
        KDevelop::IntegralType::Ptr varargType(new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed));
        
        if (funcType) {
            funcType->addArgument(KDevelop::AbstractType::Ptr(varargType));
        }
        
        processArg(static_cast<ArgAst*>(node->vararg), funcType, -1, node);
    }
    
    // Process **kwargs (kwarg)
    if (node->kwarg && node->kwarg->astType == AstType::ArgAstType) {
        KDevelop::IntegralType::Ptr kwargType(new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed));
        
        if (funcType) {
            funcType->addArgument(KDevelop::AbstractType::Ptr(kwargType));
        }
        
        processArg(static_cast<ArgAst*>(node->kwarg), funcType, -1, node);
    }
}

void DeclarationBuilder::processArg(ArgAst* node, KDevelop::FunctionType::Ptr funcType, 
                                   int defaultValueIndex, ArgumentsAst* parentArgs)
{
    if (!node || !node->argumentName) return;
    
    qCDebug(KDEV_JULIA) << "DeclarationBuilder::processArg:" << node->argumentName->value;
    
    KDevelop::QualifiedIdentifier ident(node->argumentName->value);
    KDevelop::RangeInRevision range = editorFindRange(node->argumentName, node->argumentName);
    
    KDevelop::AbstractType::Ptr argumentType;
    
    // Process type annotation if present
    if (node->annotation) {
        ExpressionVisitor exprVisitor(currentContext());
        exprVisitor.visitNode(node->annotation);
        argumentType = exprVisitor.lastType();
    }
    
    // If no annotation, try to get type from default value (direct index into defaultValues)
    if (!argumentType && defaultValueIndex >= 0 && defaultValueIndex < parentArgs->defaultValues.size()) {
        Ast* defaultValue = parentArgs->defaultValues.at(defaultValueIndex);
        if (defaultValue) {
            ExpressionVisitor exprVisitor(currentContext());
            exprVisitor.visitNode(defaultValue);
            argumentType = exprVisitor.lastType();
        }
    }
    
    // If still no type, use a generic type
    if (!argumentType) {
        argumentType = KDevelop::AbstractType::Ptr(new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed));
    }
    
    // Add argument type to FunctionType
    if (funcType) {
        funcType->addArgument(argumentType);
    }
    
    // Create the parameter declaration with the determined type
    {
        KDevelop::DUChainWriteLocker lock;
        KDevelop::Declaration* paramDecl = DeclarationBuilderBase::openDeclaration<KDevelop::Declaration>(ident, range);
        paramDecl->setKind(KDevelop::Declaration::Instance);
        paramDecl->setInSymbolTable(true);
        paramDecl->setType(argumentType);
        
        DeclarationBuilderBase::closeDeclaration();
    }
}

void DeclarationBuilder::visitCode(CodeAst* node)
{
    if (!node) return;
    
    // Visit all statements in the code block
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
}

}
