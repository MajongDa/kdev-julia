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
#include "expressionvisitor.h"
#include "juliaeditorintegrator.h"
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

DeclarationBuilder::DeclarationBuilder(JuliaEditorIntegrator* editor)
    : m_editor(editor)
{
}

DeclarationBuilder::~DeclarationBuilder() = default;

// ============================================================================
// Virtual dispatch methods for declaration creation (Python-style)
// ============================================================================

void DeclarationBuilder::visitNode(AstNode* node)
{
    qCDebug(KDEV_JULIA) << "DeclarationBuilder::visitNode:" << (node ? nodeKindToString(node->kind()) : QStringLiteral("null"));
    DeclarationBuilderBase::visitNode(node);
}

void DeclarationBuilder::visitFunction(FunctionNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitFunction";

    QString name = node->functionName();
    qCDebug(KDEV_JULIA) << "  Function name:" << name;
    
    if (!name.isEmpty()) {
        AstNode* nameNode = node->functionNameNode();
        
        auto* decl = openDeclaration<KDevelop::FunctionDeclaration>(nameNode ? nameNode : node, node);
        qCDebug(KDEV_JULIA) << "  openDeclaration returned:" << decl << "in context:" << currentContext();
        if (decl) {
            qCDebug(KDEV_JULIA) << "  Function declaration created:" << name << "in context:" << currentContext();
            // Use Instance for functions (not Type which is for classes/structs)
            decl->setKind(KDevelop::Declaration::Instance);
            decl->setInSymbolTable(false);
            
            // Create FunctionType
            KDevelop::FunctionType::Ptr funcType(new KDevelop::FunctionType());
            
            // Add parameter types
            QList<AstNode*> params = node->parameters();
            for (AstNode* param : params) {
                if (!param) continue;
                
                AstNode* paramTypeNode = param->lastChild();
                if (paramTypeNode) {
                    KDevelop::AbstractType* paramType = TypeMapper::typeFromAstNode(paramTypeNode);
                    if (paramType) {
                        funcType->addArgument(KDevelop::AbstractType::Ptr(paramType));
                    } else {
                        auto* mixedType = new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed);
                        funcType->addArgument(KDevelop::AbstractType::Ptr(mixedType));
                    }
                }
            }
            
            // Add return type
            if (node->hasReturnType()) {
                AstNode* retTypeNode = node->returnType();
                if (retTypeNode) {
                    KDevelop::AbstractType* retType = TypeMapper::typeFromAstNode(retTypeNode);
                    if (retType) {
                        funcType->setReturnType(KDevelop::AbstractType::Ptr(retType));
                    }
                }
            }
            
            decl->setType(funcType);
            
            // Save pointer before closing - we need it after closeDeclaration
            KDevelop::Declaration* funcDecl = decl;
            closeDeclaration();
            
            // Register in symbol table - this is critical for navigation to work!
            if (funcDecl) {
                funcDecl->setInSymbolTable(true);
            }
        }
    }
    
    // Create declarations for type parameters (from where clause)
    QList<AstNode*> typeParams = node->typeParameters();
    for (AstNode* typeParam : typeParams) {
        if (!typeParam || typeParam->kind() != NodeKind::Identifier) continue;
        
        QString typeParamName = typeParam->text();
        if (!typeParamName.isEmpty()) {
            auto* tpDecl = openDeclaration<KDevelop::Declaration>(typeParam, typeParam);
            if (tpDecl) {
                tpDecl->setKind(KDevelop::Declaration::Type);
                auto* structType = new KDevelop::StructureType();
                tpDecl->setType(KDevelop::AbstractType::Ptr(structType));
                closeDeclaration();
            }
        }
    }
    
    // Continue traversal - call inherited methods from ContextBuilder to create contexts
    // (NOT visitFunction which would cause infinite recursion)
    visitFunctionParameters(node, node);
    visitFunctionBody(node, node);
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitFunction DONE";
}

