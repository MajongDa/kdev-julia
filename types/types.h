#ifndef JULIA_TYPES_H
#define JULIA_TYPES_H

#include <QString>

#include <language/duchain/types/integraltype.h>
#include <language/duchain/types/functiontype.h>
#include <language/duchain/types/pointertype.h>
#include <language/duchain/types/arraytype.h>
#include <language/duchain/ducontext.h>

#include "../parser/ast.h"

namespace Julia {

class TypeMapper
{
public:
    static KDevelop::AbstractType* typeFromString(const QString& typeStr, KDevelop::DUContext* context = nullptr);
    static KDevelop::AbstractType* typeFromAstNode(class AstNode* node);
    static KDevelop::AbstractType* typeFromParametricType(class AstNode* node);

    enum class JuliaType {
        Unknown,
        Int,
        Int8,
        Int16,
        Int32,
        Int64,
        UInt,
        UInt8,
        UInt16,
        UInt32,
        UInt64,
        Float16,
        Float32,
        Float64,
        Bool,
        Char,
        String,
        Symbol,
        Nothing,
        Missing,
        Any,
        Array,
        Dict,
        Tuple,
        Set,
        Function,
        Type,
        Union,
        Struct,
        Module
    };

    static JuliaType stringToJuliaType(const QString& typeStr);
    static QString juliaTypeToString(JuliaType type);

private:
    static uint integralTypeKind(JuliaType type);
};

}

#endif
