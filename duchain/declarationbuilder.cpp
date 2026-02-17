#include "declarationbuilder.h"

#include <QDebug>
#include <QProcess>

#include <language/duchain/duchain.h>
#include <language/duchain/declaration.h>
#include <language/duchain/functiondeclaration.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/problem.h>
#include <language/duchain/types/functiontype.h>
#include <language/duchain/types/structuretype.h>
#include <language/duchain/types/integraltype.h>
#include <language/editor/documentrange.h>
#include <interfaces/iproblem.h>

#include "../parser/ast.h"
#include "../types/types.h"
#include "juliadebug.h"
#include "kdevjuliaversion.h"

namespace Julia {

namespace {

void reportProblem(KDevelop::TopDUContext* topContext, const KDevelop::RangeInRevision& range, 
                   const QString& message, KDevelop::IProblem::Severity severity = KDevelop::IProblem::Warning)
{
    if (!topContext) return;
    
    KDevelop::Problem* p = new KDevelop::Problem();
    p->setFinalLocation(KDevelop::DocumentRange(topContext->url(), range.castToSimpleRange()));
    p->setSource(KDevelop::IProblem::SemanticAnalysis);
    p->setSeverity(severity);
    p->setDescription(message);
    
    KDevelop::DUChainWriteLocker lock(KDevelop::DUChain::lock());
    topContext->addProblem(KDevelop::ProblemPointer(p));
}

QString getJuliaExecutable()
{
    static QString juliaPath = QStringLiteral(JULIA_EXECUTABLE);
    return juliaPath;
}

QString findJuliaModule(const QString& moduleName)
{
    QString julia = getJuliaExecutable();
    if (julia.isEmpty()) {
        qCDebug(KDEV_JULIA) << "Julia executable not found";
        return QString();
    }
    
    QString script = QStringLiteral("try pkg = Base.find_package(%1); pkg === nothing ? println(\"NOT_FOUND\") : println(pkg); catch; println(\"NOT_FOUND\"); end")
                        .arg(QStringLiteral("\"%1\"").arg(moduleName));
    
    QProcess process;
    process.start(julia, {QStringLiteral("-e"), script});
    process.waitForFinished(3000);
    
    QString result = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    
    if (result.isEmpty() || result == QLatin1String("NOT_FOUND")) {
        return QString();
    }
    
    return result;
}

bool isBuiltInModule(const QString& name)
{
    static const QStringList builtIn = {
        QStringLiteral("Base"),
        QStringLiteral("Core"),
        QStringLiteral("Main"),
        QStringLiteral("Main.Include"),
        QStringLiteral("Base.Include")
    };
    return builtIn.contains(name);
}

}

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
            
            // Handle parametric struct: struct Foo{T} ... 
            // First child is Curly node, not Identifier
            if (nameNode && nameNode->kind() == NodeKind::Curly) {
                // Get the actual name from the curly node
                if (CurlyNode* curly = dynamic_cast<CurlyNode*>(nameNode)) {
                    nameNode = curly->firstChild();
                }
            }
            
            if (nameNode && nameNode->kind() == NodeKind::Identifier) {
                QString name = nameNode->text();
                if (!name.isEmpty()) {
                    auto* decl = openDeclaration<KDevelop::Declaration>(nameNode, node);
                    if (decl) {
                        decl->setKind(KDevelop::Declaration::Type);
                        auto* structType = new KDevelop::StructureType();
                        structType->setDeclaration(decl);
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
                        structType->setDeclaration(decl);
                        decl->setType(KDevelop::AbstractType::Ptr(structType));
                        qCDebug(KDEV_JULIA) << "Module declaration created:" << name;
                        closeDeclaration();
                    }
                }
            }
            
