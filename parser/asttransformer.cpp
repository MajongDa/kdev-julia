#include "asttransformer.h"

#include <QJsonDocument>
#include <QDebug>

#include "juliadebug.h"

namespace Julia {

TopLevelAst* AstTransformer::parse(const QByteArray& json)
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
    
    TopLevelAst* code = new TopLevelAst();
    QJsonObject root = doc.object();
    
    setRange(code, root);

    if (root.contains(QLatin1String("children"))) {
        QJsonArray block = root.value(QLatin1String("children")).toArray();
        qCDebug(KDEV_JULIA()) << block;
        for (const QJsonValue& item : block) {
            if (item.isObject()) {
                Ast* child = fromJson(item.toObject(), code);
                if (child) {
                    code->children.append(child);
                }
            }
        }
    }

    return code;
}

FunctionSignatureAst* AstTransformer::parseFunctionSignature(Ast* rawNode, Ast* parent)
{
    auto* sig = new FunctionSignatureAst(parent);
    sig->rawSignature = rawNode;  // Preserve nested for ContextBuilder

    Ast* current = rawNode;
    while (current) {
        switch (current->astType) {
            case AstType::CallAstType: {
                auto* call = static_cast<CallAst*>(current);
                sig->name = call->name;
                // arguments[] excludes name (stored separately in call->name)
                for (auto* arg : call->arguments) {
                    if (arg->astType == AstType::ParameterAstType)
                        sig->keywordArgs.append(arg);
                    else
                        sig->positionalArgs.append(arg);
                }
                return sig;
            }
            case AstType::TypeAnnotationAstType: {
                auto* ann = static_cast<TypeAnnotationAst*>(current);
                sig->returnType = ann->type;
                current = ann->value;  // Unwrap to inner call
                break;
            }
            case AstType::WhereAstType: {
                auto* where = static_cast<WhereAst*>(current);
                // Braces already unwrapped during JSON parsing
                // Prepend in reverse to maintain original constraint order
                for (int i = where->constraints.size() - 1; i >= 0; --i) {
                    sig->whereConstraints.prepend(where->constraints[i]);
                }
                current = where->signature;
                break;
            }
            case AstType::TupleAstType: {
                // Anonymous function(a,b): rawSignature is a TupleAst with elements a, b
                auto* tuple = static_cast<TupleAst*>(current);
                sig->positionalArgs = tuple->elements;
                return sig;
            }
            case AstType::IdentifierAstType: {
                // Anonymous function(a): rawSignature is a single IdentifierAst
                sig->positionalArgs.append(current);
                return sig;
            }
            default:
                return sig;
        }
    }
    return sig;
}