void DeclarationBuilder::visitStruct(StructNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitStruct";

    AstNode* nameNode = node->firstChild();
    
    // Handle parametric struct: struct Foo{T} ... 
    if (nameNode && nameNode->kind() == NodeKind::Curly) {
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
    
    // Continue traversal - call inherited method from ContextBuilder to create struct context
    visitStructBody(node);
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitStruct DONE";
}

void DeclarationBuilder::visitStructBody(StructNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitStructBody";
    
    AstNode* nameNode = node->firstChild();
    if (nameNode && nameNode->kind() == NodeKind::Curly) {
        if (CurlyNode* curly = dynamic_cast<CurlyNode*>(nameNode)) {
            nameNode = curly->firstChild();
        }
    }
    
    KDevelop::QualifiedIdentifier structId;
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        structId = KDevelop::QualifiedIdentifier(nameNode->text());
    }
    
    if (node->context) {
        openContext(node->context);
    } else {
        KDevelop::RangeInRevision range = node->range();
        openContext(node, range, KDevelop::DUContext::Class, structId);
    }
    
    QList<AstNode*> fields = node->fields();
    qCDebug(KDEV_JULIA) << "  Processing" << fields.size() << "fields";
    
    for (AstNode* field : fields) {
        if (!field || field->kind() != NodeKind::TypeAnnotation) {
            continue;
        }
        
        AstNode* idNode = field->firstChild();
        AstNode* typeNode = field->lastChild();
        
        if (!idNode || idNode->kind() != NodeKind::Identifier) {
            continue;
        }
        
        QString fieldName = idNode->text();
        qCDebug(KDEV_JULIA) << "    Field:" << fieldName;
        
        auto* decl = openDeclaration<KDevelop::Declaration>(idNode, node);
        if (decl) {
            decl->setKind(KDevelop::Declaration::Instance);
            decl->setInSymbolTable(true);
            
            if (typeNode) {
                ExpressionVisitor exprVisitor(currentContext());
                exprVisitor.visitNode(typeNode);
                if (exprVisitor.lastType()) {
                    decl->setType(exprVisitor.lastType());
                    qCDebug(KDEV_JULIA) << "      Type:" << exprVisitor.lastType()->toString();
                }
            }
            
            closeDeclaration();
        }
    }
    
    DeclarationBuilderBase::visitStructBody(node);
    
    closeContext();
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitStructBody DONE";
}

void DeclarationBuilder::visitModule(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitModule";

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
    
    // Continue traversal - call inherited method from ContextBuilder to create module context
    visitModuleBody(node);
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitModule DONE";
}

void DeclarationBuilder::visitAbstract(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitAbstract";

    AstNode* nameNode = node->firstChild();
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        QString name = nameNode->text();
        if (!name.isEmpty()) {
            auto* decl = openDeclaration<KDevelop::Declaration>(nameNode, node);
            if (decl) {
                decl->setKind(KDevelop::Declaration::Type);
                auto* structType = new KDevelop::StructureType();
                decl->setType(KDevelop::AbstractType::Ptr(structType));
                qCDebug(KDEV_JULIA) << "Abstract type declaration created:" << name;
                closeDeclaration();
            }
        }
    }
    
    // Continue traversal - call inherited method from ContextBuilder to create abstract context
    visitAbstractBody(node);
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitAbstract DONE";
}

void DeclarationBuilder::visitPrimitive(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitPrimitive";

    AstNode* nameNode = node->firstChild();
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        QString name = nameNode->text();
        if (!name.isEmpty()) {
            auto* decl = openDeclaration<KDevelop::Declaration>(nameNode, node);
            if (decl) {
                decl->setKind(KDevelop::Declaration::Type);
                auto* structType = new KDevelop::StructureType();
                decl->setType(KDevelop::AbstractType::Ptr(structType));
                qCDebug(KDEV_JULIA) << "Primitive type declaration created:" << name;
                closeDeclaration();
            }
        }
    }
    
    // Continue traversal - call inherited method from ContextBuilder to create primitive context
    visitPrimitiveBody(node);
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitPrimitive DONE";
}

void DeclarationBuilder::visitUsing(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitUsing";

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
    
    // Traversal is handled by ContextBuilder
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitUsing DONE";
}

void DeclarationBuilder::visitImport(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitImport";

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
    
    // Traversal is handled by ContextBuilder
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitImport DONE";
}

void DeclarationBuilder::visitExport(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitExport";

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
    
    // Traversal is handled by ContextBuilder
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitExport DONE";
}