            DeclarationBuilderBase::startVisiting(node);
            break;
        }
        case NodeKind::Equals: {
            AstNode* lhs = node->firstChild();
            
            if (!lhs) {
                DeclarationBuilderBase::startVisiting(node);
                break;
            }
            
            QString varName;
            QString typeName;
            AstNode* varNode = nullptr;
            
            if (lhs->kind() == NodeKind::Identifier) {
                varName = lhs->text();
                varNode = lhs;
            } else if (lhs->kind() == NodeKind::TypeAnnotation) {
                AstNode* nameNode = lhs->firstChild();
                AstNode* typeNode = lhs->lastChild();
                if (nameNode && nameNode->kind() == NodeKind::Identifier) {
                    varName = nameNode->text();
                    varNode = nameNode;
                }
                if (typeNode) {
                    typeName = typeNode->text();
                }
            }
            
            if (!varName.isEmpty() && varNode) {
                auto* decl = openDeclaration<KDevelop::Declaration>(varNode, varNode);
                if (decl) {
                    decl->setKind(KDevelop::Declaration::Instance);
                    
                    KDevelop::AbstractType::Ptr typePtr;
                    
                    if (!typeName.isEmpty()) {
                        typePtr = KDevelop::AbstractType::Ptr(TypeMapper::typeFromString(typeName, currentContext()));
                    }
                    
                    if (!typePtr) {
                        auto* intType = new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed);
                        typePtr = KDevelop::AbstractType::Ptr(intType);
                    }
                    
                    decl->setType(typePtr);
                    qCDebug(KDEV_JULIA) << "Variable declaration created:" << varName << "type:" << typeName << "range:" << decl->range();
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
                        if (!isBuiltInModule(moduleName)) {
                            QString modulePath = findJuliaModule(moduleName);
                            if (modulePath.isEmpty()) {
                                reportProblem(topContext(), child->range(), 
                                    QStringLiteral("Module \"%1\" not found").arg(moduleName),
                                    KDevelop::IProblem::Warning);
                            } else {
                                qCDebug(KDEV_JULIA) << "Found Julia module:" << moduleName << "at" << modulePath;
                            }
                        }
                        
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
                        QStringList parts = modulePath.split(QLatin1Char('.'));
                        if (!parts.isEmpty() && !isBuiltInModule(parts.first())) {
                            QString modulePathResolved = findJuliaModule(parts.first());
                            if (modulePathResolved.isEmpty()) {
                                reportProblem(topContext(), child->range(), 
                                    QStringLiteral("Module \"%1\" not found").arg(parts.first()),
                                    KDevelop::IProblem::Warning);
                            } else {
                                qCDebug(KDEV_JULIA) << "Found Julia module:" << parts.first() << "at" << modulePathResolved;
                            }
                        }
                        
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
                                if (!isBuiltInModule(moduleName)) {
                                    QString modulePath = findJuliaModule(moduleName);
                                    if (modulePath.isEmpty()) {
                                        reportProblem(topContext(), subchild->range(), 
                                            QStringLiteral("Module \"%1\" not found").arg(moduleName),
                                            KDevelop::IProblem::Warning);
                                    } else {
                                        qCDebug(KDEV_JULIA) << "Found Julia module:" << moduleName << "at" << modulePath;
                                    }
                                }
                                
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
                        if (!isBuiltInModule(name)) {
                            QString modulePath = findJuliaModule(name);
                            if (modulePath.isEmpty()) {
                                reportProblem(topContext(), child->range(), 
                                    QStringLiteral("Module \"%1\" not found").arg(name),
                                    KDevelop::IProblem::Warning);
                            } else {
                                qCDebug(KDEV_JULIA) << "Found Julia module:" << name << "at" << modulePath;
                            }
                        }
                        
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
                        QStringList parts = path.split(QLatin1Char('.'));
                        if (!parts.isEmpty() && !isBuiltInModule(parts.first())) {
                            QString modulePathResolved = findJuliaModule(parts.first());
                            if (modulePathResolved.isEmpty()) {
                                reportProblem(topContext(), child->range(), 
                                    QStringLiteral("Module \"%1\" not found").arg(parts.first()),
                                    KDevelop::IProblem::Warning);
                            } else {
                                qCDebug(KDEV_JULIA) << "Found Julia module:" << parts.first() << "at" << modulePathResolved;
                            }
                        }
                        
                        auto* decl = openDeclaration<KDevelop::Declaration>(child, node);
                        if (decl) {
                            decl->setKind(KDevelop::Declaration::Namespace);
                            auto* structType = new KDevelop::StructureType();
                            decl->setType(KDevelop::AbstractType::Ptr(structType));
                            qCDebug(KDEV_JULIA) << "Import path declaration created:" << path;
                            closeDeclaration();
                        }
                    }
                } else if (child->kind() == NodeKind::Colon) {
                    for (AstNode* importPath : child->children()) {
                        if (!importPath) continue;
                        QString pathText = importPath->text().trimmed();
                        if (pathText.isEmpty()) continue;
                        
                        auto* decl = openDeclaration<KDevelop::Declaration>(importPath, node);
                        if (decl) {
                            decl->setKind(KDevelop::Declaration::Instance);
                            auto* structType = new KDevelop::StructureType();
                            decl->setType(KDevelop::AbstractType::Ptr(structType));
                            qCDebug(KDEV_JULIA) << "Import from declaration created:" << pathText;
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
