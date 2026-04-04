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
    
    KDevelop::Declaration* funcDecl = nullptr;
    KDevelop::FunctionType::Ptr funcType;
    
    if (!name.isEmpty()) {
        AstNode* nameNode = node->functionNameNode();
        
        auto* decl = openDeclaration<KDevelop::FunctionDeclaration>(nameNode ? nameNode : node, node);
        qCDebug(KDEV_JULIA) << "  openDeclaration returned:" << decl << "in context:" << currentContext();
        if (decl) {
            qCDebug(KDEV_JULIA) << "  Function declaration created:" << name << "in context:" << currentContext();
            decl->setKind(KDevelop::Declaration::Instance);
            decl->setInSymbolTable(false);
            
            // Create FunctionType - parameter types will be collected during traversal
            funcType = KDevelop::FunctionType::Ptr(new KDevelop::FunctionType());
            
            // Add placeholder arguments (will be updated after traversal)
            QList<AstNode*> params = node->parameters();
            for (int i = 0; i < params.size(); ++i) {
                auto* mixedType = new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed);
                funcType->addArgument(KDevelop::AbstractType::Ptr(mixedType));
            }
            
            // Add return type using ExpressionVisitor (like Python)
            if (node->hasReturnType()) {
                AstNode* retTypeNode = node->returnType();
                if (retTypeNode) {
                    ExpressionVisitor typeVisitor(currentContext());
                    typeVisitor.visitNode(retTypeNode);
                    if (typeVisitor.lastType()) {
                        funcType->setReturnType(typeVisitor.lastType());
                        qCDebug(KDEV_JULIA) << "  Return type from annotation:" << typeVisitor.lastType()->toString();
                    }
                }
            }
            
            decl->setType(funcType);
            funcDecl = decl;
            
            // Push FunctionType onto stack so return statements can find it via currentType<FunctionType>()
            // This must be done BEFORE traversal, like Python does
            openType(KDevelop::AbstractType::Ptr(funcType.data()));
            
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
    
    // Continue traversal - inherited methods from ContextBuilder handle context creation
    // Parameter declarations are created in visitParameters during traversal
    visitFunctionParameters(node);
    visitFunctionBody(node);
    
    // Close declaration AFTER traversal (like Python does)
    closeDeclaration();
    
    // Pop FunctionType from stack
    closeType();
    
    // Update declaration's type AFTER traversal to capture any modifications (like return type)
    // This is exactly what Python does - call setType again after traversal
    if (funcDecl && funcType) {
        funcDecl->setType(KDevelop::AbstractType::Ptr(funcType.data()));
    }
    
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
    
    // Check if declaration already exists in current context (prevent duplicates)
    // Similar to Python's eventuallyReopenDeclaration
    QString varName = identifierNode->text();
    KDevelop::QualifiedIdentifier id(varName);
    KDevelop::CursorInRevision searchPos = identifierNode->range().start;
    QList<KDevelop::Declaration*> existing = currentContext()->findDeclarations(id, searchPos);
    
    KDevelop::Declaration* decl = nullptr;
    if (!existing.isEmpty()) {
        // Reuse existing declaration - keep original range, just update type
        // DO NOT call openDeclaration here - it would create a duplicate!
        // DO NOT overwrite range - keep the original declaration position
        qCDebug(KDEV_JULIA) << "  Reusing existing declaration:" << varName;
        decl = existing.first();
        decl->setType(declType);
        decl->setInSymbolTable(true);
    } else {
        // Create new declaration
        qCDebug(KDEV_JULIA) << "  Creating new declaration:" << varName;
        decl = openDeclaration<KDevelop::Declaration>(identifierNode, target);
        if (decl) {
            decl->setType(declType);
            closeDeclaration();
            decl->setInSymbolTable(true);
        }
    }
    
    if (decl) {
        qCDebug(KDEV_JULIA) << "  Assignment declaration:" << varName << "type:" << declType->toString();
    }
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitAssignment DONE";
}

void DeclarationBuilder::visitFor(ForNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitFor";
    
    JuliaAstDefaultVisitor::visitFor(node);
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitFor DONE";
}

void DeclarationBuilder::visitWhile(WhileNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitWhile";
    
    JuliaAstDefaultVisitor::visitWhile(node);
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitWhile DONE";
}

void DeclarationBuilder::visitGlobal(GlobalNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitGlobal";
    
    JuliaAstDefaultVisitor::visitGlobal(node);
    
    // Global declares variables in the global scope
    for (AstNode* ident : node->identifiers()) {
        if (ident && ident->kind() == NodeKind::Identifier) {
            QString name = ident->text();
            qCDebug(KDEV_JULIA) << "  Global variable:" << name;
            
            auto* decl = openDeclaration<KDevelop::Declaration>(ident, node);
            if (decl) {
                decl->setKind(KDevelop::Declaration::Instance);
                auto* mixedType = new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed);
                decl->setType(KDevelop::AbstractType::Ptr(mixedType));
                closeDeclaration();
                decl->setInSymbolTable(true);
            }
        }
    }
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitGlobal DONE";
}

