#ifndef JULIA_ASTTRANSFORMER_H
#define JULIA_ASTTRANSFORMER_H

#include "ast.h"

#include <QList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Julia {

class Ast;
class IdentifierAst;
class CodeAst;

class AstTransformer
{
public:
    CodeAst* parse(const QByteArray& json);

private:
    Ast* fromJson(const QJsonObject& json, Ast* parent = nullptr);
    
    AstType stringToAstType(const QString& kindStr);
    void setRange(Ast* node, const QJsonObject& json);
    
    IdentifierAst* identifierFromJson(const QJsonObject& json, Ast* parent);
    
    // Helper functions for proper argument categorization
    void processPositionalArguments(FunctionDefinitionAst* fn, const QJsonObject& callObj, Ast* parent);
    void processKeywordArguments(FunctionDefinitionAst* fn, const QJsonObject& paramsObj, Ast* parent);
    ArgAst* createArgFromCallChild(const QJsonObject& childObj, Ast* parent);
};

} // namespace Julia

#endif // JULIA_ASTTRANSFORMER_H