void DeclarationBuilder::visitAssignment(AssignmentNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitAssignment";
    
    // First traverse children to build contexts (like JuliaAstDefaultVisitor does)
    JuliaAstDefaultVisitor::visitAssignment(node);
    
    // Get target and value
    AstNode* target = node->leftHandSide();
    AstNode* value = node->rightHandSide();
    
    if (!target || !value) {
        qCDebug(KDEV_JULIA) << "  No target or value, skipping";
        return;
    }
    
    // Handle type annotation: a::Bool = true
    // target is TypeAnnotation node, first child is identifier, last child is type
    AstNode* identifierNode = target;
    AstNode* explicitType = nullptr;
    
    if (target->kind() == NodeKind::TypeAnnotation) {
        identifierNode = target->firstChild();  // the 'a' part
        explicitType = target->lastChild();     // the 'Bool' part
        qCDebug(KDEV_JULIA) << "  Type annotation detected:" << identifierNode->text();
    }
    
    if (!identifierNode || identifierNode->kind() != NodeKind::Identifier) {
        qCDebug(KDEV_JULIA) << "  Target is not identifier, skipping";
        return;
    }
    
    // Get type - either from explicit annotation or from value inference
    KDevelop::AbstractType::Ptr declType;
    
    if (explicitType) {
        // Use explicit type from annotation
        ExpressionVisitor typeVisitor(currentContext());
        typeVisitor.visitNode(explicitType);
        declType = typeVisitor.lastType();
        qCDebug(KDEV_JULIA) << "  Using explicit type:" << (declType ? declType->toString() : QStringLiteral("unknown"));
    } else {
        // Infer from value
        ExpressionVisitor v(currentContext());
        v.visitNode(value);
        declType = v.lastType();
    }
    
    if (!declType) {
        qCDebug(KDEV_JULIA) << "  No type found, using mixed";
        auto* mixedType = new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed);
        declType = KDevelop::AbstractType::Ptr(mixedType);
    }
    
    // Create declaration for target
    KDevelop::Declaration* decl = openDeclaration<KDevelop::Declaration>(identifierNode, target);
    if (decl) {
        decl->setType(declType);
        qCDebug(KDEV_JULIA) << "  Assignment declaration created:" << identifierNode->text() << "type:" << declType->toString();
        KDevelop::Declaration* varDecl = decl;
        closeDeclaration();
        // Register in symbol table for navigation to work
        varDecl->setInSymbolTable(true);
    }
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitAssignment DONE";
}

// ============================================================================
// Required overrides
// ============================================================================

KDevelop::RangeInRevision DeclarationBuilder::editorFindRange(AstNode* fromNode, AstNode* toNode)
{
    if (!fromNode || !toNode) {
        return KDevelop::RangeInRevision(0, 0, 0, 0);
    }

    if (m_editor) {
        return m_editor->findRange(fromNode, toNode);
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

KDevelop::QualifiedIdentifier DeclarationBuilder::identifierForNode(AstNode* node)
{
    if (!node) {
        return KDevelop::QualifiedIdentifier();
    }
    if (node->kind() == NodeKind::Identifier) {
        return KDevelop::QualifiedIdentifier(node->text());
    }
    return KDevelop::QualifiedIdentifier();
}

void DeclarationBuilder::visitReturn(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitReturn";
    
    KDevelop::Declaration* funcDecl = currentDeclaration();
    if (!funcDecl || !funcDecl->isFunctionDeclaration()) {
        qCDebug(KDEV_JULIA) << "Return outside function, skipping";
        return;
    }
    
    AstNode* valueNode = nullptr;
    for (AstNode* child : node->children()) {
        if (child && child->kind() != NodeKind::Return) {
            valueNode = child;
            break;
        }
    }
    
    if (valueNode) {
        ExpressionVisitor exprVisitor(currentContext());
        exprVisitor.visitNode(valueNode);
        KDevelop::AbstractType::Ptr returnType = exprVisitor.lastType();
        
        if (returnType && funcDecl->abstractType()) {
            KDevelop::DUChainWriteLocker lock;
            if (auto* funcType = dynamic_cast<KDevelop::FunctionType*>(funcDecl->abstractType().data())) {
                funcType->setReturnType(returnType);
                qCDebug(KDEV_JULIA) << "Set return type:" << returnType->toString();
            }
        }
    }
    
    DeclarationBuilderBase::visitReturn(node);
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitReturn DONE";
}

void DeclarationBuilder::visitMacro(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitMacro";
    
    for (AstNode* child : node->children()) {
        if (!child) continue;
        
        if (child->kind() == NodeKind::Identifier) {
            QString macroName = child->text().trimmed();
            if (!macroName.isEmpty()) {
                auto* decl = openDeclaration<KDevelop::Declaration>(child, node);
                if (decl) {
                    decl->setKind(KDevelop::Declaration::Instance);
                    decl->setInSymbolTable(true);
                    qCDebug(KDEV_JULIA) << "Macro declaration created:" << macroName;
                    closeDeclaration();
                }
            }
        }
    }
    
    DeclarationBuilderBase::visitMacro(node);
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitMacro DONE";
}

void DeclarationBuilder::visitMacroCall(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitMacroCall";
    
    DeclarationBuilderBase::visitMacroCall(node);
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitMacroCall DONE";
}

void DeclarationBuilder::visitImportPath(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitImportPath";
    
    DeclarationBuilderBase::visitImportPath(node);
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitImportPath DONE";
}

}