Ast* AstTransformer::fromJson(const QJsonObject& json, Ast* parent)
{
    QString kindStr = json.value(QLatin1String("kind")).toString();
    AstType type = stringToAstType(kindStr);
    
    Ast* node = nullptr;
    
    switch (type) {
        case AstType::BlockAstType: {
            BlockAst* block = new BlockAst(parent, type);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            for(const QJsonValue& child: children){
                block->block.append(fromJson(child.toObject(), block));
            }
            setRange(block, json);
            return block;
        }
        case AstType::FunctionDefinitionAstType: {
            FunctionDefinitionAst* function = new FunctionDefinitionAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            function->raw = fromJson(children[0].toObject(), function);

            function->signature = parseFunctionSignature(function->raw, function);
            // function->signature = static_cast<FunctionSignatureAst*>();

            function->block = static_cast<BlockAst*>(fromJson(children[1].toObject(), function));
            setRange(function, json);
            return function;
        }
        case AstType::StructAstType: {
            StructAst* structNode = new StructAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = signature, children[1] = block (block)
            structNode->signature = fromJson(children[0].toObject(), structNode);
            structNode->block = static_cast<BlockAst*>(fromJson(children[1].toObject(), structNode));
            setRange(structNode, json);
            return structNode;
        }
        case AstType::ModuleAstType: {
            ModuleAst* moduleNode = new ModuleAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            moduleNode->name = static_cast<IdentifierAst*>(fromJson(children[0].toObject(), moduleNode));
            // children[0] = name, children[1] = block (block)
            moduleNode->block = static_cast<BlockAst*>(fromJson(children[1].toObject(), moduleNode));
            setRange(moduleNode, json);
            return moduleNode;
        }
        case AstType::BaremoduleAstType: {
            BaremoduleAst* moduleNode = new BaremoduleAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = name, children[1] = block (block)
            moduleNode->name = static_cast<IdentifierAst*>(fromJson(children[0].toObject(), moduleNode));
            moduleNode->block = static_cast<BlockAst*>(fromJson(children[1].toObject(), moduleNode));
            setRange(moduleNode, json);
            return moduleNode;
        }
        case AstType::AbstractAstType: {
            AbstractAst* abstractNode = new AbstractAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = signature (Identifier, Curly, or Where)
            if (!children.isEmpty()) {
                abstractNode->signature = fromJson(children[0].toObject(), abstractNode);
            }
            setRange(abstractNode, json);
            return abstractNode;
        }
        case AstType::PrimitiveAstType: {
            PrimitiveAst* primNode = new PrimitiveAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = signature (call-like with name and bit count)
            if (!children.isEmpty()) {
                primNode->signature = fromJson(children[0].toObject(), primNode);
            }
            setRange(primNode, json);
            return primNode;
        }
        case AstType::MacroAstType: {
            MacroAst* macroNode = new MacroAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = signature (call), children[1] = block (block)
            if (children.size() >= 2) {
                macroNode->signature = fromJson(children[0].toObject(), macroNode);
                macroNode->block = static_cast<BlockAst*>(fromJson(children[1].toObject(), macroNode));
            }
            setRange(macroNode, json);
            return macroNode;
        }
        case AstType::CallAstType: {
            CallAst* call = new CallAst(parent);
            // Should contain at least one children - call itself
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            call->name = static_cast<IdentifierAst*>(fromJson(children[0].toObject(), call));
            // children[1..] = arguments
            for (int i = 1; i < children.size(); ++i) {
                call->arguments.append(
                    fromJson(children[i].toObject(), call));
            }
            setRange(call, json);
            return call;
            }
        case AstType::AssignmentAstType: {
            AssignmentAst* value = new AssignmentAst(parent, type);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            value->target = fromJson(children[0].toObject(), value); // lhs
            value->value = fromJson(children[1].toObject(), value); // rhs
            setRange(value, json);
            return value;
        }
        case AstType::CompoundAssignmentAstType: {
            CompoundAssignmentAst* node = new CompoundAssignmentAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            node->target = fromJson(children[0].toObject(), node);     // x
            node->compoundOperator = fromJson(children[1].toObject(), node);  // +
            node->value = fromJson(children[2].toObject(), node);     // 1
            setRange(node, json);
            return node;
        }
        case AstType::ReturnAstType: {
            ReturnAst* value = new ReturnAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            if (children.size()) {
                value->value = fromJson(children[0].toObject(), value);
            }
            setRange(value, json);
            return value;
        }
        case AstType::IfAstType: {
            IfAst* ifNode = new IfAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = condition, children[1] = if-block (block), children[2] = else block/elseif
            ifNode->condition = fromJson(children[0].toObject(), ifNode);
            ifNode->block = static_cast<BlockAst*>(fromJson(children[1].toObject(), ifNode));
            if (children.size() > 2) {
                ifNode->orelse = fromJson(children[2].toObject(), ifNode);
            }
            setRange(ifNode, json);
            return ifNode;
        }
        case AstType::ForAstType: {
            ForAst* forNode = new ForAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = iterator (in node), children[1] = block (block)
            if (children.size() >= 2) {
                forNode->iterator = fromJson(children[0].toObject(), forNode);
                forNode->block = static_cast<BlockAst*>(fromJson(children[1].toObject(), forNode));
            }
            setRange(forNode, json);
            return forNode;
        }
        case AstType::WhileAstType: {
            WhileAst* whileNode = new WhileAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = condition, children[1] = block (block)
            whileNode->condition = fromJson(children[0].toObject(), whileNode);
            whileNode->block = static_cast<BlockAst*>(fromJson(children[1].toObject(), whileNode));
            setRange(whileNode, json);
            return whileNode;
        }
        case AstType::TryAstType: {
            TryAst* tryNode = new TryAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = try-block, children[1] = catch handlers, children[2] = finally
            for (const QJsonValue& childValue: children){
                const QJsonObject& child = childValue.toObject();
                QString kind = child.value(QLatin1String("kind")).toString();
                if (stringToAstType(kind) == AstType::BlockAstType)
                    // children[0] = try block (required)
                    tryNode->block = static_cast<BlockAst*>(fromJson(child, tryNode));
                if (stringToAstType(kind) == AstType::CatchAstType)
                     // children[1] = catch (optional)
                    tryNode->handler = static_cast<CatchAst*>(fromJson(child, tryNode));
                if (stringToAstType(kind) == AstType::ElseAstType)
                    // children[2] = else (optional, Julia 1.8+)
                    tryNode->orelse = static_cast<BlockAst*>(fromJson(child, tryNode));
                if (stringToAstType(kind) == AstType::FinallyAstType)
                    // children[3] = finally (optional)
                    tryNode->finally = static_cast<BlockAst*>(fromJson(child, tryNode));
                }
            setRange(tryNode, json);
            return tryNode;
        }
        case AstType::TupleAstType: {
            TupleAst* tuple = new TupleAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            for (const QJsonValue& child : children) {
                tuple->elements.append(fromJson(child.toObject(), tuple));
            }
            setRange(tuple, json);
            return tuple;
        }
        case AstType::ListAstType: {
            ListAst* list = new ListAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            for (const QJsonValue& child : children) {
                list->elements.append(fromJson(child.toObject(), list));
            }
            setRange(list, json);
            return list;
        }
        case AstType::DictAstType: {
            DictAst* dict = new DictAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // Children alternate: key1, value1, key2, value2, ...
            for (int i = 0; i < children.size(); ++i) {
                if (i % 2) {
                    dict->values.append(fromJson(children[i].toObject(), dict));
                } else {
                    dict->keys.append(fromJson(children[i].toObject(), dict));
                }
            }
            setRange(dict, json);
            return dict;
        }
        case AstType::AttributeAstType: {
            AttributeAst* attr = new AttributeAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = value, children[1] = attribute
            attr->value = fromJson(children[0].toObject(), attr);
            attr->attribute = static_cast<IdentifierAst*>(fromJson(children[1].toObject(), attr));
            setRange(attr, json);
            return attr;
        }
        case AstType::StarredAstType: {
            StarredAst* star = new StarredAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            if (!children.isEmpty()) {
                star->value = fromJson(children[0].toObject(), star);
            }
            setRange(star, json);
            return star;
        }
        case AstType::ImportPathAstType: {
            ImportPathAst* path = new ImportPathAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            for (const QJsonValue& child : children) {
                QJsonObject childObj = child.toObject();
                if (child.isObject() &&
                    stringToAstType(childObj.value(QLatin1String("kind")).toString()) == AstType::IdentifierAstType) {
                    path->names.append(static_cast<IdentifierAst*>(fromJson(child.toObject(), path)));
                }
            }
            // Check for "as" alias in parent
            setRange(path, json);
            return path;
        }
        case AstType::InterpolatedStringAstType: {
            InterpolatedStringAst* str = new InterpolatedStringAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            for (const QJsonValue& child : children) {
                if (child.isObject()) {
                    str->parts.append(fromJson(child.toObject(), str));
                }
            }
            setRange(str, json);
            return str;
        }
        case AstType::AliasAstType: {
            AliasAst* alias = new AliasAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = name, children[1] = asName
            alias->name = fromJson(children[0].toObject(), alias);
            alias->asName = static_cast<IdentifierAst*>(fromJson(children[1].toObject(), alias));
            setRange(alias, json);
            return alias;
        }
        case AstType::CatchAstType: {
            CatchAst* value = new CatchAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            value->name = static_cast<IdentifierAst*>(fromJson(children[0].toObject(), value));
            value->block = static_cast<BlockAst*>(fromJson(children[1].toObject(), value));
            setRange(value, json);
            return value;
        }
        case AstType::ComprehensionAstType: {
            ComprehensionAst* comp = new ComprehensionAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = target, children[1] = iterator, children[2..] = conditions
            comp->target = fromJson(children[0].toObject(), comp);
            comp->iterator = fromJson(children[1].toObject(), comp);
            for (int i = 2; i < children.size(); ++i) {
                if (children[i].isObject()) {
                    comp->conditions.append(fromJson(children[i].toObject(), comp));
                }
            }
            setRange(comp, json);
            return comp;
        }
        case AstType::ConstAstType: {
            ConstAst* constant = new ConstAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            if (children.size() >= 1) constant->target = fromJson(children[0].toObject(), constant);
            if (children.size() >= 2) constant->value = fromJson(children[1].toObject(), constant);
            setRange(constant, json);
            return constant;
        }
        case AstType::LocalAstType: {
            LocalAst* local = new LocalAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            for (const QJsonValue& child : children) {
                if (child.isObject()) {
                    Ast* childNode = fromJson(child.toObject(), local);
                    if (childNode->astType == AstType::IdentifierAstType) {
                        local->names.append(static_cast<IdentifierAst*>(childNode));
                    }
                }
            }
            setRange(local, json);
            return local;
        }
        case AstType::GlobalAstType: {
            GlobalAst* global = new GlobalAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            for (const QJsonValue& child : children) {
                if (child.isObject()) {
                    Ast* childNode = fromJson(child.toObject(), global);
                    if (childNode->astType == AstType::IdentifierAstType) {
                        global->names.append(static_cast<IdentifierAst*>(childNode));
                    }
                }
            }
            setRange(global, json);
            return global;
        }
        case AstType::LetAstType: {
            LetAst* let = new LetAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0..n-1] = bindings, last = block
            if (!children.isEmpty()) {
                for (int i = 0; i < children.size() - 1; ++i) {
                    if (children[i].isObject()) {
                        let->bindings.append(fromJson(children[i].toObject(), let));
                    }
                }
                // Last child is block (can be block or single statement)
                let->block.append(fromJson(children.last().toObject(), let));
            }
            setRange(let, json);
            return let;
        }
        case AstType::DoAstType: {
            DoAst* doNode = new DoAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // children[0] = call, children[1] = block
            if (children.size() >= 1) doNode->call = fromJson(children[0].toObject(), doNode);
            if (children.size() >= 2) {
                doNode->block.append(fromJson(children[1].toObject(), doNode));
            }
            setRange(doNode, json);
            return doNode;
        }
        case AstType::ParameterAstType: {
            ParameterAst* parameters = new ParameterAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            for(const QJsonValue& child: children){
                parameters->kwargs.append(fromJson(child.toObject(), parameters));
            }
            setRange(parameters, json);
            return parameters;
        }
        case AstType::EllipsisAstType: {
            EllipsisAst* ellipsis = new EllipsisAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            ellipsis->name = static_cast<IdentifierAst*>(fromJson(children[0].toObject(), ellipsis));
            setRange(ellipsis, json);
            return ellipsis;
        }
        case AstType::TypeAnnotationAstType: {
            TypeAnnotationAst* ann = new TypeAnnotationAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            if (children.size() >= 1) ann->value = fromJson(children[0].toObject(), ann);
            if (children.size() >= 2) ann->type = fromJson(children[1].toObject(), ann);
            setRange(ann, json);
            return ann;
        }
        case AstType::SubtypeAstType: {
            SubtypeAst* node = new SubtypeAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            node->left = fromJson(children[0].toObject(), node);
            node->right = fromJson(children[1].toObject(), node);
            setRange(node, json);
            return node;
        }
        case AstType::CurlyAstType: {
            CurlyAst* curly = new CurlyAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            if (!children.isEmpty()) {
                curly->name = static_cast<IdentifierAst*>(fromJson(children[0].toObject(), curly));
                for (int i = 1; i < children.size(); ++i) {
                    if (children[i].isObject()) {
                        curly->parameters.append(fromJson(children[i].toObject(), curly));
                    }
                }
            }
            setRange(curly, json);
            return curly;
        }
        case AstType::LambdaAstType: {
            LambdaAst* lambda = new LambdaAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            if (children.size() >= 1) lambda->arguments = fromJson(children[0].toObject(), lambda);
            if (children.size() >= 2) lambda->block = fromJson(children[1].toObject(), lambda);
            setRange(lambda, json);
            return lambda;
        }
        case AstType::IfExpressionAstType: {
            IfExpressionAst* ifExp = new IfExpressionAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            ifExp->condition = fromJson(children[0].toObject(), ifExp);
            ifExp->block = fromJson(children[1].toObject(), ifExp);
            if (children.size() >= 3) ifExp->orelse = fromJson(children[2].toObject(), ifExp);
            setRange(ifExp, json);
            return ifExp;
        }
        case AstType::GeneratorAstType: {
            GeneratorAst* gen = new GeneratorAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            if (children.size() >= 1) gen->expression = fromJson(children[0].toObject(), gen);
            if (children.size() >= 2) gen->iterator = fromJson(children[1].toObject(), gen);
            for (int i = 2; i < children.size(); ++i) {
                if (children[i].isObject()) {
                    gen->filters.append(fromJson(children[i].toObject(), gen));
                }
            }
            setRange(gen, json);
            return gen;
        }
        case AstType::RefAstType: {
            RefAst* ref = new RefAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            if (!children.isEmpty()) {
                ref->value = fromJson(children[0].toObject(), ref);
                for (int i = 1; i < children.size(); ++i) {
                    if (children[i].isObject()) {
                        ref->indices.append(fromJson(children[i].toObject(), ref));
                    }
                }
            }
            setRange(ref, json);
            return ref;
        }
        case AstType::MacroCallAstType: {
            MacroCallAst* macroCall = new MacroCallAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            if (!children.isEmpty()) {
                macroCall->name = static_cast<IdentifierAst*>(fromJson(children[0].toObject(), macroCall));
                for (int i = 1; i < children.size(); ++i) {
                    if (children[i].isObject()) {
                        macroCall->arguments.append(fromJson(children[i].toObject(), macroCall));
                    }
                }
            }
            setRange(macroCall, json);
            return macroCall;
        }
        // Types with no children needed (leaf nodes or handled elsewhere)
        case AstType::IdentifierAstType: {
            IdentifierAst* id = new IdentifierAst(parent, json.value(QLatin1String("text")).toString());
            setRange(id, json);
            return id;
        }
        case AstType::NumberAstType: {
            NumberAst* num = new NumberAst(parent);
            num->value = json.value(QLatin1String("text")).toString();
            // TODO wtf is this shit
            num->isInt = json.value(QLatin1String("kind")).toString() == QLatin1String("Integer");
            setRange(num, json);
            return num;
        }
        case AstType::StringAstType: {
            StringAst* str = new StringAst(parent);
            str->value = json.value(QLatin1String("text")).toString();
            setRange(str, json);
            return str;
        }
        case AstType::ImportAstType: {
            ImportAst* imp = new ImportAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // First child is typically ImportPath, rest are aliases
            for (int i = 0; i < children.size(); ++i) {
                auto* childAst = fromJson(children[i].toObject(), imp);
                if (i == 0)
                    imp->module = static_cast<ImportPathAst*>(childAst);
                else
                    imp->names.append(childAst);
            }
            setRange(imp, json);
            return imp;
        }
        case AstType::SelectiveImportAstType: {
            SelectiveImportAst* imp = new SelectiveImportAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            // TODO children[0] = module (importpath like A.B)
            QJsonObject moduleObj = children[0].toObject();
            for (int i = 0; i < children.size(); ++i) {
                auto* childAst = fromJson(children[i].toObject(), imp);
                if (i == 0)
                    imp->module = static_cast<ImportPathAst*>(childAst);
                else
                    imp->names.append(childAst);
            }
            setRange(imp, json);
            return imp;
        }
        case AstType::UsingAstType: {
            UsingAst* usingNode = new UsingAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            for (const QJsonValue& child : children) {
                if (child.isObject()) {
                    usingNode->names.append(fromJson(child.toObject(), usingNode));
                }
            }
            setRange(usingNode, json);
            return usingNode;
        }
        case AstType::ExportAstType: {
            ExportAst* exp = new ExportAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            for (const QJsonValue& child : children) {
                if (child.isObject()) {
                    Ast* childNode = fromJson(child.toObject(), exp);
                    if (childNode->astType == AstType::IdentifierAstType) {
                        exp->names.append(static_cast<IdentifierAst*>(childNode));
                    }
                }
            }
            setRange(exp, json);
            return exp;
        }
        case AstType::WhereAstType: {
            WhereAst* whereNode = new WhereAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            if (!children.isEmpty()) {
                whereNode->signature = fromJson(children[0].toObject(), whereNode);
                for (int i = 1; i < children.size(); ++i) {
                    if (children[i].isObject()) {
                        QJsonObject childObj = children[i].toObject();
                        // Unwrap braces: for where {T<:N, S<:A}, extract inner constraints
                        if (childObj.value(QLatin1String("kind")).toString() == QLatin1String("braces")) {
                            QJsonArray braceChildren = childObj.value(QLatin1String("children")).toArray();
                            for (const QJsonValue& bc : braceChildren) {
                                if (bc.isObject()) {
                                    whereNode->constraints.append(fromJson(bc.toObject(), whereNode));
                                }
                            }
                        } else {
                            whereNode->constraints.append(fromJson(childObj, whereNode));
                        }
                    }
                }
            }
            setRange(whereNode, json);
            return whereNode;
        }
        case AstType::FilterAstType: {
            FilterAst* node = new FilterAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            node->iterator = fromJson(children[0].toObject(), node);
            node->condition = fromJson(children[1].toObject(), node);
            setRange(node, json);
            return node;
        }
        case AstType::IterationAstType: {
            IterationAst* node = new IterationAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            for (const QJsonValue& child : children) {
                node->iterators.append(fromJson(child.toObject(), node));
            }
            setRange(node, json);
            return node;
        }
        case AstType::InAstType: {
            InAst* node = new InAst(parent);
            QJsonArray children = json.value(QLatin1String("children")).toArray();
            node->target = fromJson(children[0].toObject(), node);
            node->iter = fromJson(children[1].toObject(), node);
            setRange(node, json);
            setRange(node, json);
            return  node;
        }
        default:
            node = new Ast(parent, type);
            setRange(node, json);
            break;
    }
    return node;
}

