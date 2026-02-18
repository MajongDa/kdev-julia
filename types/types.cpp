#include "types.h"

#include <language/duchain/types/structuretype.h>
#include <language/duchain/declaration.h>

#include "../juliadebug.h"

namespace Julia {

TypeMapper::JuliaType TypeMapper::stringToJuliaType(const QString& typeStr)
{
    static const QHash<QString, JuliaType> typeMap = {
        {QStringLiteral("Int"), JuliaType::Int},
        {QStringLiteral("Int8"), JuliaType::Int8},
        {QStringLiteral("Int16"), JuliaType::Int16},
        {QStringLiteral("Int32"), JuliaType::Int32},
        {QStringLiteral("Int64"), JuliaType::Int64},
        {QStringLiteral("UInt"), JuliaType::UInt},
        {QStringLiteral("UInt8"), JuliaType::UInt8},
        {QStringLiteral("UInt16"), JuliaType::UInt16},
        {QStringLiteral("UInt32"), JuliaType::UInt32},
        {QStringLiteral("UInt64"), JuliaType::UInt64},
        {QStringLiteral("Float16"), JuliaType::Float16},
        {QStringLiteral("Float32"), JuliaType::Float32},
        {QStringLiteral("Float64"), JuliaType::Float64},
        {QStringLiteral("Float"), JuliaType::Float64},
        {QStringLiteral("BigFloat"), JuliaType::Float64},
        {QStringLiteral("Bool"), JuliaType::Bool},
        {QStringLiteral("Char"), JuliaType::Char},
        {QStringLiteral("String"), JuliaType::String},
        {QStringLiteral("Symbol"), JuliaType::Symbol},
        {QStringLiteral("Nothing"), JuliaType::Nothing},
        {QStringLiteral("Missing"), JuliaType::Missing},
        {QStringLiteral("Any"), JuliaType::Any},
        {QStringLiteral("Array"), JuliaType::Array},
        {QStringLiteral("Vector"), JuliaType::Array},
        {QStringLiteral("Matrix"), JuliaType::Array},
        {QStringLiteral("Dict"), JuliaType::Dict},
        {QStringLiteral("Tuple"), JuliaType::Tuple},
        {QStringLiteral("Set"), JuliaType::Set},
        {QStringLiteral("Function"), JuliaType::Function},
        {QStringLiteral("Type"), JuliaType::Type},
        {QStringLiteral("Union"), JuliaType::Union},
    };

    return typeMap.value(typeStr, JuliaType::Unknown);
}

QString TypeMapper::juliaTypeToString(JuliaType type)
{
    switch (type) {
        case JuliaType::Int: return QStringLiteral("Int");
        case JuliaType::Int8: return QStringLiteral("Int8");
        case JuliaType::Int16: return QStringLiteral("Int16");
        case JuliaType::Int32: return QStringLiteral("Int32");
        case JuliaType::Int64: return QStringLiteral("Int64");
        case JuliaType::UInt: return QStringLiteral("UInt");
        case JuliaType::UInt8: return QStringLiteral("UInt8");
        case JuliaType::UInt16: return QStringLiteral("UInt16");
        case JuliaType::UInt32: return QStringLiteral("UInt32");
        case JuliaType::UInt64: return QStringLiteral("UInt64");
        case JuliaType::Float16: return QStringLiteral("Float16");
        case JuliaType::Float32: return QStringLiteral("Float32");
        case JuliaType::Float64: return QStringLiteral("Float64");
        case JuliaType::Bool: return QStringLiteral("Bool");
        case JuliaType::Char: return QStringLiteral("Char");
        case JuliaType::String: return QStringLiteral("String");
        case JuliaType::Symbol: return QStringLiteral("Symbol");
        case JuliaType::Nothing: return QStringLiteral("Nothing");
        case JuliaType::Missing: return QStringLiteral("Missing");
        case JuliaType::Any: return QStringLiteral("Any");
        case JuliaType::Array: return QStringLiteral("Array");
        case JuliaType::Dict: return QStringLiteral("Dict");
        case JuliaType::Tuple: return QStringLiteral("Tuple");
        case JuliaType::Set: return QStringLiteral("Set");
        case JuliaType::Function: return QStringLiteral("Function");
        case JuliaType::Type: return QStringLiteral("Type");
        case JuliaType::Union: return QStringLiteral("Union");
        case JuliaType::Struct: return QStringLiteral("Struct");
        case JuliaType::Module: return QStringLiteral("Module");
        default: return QStringLiteral("Unknown");
    }
}

uint TypeMapper::integralTypeKind(JuliaType type)
{
    switch (type) {
        case JuliaType::Int:
        case JuliaType::Int8:
        case JuliaType::Int16:
        case JuliaType::Int32:
        case JuliaType::Int64:
        case JuliaType::UInt:
        case JuliaType::UInt8:
        case JuliaType::UInt16:
        case JuliaType::UInt32:
        case JuliaType::UInt64:
            return KDevelop::IntegralType::TypeInt;
        case JuliaType::Float16:
        case JuliaType::Float32:
            return KDevelop::IntegralType::TypeFloat;
        case JuliaType::Float64:
            return KDevelop::IntegralType::TypeDouble;
        case JuliaType::Bool:
            return KDevelop::IntegralType::TypeBoolean;
        case JuliaType::Char:
            return KDevelop::IntegralType::TypeChar;
        case JuliaType::String:
            return KDevelop::IntegralType::TypeString;
        case JuliaType::Nothing:
        case JuliaType::Missing:
            return KDevelop::IntegralType::TypeVoid;
        default:
            return KDevelop::IntegralType::TypeNone;
    }
}

QStringList TypeMapper::parseTypeParams(const QString& typeStr)
{
    QStringList result;
    int braceStart = typeStr.indexOf(QLatin1Char('{'));
    if (braceStart < 0) {
        return result;
    }
    
    int braceCount = 0;
    int start = braceStart + 1;
    QString current;
    
    for (int i = start; i < typeStr.length(); ++i) {
        QChar c = typeStr[i];
        if (c == QLatin1Char('{')) {
            braceCount++;
            current += c;
        } else if (c == QLatin1Char('}')) {
            if (braceCount > 0) {
                braceCount--;
                current += c;
            } else {
                if (!current.isEmpty()) {
                    result.append(current.trimmed());
                }
                break;
            }
        } else if (c == QLatin1Char(',') && braceCount == 0) {
            if (!current.isEmpty()) {
                result.append(current.trimmed());
            }
            current.clear();
        } else {
            current += c;
        }
    }
    
    return result;
}

KDevelop::AbstractType* TypeMapper::typeFromString(const QString& typeStr, KDevelop::DUContext* context)
{
    QString baseType;
    QStringList params;
    
    if (typeStr.contains(QLatin1Char('{'))) {
        int bracePos = typeStr.indexOf(QLatin1Char('{'));
        baseType = typeStr.left(bracePos);
        params = parseTypeParams(typeStr);
    } else {
        baseType = typeStr;
    }
    
    JuliaType baseTypeEnum = stringToJuliaType(baseType);
    
    switch (baseTypeEnum) {
        case JuliaType::Array: {
            auto* array = new KDevelop::ArrayType();
            if (!params.isEmpty()) {
                KDevelop::AbstractType* elementType = typeFromString(params[0], context);
                if (elementType) {
                    array->setElementType(KDevelop::AbstractType::Ptr(elementType));
                }
            }
            return array;
        }
        
        case JuliaType::Dict: {
            auto* map = new KDevelop::MapType();
            if (params.size() >= 2) {
                KDevelop::AbstractType* keyType = typeFromString(params[0], context);
                KDevelop::AbstractType* valType = typeFromString(params[1], context);
                if (keyType) {
                    map->replaceKeyType(KDevelop::AbstractType::Ptr(keyType));
                }
                if (valType) {
                    map->replaceContentType(KDevelop::AbstractType::Ptr(valType));
                }
            }
            return map;
        }
        
        case JuliaType::Tuple: {
            auto* structType = new KDevelop::StructureType();
            return structType;
        }
        
        case JuliaType::Union: {
            auto* unsure = new KDevelop::UnsureType();
            if (!params.isEmpty()) {
                for (const QString& param : params) {
                    KDevelop::AbstractType* memberType = typeFromString(param, context);
                    if (memberType) {
                        unsure->addType(memberType->indexed());
                    }
                }
            }
            return unsure;
        }
        
        case JuliaType::Set: {
            auto* structType = new KDevelop::StructureType();
            return structType;
        }
        
        case JuliaType::Function: {
            auto* func = new KDevelop::FunctionType();
            return func;
        }
        
        case JuliaType::Struct:
        case JuliaType::Module: {
            auto* structType = new KDevelop::StructureType();
            return structType;
        }
        
        default: {
            KDevelop::AbstractType* type = nullptr;
            
            if (baseTypeEnum != JuliaType::Unknown && baseTypeEnum != JuliaType::Any) {
                type = new KDevelop::IntegralType(integralTypeKind(baseTypeEnum));
            } else {
                auto* structType = new KDevelop::StructureType();
                type = structType;
                
                if (context) {
                    KDevelop::QualifiedIdentifier id(baseType);
                    auto decls = context->findDeclarations(id, KDevelop::CursorInRevision::invalid());
                    
                    if (!decls.isEmpty()) {
                        KDevelop::Declaration* decl = decls.first();
                        structType->setDeclaration(decl);
                    }
                }
            }
            
            return type;
        }
    }
}

KDevelop::AbstractType* TypeMapper::typeFromAstNode(AstNode* node)
{
    if (!node) {
        return nullptr;
    }

    switch (node->kind()) {
        case NodeKind::Curly:
            return typeFromParametricType(node);
        
        case NodeKind::Call:
            return typeFromString(node->text());
        
        case NodeKind::Identifier:
            return typeFromString(node->text());
        
        case NodeKind::TypeAnnotation:
            if (AstNode* typeNode = node->lastChild()) {
                return typeFromAstNode(typeNode);
            }
            break;
        
        default:
            break;
    }

    return typeFromString(node->text());
}

KDevelop::AbstractType* TypeMapper::typeFromParametricType(AstNode* node)
{
    if (!node) {
        return nullptr;
    }
    
    CurlyNode* curly = dynamic_cast<CurlyNode*>(node);
    if (!curly) {
        return nullptr;
    }
    
    QString baseType = curly->functionName();
    QList<AstNode*> typeParams = curly->arguments();
    
    JuliaType baseEnum = stringToJuliaType(baseType);
    
    switch (baseEnum) {
        case JuliaType::Array: {
            auto* array = new KDevelop::ArrayType();
            if (!typeParams.isEmpty()) {
                AstNode* firstParam = typeParams.first();
                if (firstParam) {
                    KDevelop::AbstractType* elementType = typeFromAstNode(firstParam);
                    if (elementType) {
                        array->setElementType(KDevelop::AbstractType::Ptr(elementType));
                    }
                }
            }
            return array;
        }
        
        case JuliaType::Dict: {
            auto* map = new KDevelop::MapType();
            if (typeParams.size() >= 2) {
                KDevelop::AbstractType* keyType = typeFromAstNode(typeParams.at(0));
                KDevelop::AbstractType* valType = typeFromAstNode(typeParams.at(1));
                if (keyType) {
                    map->replaceKeyType(KDevelop::AbstractType::Ptr(keyType));
                }
                if (valType) {
                    map->replaceContentType(KDevelop::AbstractType::Ptr(valType));
                }
            }
            return map;
        }
        
        case JuliaType::Tuple: {
            auto* structType = new KDevelop::StructureType();
            return structType;
        }
        
        case JuliaType::Union: {
            auto* unsure = new KDevelop::UnsureType();
            for (AstNode* param : typeParams) {
                KDevelop::AbstractType* memberType = typeFromAstNode(param);
                if (memberType) {
                    unsure->addType(memberType->indexed());
                }
            }
            return unsure;
        }
        
        case JuliaType::Set: {
            auto* structType = new KDevelop::StructureType();
            return structType;
        }
        
        default: {
            auto* structType = new KDevelop::StructureType();
            return structType;
        }
    }
}

}
