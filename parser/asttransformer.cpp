#include "asttransformer.h"

#include <QJsonDocument>
#include <QDebug>

#include "juliadebug.h"

namespace Julia {

CodeAst* AstTransformer::parse(const QByteArray& json)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse JSON:" << error.errorString();
        return nullptr;
    }
    
    if (!doc.isObject()) {
        qWarning() << "JSON is not an object";
        return nullptr;
    }
    
    CodeAst* code = new CodeAst();
    QJsonObject root = doc.object();
    
    setRange(code, root);

    if (root.contains(QLatin1String("children"))) {
        QJsonArray body = root.value(QLatin1String("children")).toArray();
        qCDebug(KDEV_JULIA()) << body;
        for (const QJsonValue& item : body) {
            if (item.isObject()) {
                Ast* child = fromJson(item.toObject(), code);
                if (child) {
                    code->body.append(child);
                }
            }
        }
    }

    return code;
}

Ast* AstTransformer::fromJson(const QJsonObject& json, Ast* parent)
{
    QString kindStr = json.value(QLatin1String("kind")).toString();
    AstType type = stringToAstType(kindStr);
    
    Ast* node = nullptr;
    
    switch (type) {
        case AstType::FunctionDefinitionAstType:
            node = new FunctionDefinitionAst(parent);
            break;
        case AstType::StructAstType:
            node = new StructAst(parent);
            break;
        case AstType::ModuleAstType:
            node = new ModuleAst(parent);
            break;
        case AstType::BaremoduleAstType:
            node = new BaremoduleAst(parent);
            break;
        case AstType::AbstractAstType:
            node = new AbstractAst(parent);
            break;
        case AstType::PrimitiveAstType:
            node = new PrimitiveAst(parent);
            break;
        case AstType::MacroAstType:
            node = new MacroAst(parent);
            break;
        case AstType::CallAstType:
            node = new CallAst(parent);
            break;
        case AstType::AssignmentAstType:
            node = new AssignmentAst(parent);
            break;
        case AstType::ReturnAstType:
            node = new ReturnAst(parent);
            break;
        case AstType::IfAstType:
            node = new IfAst(parent);
            break;
        case AstType::ForAstType:
            node = new ForAst(parent);
            break;
        case AstType::WhileAstType:
            node = new WhileAst(parent);
            break;
        case AstType::TryAstType:
            node = new TryAst(parent);
            break;
        case AstType::ImportAstType:
            node = new ImportAst(parent);
            break;
        case AstType::UsingAstType:
            node = new UsingAst(parent);
            break;
        case AstType::ExportAstType:
            node = new ExportAst(parent);
            break;
        case AstType::BreakAstType:
            node = new BreakAst(parent);
            break;
        case AstType::ContinueAstType:
            node = new ContinueAst(parent);
            break;
        case AstType::IdentifierAstType: {
            qCDebug(KDEV_JULIA()) << "IdentifierAstType created";
            QString text = json.value(QLatin1String("text")).toString();
            node = new IdentifierAst(parent, text);
            break;
        }
        case AstType::NumberAstType: {
            QString text = json.value(QLatin1String("text")).toString();
            node = new NumberAst(parent, AstType::NumberAstType);
            static_cast<NumberAst*>(node)->value = text;
            break;
        }
        case AstType::StringAstType: {
            QString text = json.value(QLatin1String("text")).toString();
            node = new StringAst(parent, AstType::StringAstType);
            static_cast<StringAst*>(node)->value = text;
            break;
        }
        case AstType::TypeAnnotationAstType:
            node = new TypeAnnotationAst(parent);
            break;
        case AstType::CurlyAstType:
            node = new CurlyAst(parent);
            break;
        case AstType::BinaryOperationAstType:
            node = new BinaryOperationAst(parent);
            break;
        case AstType::UnaryOperationAstType:
            node = new UnaryOperationAst(parent);
            break;
        case AstType::TupleAstType:
            node = new TupleAst(parent);
            break;
        case AstType::ListAstType:
            node = new ListAst(parent);
            break;
        case AstType::DictAstType:
            node = new DictAst(parent);
            break;
        case AstType::SubscriptAstType:
            node = new SubscriptAst(parent);
            break;
        case AstType::AttributeAstType:
            node = new AttributeAst(parent);
            break;
        case AstType::StarredAstType:
            node = new StarredAst(parent);
            break;
        case AstType::LambdaAstType:
            node = new LambdaAst(parent);
            break;
        case AstType::IfExpressionAstType:
            node = new IfExpressionAst(parent);
            break;
        case AstType::GeneratorAstType:
            node = new GeneratorAst(parent);
            break;
        case AstType::RefAstType:
            node = new RefAst(parent);
            break;
        case AstType::ImportPathAstType:
            node = new ImportPathAst(parent);
            break;
        case AstType::MacroCallAstType:
            node = new MacroCallAst(parent);
            break;
        case AstType::InterpolatedStringAstType:
            node = new InterpolatedStringAst(parent);
            break;
        case AstType::KwArgAstType:
            node = new KwArgAst(parent);
            break;
        case AstType::ArgumentsAstType:
            node = new ArgumentsAst(parent);
            break;
        case AstType::ArgAstType: {
            node = new ArgAst(parent);
            // Try to extract argument name from children
            auto* argNode = static_cast<ArgAst*>(node);
            QJsonArray argChildren = json.value(QLatin1String("children")).toArray();
            for (const QJsonValue& argChild : argChildren) {
                if (argChild.isObject()) {
                    QJsonObject argChildObj = argChild.toObject();
                    QString childKind = argChildObj.value(QLatin1String("kind")).toString();
                    if (childKind == QLatin1String("IdentifierAst")) {
                        QString argName = argChildObj.value(QLatin1String("text")).toString();
                        argNode->argumentName = new IdentifierAst(parent, argName);
                    } else if (childKind == QLatin1String("::")) {
                        // Type annotation - set as annotation
                        // This will be visited later and converted to TypeAnnotationAst
                        // For now, skip - it will be processed as a child
                    }
                }
            }
            break;
        }
        case AstType::KeywordAstType:
            node = new KeywordAst(parent);
            break;
        case AstType::AliasAstType:
            node = new AliasAst(parent);
            break;
        case AstType::ExceptionHandlerAstType:
            node = new ExceptionHandlerAst(parent);
            break;
        case AstType::ComprehensionAstType:
            node = new ComprehensionAst(parent);
            break;
        case AstType::MatchAstType:
            node = new MatchAst(parent);
            break;
        case AstType::MatchCaseAstType:
            node = new MatchCaseAst(parent);
            break;
        case AstType::ConstAstType:
            node = new ConstAst(parent);
            break;
        case AstType::LocalAstType:
            node = new LocalAst(parent);
            break;
        case AstType::GlobalAstType:
            node = new GlobalAst(parent);
            break;
        case AstType::LetAstType:
            node = new LetAst(parent);
            break;
        case AstType::DoAstType:
            node = new DoAst(parent);
            break;
        case AstType::ParameterAstType:
            node = new ParameterAst(parent);
            break;
        default:
            node = new Ast(parent, type);
            break;
    }
    
    setRange(node, json);
    
    if (json.contains(QLatin1String("children"))) {
        QJsonArray children = json.value(QLatin1String("children")).toArray();
        
        // For FunctionDefinitionAst, we need special handling to categorize arguments
        if (node->astType == AstType::FunctionDefinitionAstType) {
            auto* fn = static_cast<FunctionDefinitionAst*>(node);
            
            // First, find the parameters node (keyword args delimiter)
            // If there's no parameters node, all args are positional
            int parametersIndex = -1;
            for (int i = 0; i < children.size(); ++i) {
                if (children.at(i).isObject()) {
                    QJsonObject childObj = children.at(i).toObject();
                    QString childKind = childObj.value(QLatin1String("kind")).toString();
                    if (childKind == QLatin1String("parameters")) {
                        parametersIndex = i;
                        break;
                    }
                }
            }
            
            // Process all children up to parameters (or all if no parameters)
            int endOfArgs = (parametersIndex >= 0) ? parametersIndex : children.size();
            for (int i = 0; i < endOfArgs; ++i) {
                if (!children.at(i).isObject()) continue;
                QJsonObject childObj = children.at(i).toObject();
                QString childKind = childObj.value(QLatin1String("kind")).toString();
                
                // Call node contains the positional arguments
                if (childKind == QLatin1String("call")) {
                    processPositionalArguments(fn, childObj, node);
                }
            }
            
            // Process function body (after parameters or call)
            int startOfBody = endOfArgs;
            if (parametersIndex >= 0) {
                startOfBody = parametersIndex + 1;  // Skip parameters node
            }
            for (int i = startOfBody; i < children.size(); ++i) {
                if (!children.at(i).isObject()) continue;
                QJsonObject childObj = children.at(i).toObject();
                QString childKind = childObj.value(QLatin1String("kind")).toString();
                
                // If there's a type annotation (::T), it's return type
                if (childKind == QLatin1String("::")) {
                    if (!fn->returns) {
                        fn->returns = fromJson(childObj, node);
                    }
                }
                // Otherwise it's body statement
                else if (childKind != QLatin1String("parameters")) {
                    Ast* bodyStmt = fromJson(childObj, node);
                    if (bodyStmt) {
                        fn->body.append(bodyStmt);
                    }
                }
            }
            
            // Process keyword-only arguments if parameters node exists
            if (parametersIndex >= 0) {
                QJsonObject paramsObj = children.at(parametersIndex).toObject();
                processKeywordArguments(fn, paramsObj, node);
            }
        }
        else {
            // Default processing for other node types
            for (const QJsonValue& childValue : children) {
                if (childValue.isObject()) {
                    Ast* childNode = fromJson(childValue.toObject(), node);
                    if (childNode) {
                        if (node->astType == AstType::CallAstType) {
                            auto* call = static_cast<CallAst*>(node);
                            if (call->function == nullptr && childNode->astType == AstType::IdentifierAstType) {
                                call->function = static_cast<IdentifierAst*>(childNode);
                            } else {
                                call->arguments.append(childNode);
                            }
                        }
                        else if (node->astType == AstType::AssignmentAstType) {
                            auto* assign = static_cast<AssignmentAst*>(node);
                            if (assign->targets.isEmpty()) {
                                assign->targets.append(childNode);
                            } else {
                                assign->value = childNode;
                            }
                        }
                        else if (node->astType == AstType::IfAstType) {
                            auto* ifStmt = static_cast<IfAst*>(node);
                            if (ifStmt->condition == nullptr && childNode->astType != AstType::StatementAstType) {
                                ifStmt->condition = childNode;
                            } else if (!childNode->isStatement() && ifStmt->body.size() <= ifStmt->orelse.size()) {
                                ifStmt->body.append(childNode);
                            } else {
                                ifStmt->orelse.append(childNode);
                            }
                        }
                        else if (node->astType == AstType::ModuleAstType) {
                            static_cast<ModuleAst*>(node)->body.append(childNode);
                        }
                        else if (node->astType == AstType::StructAstType) {
                            static_cast<StructAst*>(node)->body.append(childNode);
                        }
                        else if (node->astType == AstType::TopLevelAstType) {
                            static_cast<CodeAst*>(node)->body.append(childNode);
                        }
                    }
                }
            }
        }
    }
    
    return node;
}

