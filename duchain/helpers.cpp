#include "helpers.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>

#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/types/unsuretype.h>
#include <language/duchain/types/integraltype.h>
#include <language/duchain/types/containertypes.h>
#include <language/duchain/types/arraytype.h>
#include <language/duchain/types/functiontype.h>
#include <language/duchain/types/typealiastype.h>
#include <language/duchain/types/indexedtype.h>
#include <language/duchain/classdeclaration.h>
#include <language/duchain/functiondeclaration.h>
#include <language/duchain/aliasdeclaration.h>

#include "parser/ast.h"
#include "juliadebug.h"

using namespace KDevelop;

namespace Julia {

// Cache for exported symbols and resolved types
static QCache<QString, QStringList> s_exportCache(100);
static QCache<QString, AbstractType::Ptr> s_symbolCache(100);
static QList<ImportInfo> s_pendingImports;
static QMutex s_pendingImportsMutex;

QStringList Helper::getJuliaLoadPath()
{
    QStringList paths;
    
    // Default Julia load paths
    static const char* defaultPaths[] = {
        ".",
        "./src",
        "~/julia/usr/lib/julia",
        "/usr/share/julia/lib",
        "/usr/local/share/julia/lib"
    };
    
    for (const char* p : defaultPaths) {
        QString path = QDir::cleanPath(QDir::homePath() + QLatin1String(p));
        if (QFileInfo::exists(path)) {
            paths.append(path);
        }
        path = QLatin1String(p);
        if (QFileInfo::exists(path)) {
            paths.append(path);
        }
    }
    
    // Try to get from Julia's LOAD_PATH
    QProcess process;
    process.start(QStringLiteral("julia"), {QStringLiteral("-e"), QStringLiteral("println(join(LOAD_PATH, ','))")});
    if (process.waitForFinished(3000)) {
        QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        if (!output.isEmpty()) {
            for (const QString& p : output.split(QLatin1Char(','))) {
                QString path = p.trimmed();
                if (QFileInfo::exists(path)) {
                    paths.append(path);
                }
            }
        }
    }
    
    return paths;
}

QStringList Helper::findJuliaModuleFiles(const QString& moduleName)
{
    QStringList result;
    
    QStringList loadPaths = getJuliaLoadPath();
    
    // Julia module naming conventions
    // - ModuleName.jl -> file ModuleName.jl in src/
    // - ModuleName/ModuleName.jl -> file ModuleName.jl in directory ModuleName/
    
    QStringList candidates;
    candidates << QStringLiteral("src/%1.jl").arg(moduleName);
    candidates << QStringLiteral("%1.jl").arg(moduleName);
    candidates << QStringLiteral("%1/%1.jl").arg(moduleName);
    
    for (const QString& basePath : loadPaths) {
        for (const QString& candidate : candidates) {
            QString fullPath = basePath + QLatin1String("/") + candidate;
            fullPath = QDir::cleanPath(fullPath);
            if (QFileInfo::exists(fullPath)) {
                result.append(fullPath);
            }
        }
    }
    
    return result;
}

QStringList Helper::getExportedSymbols(const QString& filePath)
{
    // Check cache first
    if (s_exportCache.contains(filePath)) {
        return *s_exportCache.object(filePath);
    }
    
    QStringList exports;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return exports;
    }
    
    QTextStream in(&file);
    QRegularExpression exportRegex(QStringLiteral("^\\s*export\\s+([\\w,]+)"));
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        
        QRegularExpressionMatch match = exportRegex.match(line);
        if (match.hasMatch()) {
            QStringList names = match.captured(1).split(QLatin1Char(','), Qt::SkipEmptyParts);
            for (const QString& name : names) {
                exports.append(name.trimmed());
            }
        }
    }
    
    file.close();
    
    // Cache the result
    s_exportCache.insert(filePath, new QStringList(exports));
    
    return exports;
}

AbstractType::Ptr Helper::resolveImportedSymbol(const QString& filePath,
                                               const QString& symbolName,
                                               const TopDUContext* topContext)
{
    // Check cache first
    QString cacheKey = filePath + QLatin1String(":") + symbolName;
    if (s_symbolCache.contains(cacheKey)) {
        return *s_symbolCache.object(cacheKey);
    }
    
    // TODO: Parse the .jl file to find symbol definition
    // For now, return mixed type - will be enhanced later
    
    AbstractType::Ptr result(new IntegralType(IntegralType::TypeMixed));
    
    // Cache the result
    s_symbolCache.insert(cacheKey, new AbstractType::Ptr(result));
    
    return result;
}

