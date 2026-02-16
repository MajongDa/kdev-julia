#include "declarationbuilder.h"

#include <QDebug>

#include <language/duchain/duchain.h>
#include <language/duchain/declaration.h>
#include <language/duchain/functiondeclaration.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/types/functiontype.h>
#include <language/duchain/types/structuretype.h>
#include <language/duchain/types/integraltype.h>

#include "../parser/ast.h"
#include "../types/types.h"
#include "juliadebug.h"

namespace Julia {

DeclarationBuilder::DeclarationBuilder() = default;

DeclarationBuilder::~DeclarationBuilder() = default;

void DeclarationBuilder::startVisiting(AstNode* node)
{
    if (!node) {
        return;
    }

    switch (node->kind()) {
        case NodeKind::TopLevel:
        case NodeKind::Block: {
            DeclarationBuilderBase::startVisiting(node);
            break;
        }
        case NodeKind::Function: {
            qCDebug(KDEV_JULIA) << "Visiting function, creating declaration";
            
            AstNode* nameNode = node->firstChild();
            if (nameNode && nameNode->kind() == NodeKind::Call) {
                nameNode = nameNode->firstChild();
            }
            
            if (nameNode && nameNode->kind() == NodeKind::Identifier) {
                QString name = nameNode->text();
                if (!name.isEmpty()) {
                    auto* decl = openDeclaration<KDevelop::Declaration>(nameNode, node);
                    if (decl) {
                        decl->setKind(KDevelop::Declaration::Type);
                        auto* funcType = new KDevelop::FunctionType();
                        decl->setType(KDevelop::AbstractType::Ptr(funcType));
                        qCDebug(KDEV_JULIA) << "Function declaration created:" << name;
                        closeDeclaration();
                    }
                }
            }
            
            DeclarationBuilderBase::startVisiting(node);
            break;
        }
        case NodeKind::Struct: {
            qCDebug(KDEV_JULIA) << "Visiting struct, creating declaration";
            
            AstNode* nameNode = node->firstChild();
            if (nameNode && nameNode->kind() == NodeKind::Identifier) {
                QString name = nameNode->text();
                if (!name.isEmpty()) {
                    auto* decl = openDeclaration<KDevelop::Declaration>(nameNode, node);
                    if (decl) {
                        decl->setKind(KDevelop::Declaration::Type);
                        auto* structType = new KDevelop::StructureType();
                        decl->setType(KDevelop::AbstractType::Ptr(structType));
                        qCDebug(KDEV_JULIA) << "Struct declaration created:" << name;
                        closeDeclaration();
                    }
                }
            }
            
            DeclarationBuilderBase::startVisiting(node);
            break;
        }
        case NodeKind::Module: {
            qCDebug(KDEV_JULIA) << "Visiting module";
            
            AstNode* nameNode = node->firstChild();
            if (nameNode && nameNode->kind() == NodeKind::Identifier) {
                QString name = nameNode->text();
                if (!name.isEmpty()) {
                    auto* decl = openDeclaration<KDevelop::Declaration>(nameNode, node);
                    if (decl) {
                        decl->setKind(KDevelop::Declaration::Type);
                        auto* structType = new KDevelop::StructureType();
                        decl->setType(KDevelop::AbstractType::Ptr(structType));
                        qCDebug(KDEV_JULIA) << "Module declaration created:" << name;
                        closeDeclaration();
                    }
                }
            }
            
            DeclarationBuilderBase::startVisiting(node);
            break;
        }
        case NodeKind::Assignment: {
            AstNode* lhs = node->firstChild();
            
            QString varName;
            QString typeName;
            
            if (lhs && lhs->kind() == NodeKind::Identifier) {
                varName = lhs->text();
            } else if (lhs && lhs->kind() == NodeKind::TypeAnnotation) {
                AstNode* nameNode = lhs->firstChild();
                AstNode* typeNode = lhs->lastChild();
                if (nameNode && nameNode->kind() == NodeKind::Identifier) {
                    varName = nameNode->text();
                }
                if (typeNode) {
                    typeName = typeNode->text();
                }
            }
            
            if (!varName.isEmpty()) {
                auto* decl = openDeclaration<KDevelop::Declaration>(lhs, lhs);
                if (decl) {
                    decl->setKind(KDevelop::Declaration::Instance);
                    
                    KDevelop::AbstractType::Ptr typePtr;
                    
                    if (!typeName.isEmpty()) {
                        typePtr = KDevelop::AbstractType::Ptr(TypeMapper::typeFromString(typeName));
                    }
                    
                    if (!typePtr) {
                        auto* intType = new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed);
                        typePtr = KDevelop::AbstractType::Ptr(intType);
                    }
                    
                    decl->setType(typePtr);
                    qCDebug(KDEV_JULIA) << "Variable declaration created:" << varName << "type:" << typeName;
                    closeDeclaration();
                }
            }
            
            DeclarationBuilderBase::startVisiting(node);
            break;
        }
        case NodeKind::Using: {
            qCDebug(KDEV_JULIA) << "Visiting using statement";
            
            for (AstNode* child : node->children()) {
                if (!child) continue;
                
                if (child->kind() == NodeKind::Identifier) {
                    QString moduleName = child->text().trimmed();
                    if (!moduleName.isEmpty()) {
                        auto* decl = openDeclaration<KDevelop::Declaration>(child, node);
                        if (decl) {
                            decl->setKind(KDevelop::Declaration::Namespace);
                            auto* structType = new KDevelop::StructureType();
                            decl->setType(KDevelop::AbstractType::Ptr(structType));
                            qCDebug(KDEV_JULIA) << "Using declaration created:" << moduleName;
                            closeDeclaration();
                        }
                    }
                } else if (child->kind() == NodeKind::Dot) {
                    QString modulePath = child->text().trimmed();
                    if (!modulePath.isEmpty()) {
                        auto* decl = openDeclaration<KDevelop::Declaration>(child, node);
                        if (decl) {
                            decl->setKind(KDevelop::Declaration::Namespace);
                            auto* structType = new KDevelop::StructureType();
                            decl->setType(KDevelop::AbstractType::Ptr(structType));
                            qCDebug(KDEV_JULIA) << "Using path declaration created:" << modulePath;
                            closeDeclaration();
                        }
                    }
                } else {
                    for (AstNode* subchild : child->children()) {
                        if (subchild && subchild->kind() == NodeKind::Identifier) {
                            QString moduleName = subchild->text().trimmed();
                            if (!moduleName.isEmpty()) {
                                auto* decl = openDeclaration<KDevelop::Declaration>(subchild, node);
                                if (decl) {
                                    decl->setKind(KDevelop::Declaration::Namespace);
                                    auto* structType = new KDevelop::StructureType();
                                    decl->setType(KDevelop::AbstractType::Ptr(structType));
                                    qCDebug(KDEV_JULIA) << "Using (nested) declaration created:" << moduleName;
                                    closeDeclaration();
                                }
                            }
                        }
                    }
                }
            }
            
            DeclarationBuilderBase::startVisiting(node);
            break;
        }
        case NodeKind::Import: {
            qCDebug(KDEV_JULIA) << "Visiting import statement";
            
            for (AstNode* child : node->children()) {
                if (!child) continue;
                
                if (child->kind() == NodeKind::Identifier) {
                    QString name = child->text().trimmed();
                    if (!name.isEmpty()) {
                        auto* decl = openDeclaration<KDevelop::Declaration>(child, node);
                        if (decl) {
                            decl->setKind(KDevelop::Declaration::Instance);
                            auto* structType = new KDevelop::StructureType();
                            decl->setType(KDevelop::AbstractType::Ptr(structType));
                            qCDebug(KDEV_JULIA) << "Import declaration created:" << name;
                            closeDeclaration();
                        }
                    }
                } else if (child->kind() == NodeKind::Dot) {
                    QString path = child->text().trimmed();
                    if (!path.isEmpty()) {
                        auto* decl = openDeclaration<KDevelop::Declaration>(child, node);
                        if (decl) {
                            decl->setKind(KDevelop::Declaration::Namespace);
                            auto* structType = new KDevelop::StructureType();
                            decl->setType(KDevelop::AbstractType::Ptr(structType));
                            qCDebug(KDEV_JULIA) << "Import path declaration created:" << path;
                            closeDeclaration();
                        }
                    }
                }
            }
            
            DeclarationBuilderBase::startVisiting(node);
            break;
        }
        case NodeKind::Export: {
            qCDebug(KDEV_JULIA) << "Visiting export statement";
            
            for (AstNode* child : node->children()) {
                if (!child || child->kind() != NodeKind::Identifier) continue;
                
                QString name = child->text().trimmed();
                if (!name.isEmpty()) {
                    auto* decl = openDeclaration<KDevelop::Declaration>(child, node);
                    if (decl) {
                        decl->setKind(KDevelop::Declaration::Type);
                        auto* structType = new KDevelop::StructureType();
                        decl->setType(KDevelop::AbstractType::Ptr(structType));
                        qCDebug(KDEV_JULIA) << "Export declaration created:" << name;
                        closeDeclaration();
                    }
                }
            }
            
            DeclarationBuilderBase::startVisiting(node);
            break;
        }
        default:
            DeclarationBuilderBase::startVisiting(node);
            break;
    }
}

KDevelop::RangeInRevision DeclarationBuilder::editorFindRange(AstNode* fromNode, AstNode* toNode)
{
    if (!fromNode || !toNode) {
        return KDevelop::RangeInRevision(0, 0, 0, 0);
    }

    return fromNode->range();
}

void DeclarationBuilder::setContextOnNode(AstNode* node, KDevelop::DUContext* context)
{
    if (node) {
        node->context = context;
    }
}

KDevelop::DUContext* DeclarationBuilder::contextFromNode(AstNode* node)
{
    if (!node) {
        return nullptr;
    }
    return node->context;
}

}
