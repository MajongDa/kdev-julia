#ifndef JULIA_HELPERS_H
#define JULIA_HELPERS_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QCache>

#include <language/duchain/declaration.h>
#include <language/duchain/types/unsuretype.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/types/structuretype.h>
#include <language/duchain/types/integraltype.h>

#include "parser/ast.h"

namespace Julia {

struct ImportInfo {
    QString moduleName;
    QString moduleFile;
    QStringList importedSymbols;
};

class Helper
{
public:
    static KDevelop::Declaration* accessAttribute(const KDevelop::AbstractType::Ptr accessed,
                                                  const QString& attribute,
                                                  const KDevelop::TopDUContext* topContext);

    static KDevelop::AbstractType::Ptr resolveAliasType(const KDevelop::AbstractType::Ptr eventualAlias);

    static KDevelop::AbstractType::Ptr contentOfIterable(const KDevelop::AbstractType::Ptr iterable,
                                                         const KDevelop::TopDUContext* topContext);

    static KDevelop::AbstractType::Ptr mergeTypes(KDevelop::AbstractType::Ptr type,
                                                   const KDevelop::AbstractType::Ptr newType);

    static bool isUsefulType(KDevelop::AbstractType::Ptr type);

    static KDevelop::Declaration* declarationForName(const QString& name,
                                                     const KDevelop::CursorInRevision& location,
                                                     const KDevelop::DUContext* context);

    struct FuncInfo {
        KDevelop::Declaration* declaration;
        bool isConstructor;
    };
    static FuncInfo functionForCalled(KDevelop::Declaration* called, bool isAlias = true);

    static QVector<KDevelop::DUContext*> internalContextsForClass(const KDevelop::StructureType::Ptr classType,
                                                                   const KDevelop::TopDUContext* context,
                                                                   int depth = 0);

    static KDevelop::Declaration* resolveAliasDeclaration(KDevelop::Declaration* decl);

    // Import handling - find module files in Julia's LOAD_PATH
    static QStringList findJuliaModuleFiles(const QString& moduleName);
    static QStringList getJuliaLoadPath();

    // Parse exported symbols from .jl file
    static QStringList getExportedSymbols(const QString& filePath);

    // Resolve imported symbol type
    static KDevelop::AbstractType::Ptr resolveImportedSymbol(const QString& filePath,
                                                            const QString& symbolName,
                                                            const KDevelop::TopDUContext* topContext);

    // Track pending imports for lazy resolution
    static void addPendingImport(const ImportInfo& info);
    static QList<ImportInfo> pendingImports();
    static void clearPendingImports();
};

}

#endif