AstType AstTransformer::stringToAstType(const QString& kindStr)
{
    static const QHash<QString, AstType> kindMap = {
        {QLatin1String("toplevel"), AstType::TopLevelAstType},
        {QLatin1String("block"), AstType::BlockAstType},
        {QLatin1String("function"), AstType::FunctionDefinitionAstType},
        {QLatin1String("struct"), AstType::StructAstType},
        {QLatin1String("module"), AstType::ModuleAstType},
        {QLatin1String("baremodule"), AstType::BaremoduleAstType},
        {QLatin1String("abstract"), AstType::AbstractAstType},
        {QLatin1String("primitive"), AstType::PrimitiveAstType},
        {QLatin1String("macro"), AstType::MacroAstType},
        {QLatin1String("macrocall"), AstType::MacroCallAstType},
        {QLatin1String("assignment"), AstType::AssignmentAstType},
        {QLatin1String("return"), AstType::ReturnAstType},
        {QLatin1String("if"), AstType::IfAstType},
        {QLatin1String("elseif"), AstType::IfAstType},
        {QLatin1String("while"), AstType::WhileAstType},
        {QLatin1String("for"), AstType::ForAstType},
        {QLatin1String("break"), AstType::BreakAstType},
        {QLatin1String("continue"), AstType::ContinueAstType},
        {QLatin1String("call"), AstType::CallAstType},
        {QLatin1String("curly"), AstType::CurlyAstType},
        {QLatin1String("where"), AstType::WhereAstType},
        {QLatin1String("parameters"), AstType::ParameterAstType},
        {QLatin1String("Identifier"), AstType::IdentifierAstType},
        {QLatin1String("string"), AstType::StringAstType},
        {QLatin1String("Float"), AstType::NumberAstType},
        {QLatin1String("Integer"), AstType::NumberAstType},
        {QLatin1String("Bool"), AstType::IdentifierAstType},
        {QLatin1String("using"), AstType::UsingAstType},
        {QLatin1String("import"), AstType::ImportAstType},
        {QLatin1String("export"), AstType::ExportAstType},
        {QLatin1String("="), AstType::AssignmentAstType},
        {QLatin1String("op="), AstType::CompoundAssignmentAstType},
        {QLatin1String("ref"), AstType::RefAstType},
        {QLatin1String("vect"), AstType::ListAstType},
        {QLatin1String("generator"), AstType::GeneratorAstType},
        {QLatin1String("comprehension"), AstType::GeneratorAstType},
        {QLatin1String("importpath"), AstType::ImportPathAstType},
        {QLatin1String("as"), AstType::AliasAstType},
        {QLatin1String("macro_name"), AstType::IdentifierAstType},
        {QLatin1String(":="), AstType::AssignmentAstType},
        {QLatin1String("."), AstType::AttributeAstType},
        {QLatin1String(":"), AstType::ExpressionAstType},
        {QLatin1String(";"), AstType::BlockAstType},
        {QLatin1String("=>"), AstType::ExpressionAstType},
        {QLatin1String("::"), AstType::TypeAnnotationAstType},
        {QLatin1String("<:"), AstType::SubtypeAstType},
        {QLatin1String(":>"), AstType::SubtypeAstType},
        {QLatin1String("in"), AstType::InAstType},
        {QLatin1String("isa"), AstType::ExpressionAstType},
        {QLatin1String("comment"), AstType::BlockAstType},
        {QLatin1String("finally"), AstType::BlockAstType},
        {QLatin1String("begin"), AstType::BlockAstType},
        {QLatin1String("let"), AstType::LetAstType},
        {QLatin1String("do"), AstType::DoAstType},
        {QLatin1String("tuple"), AstType::TupleAstType},
        {QLatin1String("array"), AstType::ListAstType},
        {QLatin1String("dict"), AstType::DictAstType},
        {QLatin1String("interpolated_string"), AstType::InterpolatedStringAstType},
        {QLatin1String("..."), AstType::EllipsisAstType},
        {QLatin1String("catch"), AstType::CatchAstType},
        {QLatin1String("comparison"), AstType::ExpressionAstType},  // comparison expressions
        // {QLatin1String("Operator"), AstType::BinaryOperationAstType},  // Unused - binary ops are calls in JSON
        // TODO this nodekind resolves to generic block, it may lead
        // to sketchy shit on context building stage
        {QLatin1String("else"), AstType::BlockAstType},
        {QLatin1String("filter"), AstType::FilterAstType},
        {QLatin1String("in"), AstType::InAstType},
        {QLatin1String("iteration"), AstType::IterationAstType},
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

} // namespace Julia