AstType AstTransformer::stringToAstType(const QString& kindStr)
{
    static const QHash<QString, AstType> kindMap = {
        {QStringLiteral("toplevel"), AstType::TopLevelAstType},
        {QStringLiteral("block"), AstType::StatementAstType},
        {QStringLiteral("function"), AstType::FunctionDefinitionAstType},
        {QStringLiteral("struct"), AstType::StructAstType},
        {QStringLiteral("module"), AstType::ModuleAstType},
        {QStringLiteral("baremodule"), AstType::BaremoduleAstType},
        {QStringLiteral("abstract"), AstType::AbstractAstType},
        {QStringLiteral("primitive"), AstType::PrimitiveAstType},
        {QStringLiteral("macro"), AstType::MacroAstType},
        {QStringLiteral("macrocall"), AstType::MacroCallAstType},
        {QStringLiteral("assignment"), AstType::AssignmentAstType},
        {QStringLiteral("return"), AstType::ReturnAstType},
        {QStringLiteral("if"), AstType::IfAstType},
        {QStringLiteral("while"), AstType::WhileAstType},
        {QStringLiteral("for"), AstType::ForAstType},
        {QStringLiteral("break"), AstType::BreakAstType},
        {QStringLiteral("continue"), AstType::ContinueAstType},
        {QStringLiteral("call"), AstType::CallAstType},
        {QStringLiteral("curly"), AstType::CurlyAstType},
        {QStringLiteral("where"), AstType::ExpressionAstType},
        {QStringLiteral("parameters"), AstType::ParameterAstType},
        {QStringLiteral("Identifier"), AstType::IdentifierAstType},
        {QStringLiteral("string"), AstType::StringAstType},
        {QStringLiteral("Float"), AstType::NumberAstType},
        {QStringLiteral("Integer"), AstType::NumberAstType},
        {QStringLiteral("Bool"), AstType::IdentifierAstType},
        {QStringLiteral("Operator"), AstType::BinaryOperationAstType},
        {QStringLiteral("::"), AstType::TypeAnnotationAstType},
        {QStringLiteral("using"), AstType::UsingAstType},
        {QStringLiteral("import"), AstType::ImportAstType},
        {QStringLiteral("export"), AstType::ExportAstType},
        {QStringLiteral("="), AstType::AssignmentAstType},
        {QStringLiteral("ref"), AstType::RefAstType},
        {QStringLiteral("vect"), AstType::ListAstType},
        {QStringLiteral("generator"), AstType::GeneratorAstType},
        {QStringLiteral("comprehension"), AstType::GeneratorAstType},
        {QStringLiteral("importpath"), AstType::ImportPathAstType},
        {QStringLiteral("macro_name"), AstType::IdentifierAstType},
        {QStringLiteral(":="), AstType::AssignmentAstType},
        {QStringLiteral("."), AstType::AttributeAstType},
        {QStringLiteral(":"), AstType::ExpressionAstType},
        {QStringLiteral(";"), AstType::StatementAstType},
        {QStringLiteral(","), AstType::ExpressionAstType},
        {QStringLiteral("comment"), AstType::StatementAstType},
        {QStringLiteral("Error"), AstType::AstType},
        {QStringLiteral("global"), AstType::GlobalAstType},
        {QStringLiteral("local"), AstType::LocalAstType},
        {QStringLiteral("try"), AstType::TryAstType},
        {QStringLiteral("catch"), AstType::ExceptionHandlerAstType},
        {QStringLiteral("finally"), AstType::StatementAstType},
        {QStringLiteral("begin"), AstType::StatementAstType},
        {QStringLiteral("let"), AstType::LetAstType},
        {QStringLiteral("do"), AstType::DoAstType},
        {QStringLiteral("tuple"), AstType::TupleAstType},
        {QStringLiteral("array"), AstType::ListAstType},
        {QStringLiteral("dict"), AstType::DictAstType},
        {QStringLiteral("interpolated_string"), AstType::InterpolatedStringAstType},
        {QStringLiteral("..."), AstType::EllipsisAstType},
    };
    
    return kindMap.value(kindStr, AstType::AstType);
}