void Helper::addPendingImport(const ImportInfo& info)
{
    QMutexLocker locker(&s_pendingImportsMutex);
    s_pendingImports.append(info);
}

QList<ImportInfo> Helper::pendingImports()
{
    QMutexLocker locker(&s_pendingImportsMutex);
    return s_pendingImports;
}

void Helper::clearPendingImports()
{
    QMutexLocker locker(&s_pendingImportsMutex);
    s_pendingImports.clear();
}

Declaration* Helper::accessAttribute(const AbstractType::Ptr accessed,
                                    const QString& attribute,
                                    const TopDUContext* topContext)
{
    if (!accessed || !topContext) {
        return nullptr;
    }

    DUChainReadLocker lock(DUChain::lock());

    StructureType::Ptr structType = accessed.dynamicCast<StructureType>();
    if (structType) {
        Declaration* decl = structType->declaration(topContext);
        if (decl) {
            DUContext* classContext = decl->internalContext();
            if (classContext) {
                QualifiedIdentifier attrId(attribute);
                QList<Declaration*> decls = classContext->findDeclarations(attrId);
                if (!decls.isEmpty()) {
                    return decls.first();
                }
            }
        }
    }

    UnsureType::Ptr unsure = accessed.dynamicCast<UnsureType>();
    if (unsure) {
        for (uint i = 0; i < unsure->typesSize(); i++) {
            AbstractType::Ptr type = unsure->types()[i].abstractType();
            Declaration* decl = accessAttribute(type, attribute, topContext);
            if (decl) {
                return decl;
            }
        }
    }

    return nullptr;
}

AbstractType::Ptr Helper::resolveAliasType(const AbstractType::Ptr eventualAlias)
{
    if (!eventualAlias) {
        return eventualAlias;
    }

    TypeAliasType::Ptr aliasType = eventualAlias.dynamicCast<TypeAliasType>();
    if (aliasType) {
        return aliasType->type();
    }

    return eventualAlias;
}

AbstractType::Ptr Helper::contentOfIterable(const AbstractType::Ptr iterable, const TopDUContext* topContext)
{
    if (!iterable || !topContext) {
        return AbstractType::Ptr(new IntegralType(IntegralType::TypeMixed));
    }

    // Handle ArrayType - return element type
    if (ArrayType* arr = dynamic_cast<ArrayType*>(iterable.data())) {
        return arr->elementType();
    }

    // Handle MapType - return key type (for iteration over dicts)
    if (MapType* map = dynamic_cast<MapType*>(iterable.data())) {
        return map->keyType().abstractType();
    }

    // Handle UnsureType - recurse into each type and merge
    if (UnsureType* unsure = dynamic_cast<UnsureType*>(iterable.data())) {
        AbstractType::Ptr result;
        for (uint i = 0; i < unsure->typesSize(); i++) {
            AbstractType::Ptr type = unsure->types()[i].abstractType();
            AbstractType::Ptr content = contentOfIterable(type, topContext);
            result = mergeTypes(result, content);
        }
        if (result) {
            return result;
        }
    }

    return AbstractType::Ptr(new IntegralType(IntegralType::TypeMixed));
}

AbstractType::Ptr Helper::mergeTypes(AbstractType::Ptr type, const AbstractType::Ptr newType)
{
    if (!type) {
        return newType;
    }
    if (!newType) {
        return type;
    }

    IntegralType::Ptr integralType = type.dynamicCast<IntegralType>();
    IntegralType::Ptr newIntegralType = newType.dynamicCast<IntegralType>();
    
    if (integralType && integralType->dataType() == IntegralType::TypeMixed) {
        return type;
    }
    if (newIntegralType && newIntegralType->dataType() == IntegralType::TypeMixed) {
        return newType;
    }

    if (type->equals(newType.data())) {
        return type;
    }

    UnsureType::Ptr result(new UnsureType());
    result->addType(IndexedType(type));
    result->addType(IndexedType(newType));
    return result;
}

bool Helper::isUsefulType(AbstractType::Ptr type)
{
    if (!type) {
        return false;
    }

    IntegralType::Ptr integral = type.dynamicCast<IntegralType>();
    if (integral) {
        auto itype = integral->dataType();
        return itype != IntegralType::TypeMixed && itype != IntegralType::TypeVoid;
    }

    return true;
}

