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
        case JuliaType::Float64:
            return KDevelop::IntegralType::TypeDouble;
        case JuliaType::Bool:
            return KDevelop::IntegralType::TypeBoolean;
        case JuliaType::Char:
        case JuliaType::String:
            return KDevelop::IntegralType::TypeString;
        case JuliaType::Nothing:
        case JuliaType::Missing:
            return KDevelop::IntegralType::TypeVoid;
        default:
            return KDevelop::IntegralType::TypeNone;
    }
}

KDevelop::AbstractType* TypeMapper::typeFromString(const QString& typeStr, KDevelop::DUContext* context)
{
    JuliaType juliaType = stringToJuliaType(typeStr);
    
    KDevelop::AbstractType* type = nullptr;

    if (juliaType == JuliaType::Unknown) {
        auto* structType = new KDevelop::StructureType();
        type = structType;
    } else if (juliaType == JuliaType::Struct || juliaType == JuliaType::Module) {
        auto* structType = new KDevelop::StructureType();
        type = structType;
    } else if (juliaType == JuliaType::Function) {
        auto* func = new KDevelop::FunctionType();
        type = func;
    } else if (juliaType == JuliaType::Dict || juliaType == JuliaType::Tuple || juliaType == JuliaType::Set) {
        auto* structType = new KDevelop::StructureType();
        type = structType;
    } else {
        auto* integral = new KDevelop::IntegralType(integralTypeKind(juliaType));
        type = integral;
    }
    
    if (type && context) {
        KDevelop::QualifiedIdentifier id(typeStr);
        auto decls = context->findDeclarations(id, KDevelop::CursorInRevision::invalid());
        
        if (!decls.isEmpty()) {
            KDevelop::Declaration* decl = decls.first();
            if (auto* structType = dynamic_cast<KDevelop::StructureType*>(type)) {
                structType->setDeclaration(decl);
                qCDebug(KDEV_JULIA) << "Linked type" << typeStr << "to declaration:" << decl->identifier().toString();
            }
        }
    }
    
    return type;
}

KDevelop::AbstractType* TypeMapper::typeFromAstNode(AstNode* node)
{
    if (!node) {
        return nullptr;
    }

    if (node->kind() == NodeKind::Call) {
        QString typeName = node->text();
        if (typeName.contains(QLatin1Char('{'))) {
            return typeFromParametricType(node);
        }
        
        if (typeName.startsWith(QLatin1String("Union{"))) {
            auto* callNode = dynamic_cast<CallNode*>(node);
            if (callNode) {
                auto* structType = new KDevelop::StructureType();
                qCDebug(KDEV_JULIA) << "Processing Union type with" << callNode->argumentCount() << "arguments";
                return structType;
            }
        }
    }

    if (node->kind() == NodeKind::Identifier) {
        return typeFromString(node->text());
    }

    if (node->kind() == NodeKind::TypeAnnotation) {
        if (AstNode* typeNode = node->lastChild()) {
            return typeFromAstNode(typeNode);
        }
    }

    return typeFromString(node->text());
}

KDevelop::AbstractType* TypeMapper::typeFromParametricType(AstNode* node)
{
    if (!node) {
        return nullptr;
    }
    
    QString typeName;
    QList<AstNode*> typeParams;
    
    if (node->kind() == NodeKind::Call) {
        auto* callNode = dynamic_cast<CallNode*>(node);
        if (callNode) {
            typeName = callNode->functionName();
            typeParams = callNode->arguments();
        } else {
            typeName = node->text();
            int bracePos = typeName.indexOf(QLatin1Char('{'));
            if (bracePos > 0) {
                typeName = typeName.left(bracePos);
            }
        }
    } else {
        typeName = node->text();
        int bracePos = typeName.indexOf(QLatin1Char('{'));
        if (bracePos > 0) {
            typeName = typeName.left(bracePos);
        }
    }
    
    if (typeName == QLatin1String("Array") || typeName == QLatin1String("Vector") || typeName == QLatin1String("Matrix")) {
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
    
    if (typeName == QLatin1String("Dict")) {
        auto* structType = new KDevelop::StructureType();
        
        if (typeParams.size() >= 2) {
            KDevelop::AbstractType* keyType = typeFromAstNode(typeParams.at(0));
            KDevelop::AbstractType* valType = typeFromAstNode(typeParams.at(1));
            if (keyType && valType) {
                qCDebug(KDEV_JULIA) << "Dict key type:" << keyType->toString() << "value type:" << valType->toString();
            }
        }
        
        return structType;
    }
    
    if (typeName == QLatin1String("Tuple")) {
        auto* structType = new KDevelop::StructureType();
        
        if (!typeParams.isEmpty()) {
            qCDebug(KDEV_JULIA) << "Tuple has" << typeParams.size() << "type parameters";
        }
        
        return structType;
    }
    
    if (typeName == QLatin1String("Set")) {
        auto* structType = new KDevelop::StructureType();
        
        if (!typeParams.isEmpty()) {
            KDevelop::AbstractType* elemType = typeFromAstNode(typeParams.first());
            if (elemType) {
                qCDebug(KDEV_JULIA) << "Set element type:" << elemType->toString();
            }
        }
        
        return structType;
    }
    
    auto* structType = new KDevelop::StructureType();
    return structType;
}

}