void DeclarationBuilder::visitLocal(LocalNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitLocal";
    
    JuliaAstDefaultVisitor::visitLocal(node);
    
    // Local declares variables in the current scope
    for (AstNode* ident : node->identifiers()) {
        if (ident && ident->kind() == NodeKind::Identifier) {
            QString name = ident->text();
            qCDebug(KDEV_JULIA) << "  Local variable:" << name;
            
            auto* decl = openDeclaration<KDevelop::Declaration>(ident, node);
            if (decl) {
                decl->setKind(KDevelop::Declaration::Instance);
                auto* mixedType = new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed);
                decl->setType(KDevelop::AbstractType::Ptr(mixedType));
                closeDeclaration();
                decl->setInSymbolTable(true);
            }
        }
    }
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitLocal DONE";
}

void DeclarationBuilder::visitConst(ConstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitConst";
    
    JuliaAstDefaultVisitor::visitConst(node);
    
    AstNode* target = node->target();
    if (!target) {
        return;
    }
    
    // Handle type annotation: const a::Bool = 1
    AstNode* identifierNode = target;
    AstNode* explicitType = nullptr;
    
    if (target->kind() == NodeKind::TypeAnnotation) {
        identifierNode = target->firstChild();
        explicitType = target->lastChild();
    }
    
    if (!identifierNode || identifierNode->kind() != NodeKind::Identifier) {
        return;
    }
    
    QString name = identifierNode->text();
    qCDebug(KDEV_JULIA) << "  Const variable:" << name;
    
    KDevelop::AbstractType::Ptr declType;
    
    if (explicitType) {
        ExpressionVisitor typeVisitor(currentContext());
        typeVisitor.visitNode(explicitType);
        declType = typeVisitor.lastType();
    } else {
        AstNode* value = node->value();
        if (value) {
            ExpressionVisitor v(currentContext());
            v.visitNode(value);
            declType = v.lastType();
        }
    }
    
    if (!declType) {
        auto* mixedType = new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed);
        declType = KDevelop::AbstractType::Ptr(mixedType);
    }
    
    auto* decl = openDeclaration<KDevelop::Declaration>(identifierNode, node);
    if (decl) {
        decl->setKind(KDevelop::Declaration::Instance);
        decl->setType(declType);
        qCDebug(KDEV_JULIA) << "  Const declaration created:" << name << "type:" << declType->toString();
        closeDeclaration();
        decl->setInSymbolTable(true);
    }
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitConst DONE";
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

void DeclarationBuilder::visitReturn(ReturnNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitReturn";
    
    // Use currentType<FunctionType>() like Python does - finds function from context stack
    KDevelop::FunctionType::Ptr funcType = currentType<KDevelop::FunctionType>();
    if (funcType) {
        qCDebug(KDEV_JULIA) << "  currentType<FunctionType>() returned:" << funcType->toString();
    } else {
        qCDebug(KDEV_JULIA) << "  currentType<FunctionType>() returned: nullptr";
    }
    
    if (!funcType) {
        qCDebug(KDEV_JULIA) << "Return outside function, skipping";
        DeclarationBuilderBase::visitReturn(node);
        return;
    }
    
    AstNode* valueNode = node->value();
    
    if (valueNode) {
        ExpressionVisitor exprVisitor(currentContext());
        exprVisitor.visitNode(valueNode);
        KDevelop::AbstractType::Ptr returnType = exprVisitor.lastType();
        
        if (returnType && funcType) {
            KDevelop::DUChainWriteLocker lock;
            funcType->setReturnType(returnType);
            qCDebug(KDEV_JULIA) << "Set return type:" << returnType->toString();
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

// visitFunctionSignature creates declarations for function parameters from the Call node
// The Call node's first child is the function name, remaining children are parameters
void DeclarationBuilder::visitFunctionSignature(CallNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> DeclarationBuilder::visitFunctionSignature";
    
    // Create declarations for each parameter
    // At this point, currentContext() is the function's parameter context
    // First child is function name, rest are parameters
    const auto& children = node->children();
    for (int i = 1; i < children.size(); ++i) {
        AstNode* param = children.at(i);
        if (!param) continue;
        
        // Handle parameter: either simple identifier or TypeAnnotation (x::Int)
        AstNode* idNode = param;
        
        if (param->kind() == NodeKind::TypeAnnotation) {
            idNode = param->firstChild();
        }
        
        if (!idNode || idNode->kind() != NodeKind::Identifier) {
            continue;
        }
        
        QString name = idNode->text();
        qCDebug(KDEV_JULIA) << "  Parameter:" << name;
        
        // Get type - from annotation or default value or mixed
        KDevelop::AbstractType::Ptr paramType;
        
        if (param->kind() == NodeKind::TypeAnnotation) {
            AstNode* typeNode = param->lastChild();
            if (typeNode) {
                ExpressionVisitor v(currentContext());
                v.visitNode(typeNode);
                paramType = v.lastType();
            }
        }
        
        if (!paramType) {
            paramType = KDevelop::AbstractType::Ptr(
                new KDevelop::IntegralType(KDevelop::IntegralType::TypeMixed));
        }
        
        // Create declaration in current (function parameter) context
        auto* decl = openDeclaration<KDevelop::Declaration>(idNode, param);
        if (decl) {
            decl->setKind(KDevelop::Declaration::Instance);
            decl->setType(paramType);
            closeDeclaration();
            decl->setInSymbolTable(true);
        }
    }
    
    qCDebug(KDEV_JULIA) << "<<< DeclarationBuilder::visitFunctionSignature DONE";
}

}