void AstTransformer::setRange(Ast* node, const QJsonObject& json)
{
    if (json.contains(QLatin1String("range"))) {
        QJsonObject rangeObj = json.value(QLatin1String("range")).toObject();
        node->startLine = rangeObj.value(QLatin1String("start_line")).toInt() - 1;
        node->endLine = rangeObj.value(QLatin1String("end_line")).toInt() - 1;
        node->startCol = rangeObj.value(QLatin1String("start_column")).toInt() - 1;
        node->endCol = rangeObj.value(QLatin1String("end_column")).toInt() - 1;
    }
}

IdentifierAst* AstTransformer::identifierFromJson(const QJsonObject& json, Ast* parent)
{
    QString text = json.value(QLatin1String("text")).toString();
    return new IdentifierAst(parent, text);
}

void AstTransformer::processPositionalArguments(FunctionDefinitionAst* fn, const QJsonObject& callObj, Ast* parent)
{
    if (!fn->arguments) {
        fn->arguments = new ArgumentsAst(parent);
    }
    ArgumentsAst* args = fn->arguments;
    
    if (!callObj.contains(QLatin1String("children"))) return;
    QJsonArray callChildren = callObj.value(QLatin1String("children")).toArray();
    
    for (int i = 0; i < callChildren.size(); ++i) {
        if (!callChildren.at(i).isObject()) continue;
        QJsonObject childObj = callChildren.at(i).toObject();
        QString childKind = childObj.value(QLatin1String("kind")).toString();
        
        if (childKind == QLatin1String("IdentifierAst")) {
            // Standalone identifier - could be name or vararg
            QString text = childObj.value(QLatin1String("text")).toString();
            // Check if next sibling is "..." (vararg)
            if (i + 1 < callChildren.size()) {
                QJsonObject nextObj = callChildren.at(i + 1).toObject();
                if (nextObj.value(QLatin1String("kind")).toString() == QLatin1String("...")) {
                    ArgAst* arg = new ArgAst(parent);
                    arg->argumentName = new IdentifierAst(parent, text);
                    setRange(arg, childObj);
                    args->vararg = arg;
                    continue;
                }
            }
            // Plain identifier without type or default - create ArgAst
            ArgAst* arg = new ArgAst(parent);
            arg->argumentName = new IdentifierAst(parent, text);
            setRange(arg, childObj);
            args->arguments.append(arg);
        }
        else if (childKind == QLatin1String("::")) {
            // Type annotation: arg::Type
            ArgAst* arg = createArgFromCallChild(childObj, parent);
            if (arg) {
                args->arguments.append(arg);
            }
        }
        else if (childKind == QLatin1String("=")) {
            // Default value: arg = value
            // Create ArgAst for the arg name (left side of =)
            QJsonArray eqChildren = childObj.value(QLatin1String("children")).toArray();
            if (!eqChildren.isEmpty()) {
                QJsonObject firstChild = eqChildren.at(0).toObject();
                QString firstKind = firstChild.value(QLatin1String("kind")).toString();
                
                if (firstKind == QLatin1String("IdentifierAst")) {
                    // arg = value - create ArgAst for the arg name
                    QString argName = firstChild.value(QLatin1String("text")).toString();
                    ArgAst* arg = new ArgAst(parent);
                    arg->argumentName = new IdentifierAst(parent, argName);
                    setRange(arg, firstChild);
                    args->arguments.append(arg);
                    
                    // Add the default value to defaultValues
                    Ast* defaultValue = fromJson(childObj, parent);
                    if (defaultValue) {
                        args->defaultValues.append(defaultValue);
                    }
                }
            }
        }
    }
}