Declaration* Helper::declarationForName(const QString& name,
                                         const CursorInRevision& location,
                                         const DUContext* context)
{
    if (!context || name.isEmpty()) {
        return nullptr;
    }

    DUChainReadLocker lock(DUChain::lock());

    QualifiedIdentifier id(name);
    
    // First try findLocalDeclarations (like Python's helpers.cpp does)
    QList<Declaration*> localDeclarations = context->findLocalDeclarations(
        id.last(), location, nullptr,
        AbstractType::Ptr(), DUContext::DontResolveAliases);
    
    if (!localDeclarations.isEmpty()) {
        return localDeclarations.last();
    }
    
    // Use findDeclarations with findUntil parameter like Python does
    // This finds declarations whose range ends before findUntil
    // If location is valid, use it; otherwise use end of top context
    CursorInRevision findUntil = location.isValid() ? location 
                                                     : context->topContext()->range().end;
    
    QList<Declaration*> declarations = context->findDeclarations(id, findUntil);
    if (!declarations.isEmpty()) {
        return declarations.first();
    }
    
    // Search in parent contexts (like Python's helpers.cpp does)
    const DUContext* currentContext = context;
    while ((currentContext = currentContext->parentContext())) {
        // Try findLocalDeclarations first
        QList<Declaration*> localDecls = currentContext->findLocalDeclarations(
            id.last(), location, nullptr,
            AbstractType::Ptr(), DUContext::DontResolveAliases);
        
        if (!localDecls.isEmpty()) {
            return localDecls.last();
        }
        
        // Also try findDeclarations with findUntil for broader search
        CursorInRevision parentFindUntil = location.isValid() ? location 
                                                               : currentContext->topContext()->range().end;
        declarations = currentContext->findDeclarations(id, parentFindUntil);
        if (!declarations.isEmpty()) {
            return declarations.first();
        }
    }

    // Final fallback: try with invalid cursor to catch exact position matches
    // This handles the case where position equals declaration start position
    declarations = context->findDeclarations(id, CursorInRevision::invalid());
    if (!declarations.isEmpty()) {
        return declarations.first();
    }
    
    // Search in parent contexts with invalid cursor as last resort
    currentContext = context;
    while ((currentContext = currentContext->parentContext())) {
        declarations = currentContext->findDeclarations(id, CursorInRevision::invalid());
        if (!declarations.isEmpty()) {
            return declarations.first();
        }
    }

    return nullptr;
}

Helper::FuncInfo Helper::functionForCalled(Declaration* called, bool isAlias)
{
    FuncInfo info = {nullptr, false};

    if (!called) {
        return info;
    }

    DUChainReadLocker lock(DUChain::lock());

    if (isAlias) {
        ClassDeclaration* classDecl = dynamic_cast<ClassDeclaration*>(called);
        if (classDecl) {
            DUContext* ctx = classDecl->internalContext();
            if (ctx) {
                QualifiedIdentifier initId(QStringLiteral("初始化"));
                QList<Declaration*> decls = ctx->findDeclarations(initId);
                if (!decls.isEmpty()) {
                    info.declaration = decls.first();
                    info.isConstructor = true;
                    return info;
                }
            }
        }
    }

    FunctionDeclaration* funcDecl = dynamic_cast<FunctionDeclaration*>(called);
    if (funcDecl) {
        info.declaration = funcDecl;
        info.isConstructor = false;
        return info;
    }

    return info;
}

QVector<DUContext*> Helper::internalContextsForClass(const StructureType::Ptr classType,
                                                     const TopDUContext* context,
                                                     int depth)
{
    QVector<DUContext*> result;

    if (!classType || !context || depth > 5) {
        return result;
    }

    DUChainReadLocker lock(DUChain::lock());

    Declaration* decl = classType->declaration(context);
    if (!decl) {
        return result;
    }

    DUContext* ctx = decl->internalContext();
    if (ctx) {
        result.append(ctx);
    }

    return result;
}

Declaration* Helper::resolveAliasDeclaration(Declaration* decl)
{
    if (!decl) {
        return nullptr;
    }

    AliasDeclaration* alias = dynamic_cast<AliasDeclaration*>(decl);
    if (alias) {
        return alias->aliasedDeclaration().data();
    }

    return decl;
}

}