void AstTransformer::processKeywordArguments(FunctionDefinitionAst* fn, const QJsonObject& paramsObj, Ast* parent)
{
    if (!fn->arguments) {
        fn->arguments = new ArgumentsAst(parent);
    }
    ArgumentsAst* args = fn->arguments;
    
    // Get children from the parameters node
    if (!paramsObj.contains(QLatin1String("children"))) return;
    QJsonArray paramChildren = paramsObj.value(QLatin1String("children")).toArray();
    
    // Each child is typically "=" (default assignment) for keyword args
    for (const QJsonValue& childValue : paramChildren) {
        if (!childValue.isObject()) continue;
        QJsonObject childObj = childValue.toObject();
        QString childKind = childObj.value(QLatin1String("kind")).toString();
        
        if (childKind == QLatin1String("=")) {
            // Keyword argument with default value
            QJsonArray eqChildren = childObj.value(QLatin1String("children")).toArray();
            if (eqChildren.isEmpty()) continue;
            
            // First child is the arg (could be :: or IdentifierAst)
            QJsonObject firstChild = eqChildren.at(0).toObject();
            QString firstKind = firstChild.value(QLatin1String("kind")).toString();
            
            ArgAst* arg = new ArgAst(parent);
            
            if (firstKind == QLatin1String("::")) {
                // Has type annotation: arg::Type = value
                // Create ArgAst with annotation
                arg = createArgFromCallChild(firstChild, parent);
                args->kwonlyargs.append(arg);
                
                // Add default value
                Ast* defaultValue = fromJson(childObj, parent);
                if (defaultValue) {
                    args->defaultValues.append(defaultValue);
                }
            }
            else if (firstKind == QLatin1String("IdentifierAst")) {
                // No type annotation: arg = value
                QString argName = firstChild.value(QLatin1String("text")).toString();
                arg->argumentName = new IdentifierAst(parent, argName);
                setRange(arg, firstChild);
                args->kwonlyargs.append(arg);
                
                // Add default value
                Ast* defaultValue = fromJson(childObj, parent);
                if (defaultValue) {
                    args->defaultValues.append(defaultValue);
                }
            }
        }
        else if (childKind == QLatin1String("IdentifierAst")) {
            // Keyword-only arg without default
            QString argName = childObj.value(QLatin1String("text")).toString();
            ArgAst* arg = new ArgAst(parent);
            arg->argumentName = new IdentifierAst(parent, argName);
            setRange(arg, childObj);
            args->kwonlyargs.append(arg);
        }
    }
}

ArgAst* AstTransformer::createArgFromCallChild(const QJsonObject& childObj, Ast* parent)
{
    ArgAst* arg = new ArgAst(parent);
    
    if (childObj.contains(QLatin1String("children"))) {
        QJsonArray children = childObj.value(QLatin1String("children")).toArray();
        for (int i = 0; i < children.size(); ++i) {
            if (!children.at(i).isObject()) continue;
            QJsonObject child = children.at(i).toObject();
            QString kind = child.value(QLatin1String("kind")).toString();
            
            if (kind == QLatin1String("IdentifierAst")) {
                QString name = child.value(QLatin1String("text")).toString();
                arg->argumentName = new IdentifierAst(parent, name);
                setRange(arg, child);
            }
            else if (kind == QLatin1String("IdentifierAst") && i == 0) {
                // First child is the arg name
                QString name = child.value(QLatin1String("text")).toString();
                arg->argumentName = new IdentifierAst(parent, name);
            }
            else if (kind == QLatin1String("IdentifierAst") && i > 0) {
                // Second child is type annotation
                arg->annotation = fromJson(child, parent);
            }
        }
    }
    
    return arg;
}

} // namespace Julia
