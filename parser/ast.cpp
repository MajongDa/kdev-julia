#include "juliadebug.h"
#include "ast.h"
#include "juliaastdefaultvisitor.h"

namespace Julia {


static void dumpNode(QString &r, QString prefix, const Ast* node)
{
    r.append(prefix);
    r.append(node ? node->dump(): QStringLiteral("None"));
}

template<class T>
static void dumpList(QString &r, QString prefix, const T list,QString sep=QStringLiteral(", "))
{
    int i = 0;
    r.append(prefix);
    r.append(QLatin1Char('['));
    for (const Ast* node : list) {
        i += 1;
        dumpNode(r, QString(), node);
        if (i < list.size())
            r.append(sep);
    }
    r.append(QLatin1Char(']'));
}

// Ast implementation
Ast::Ast(Ast* parent, AstType type)
    : parent(parent), astType(type)
{
}

// Ast::~Ast() = default;
Ast::~Ast() { qCDebug(KDEV_JULIA) << "hello from base class";}

// IdentifierAst implementation
IdentifierAst::IdentifierAst(Ast* parent, const QString& value)
    : Ast(parent, AstType::IdentifierAstType), value(value)
{
    if (parent) {
        startLine = parent->startLine;
        startCol = parent->startCol;
        endLine = parent->endLine;
        endCol = parent->startCol + value.length() - 1;
    }
}

QString IdentifierAst::dump() const
{
    return QStringLiteral("IdentifierAst(value='") + value + QStringLiteral("')");
}


// Statement implementations
FunctionDefinitionAst::FunctionDefinitionAst(Ast* parent)
    : StatementAst(parent, AstType::FunctionDefinitionAstType), name(nullptr),
      arguments(nullptr), returns(nullptr)
{
}

QString FunctionDefinitionAst::dump() const
{
    QString r = QStringLiteral("FunctionDefinition(name=");
    if (name) r += name->dump();
    r += QStringLiteral(", body=[%1])").arg(body.size());
    return r;
}

ReturnAst::ReturnAst(Ast* parent)
    : StatementAst(parent, AstType::ReturnAstType)
{
}

QString ReturnAst::dump() const
{
    QString r = QStringLiteral("Return(");
    if (value) r += value->dump();
    r += QStringLiteral(")");
    return r;
}

AssignmentAst::AssignmentAst(Ast* parent)
    : StatementAst(parent, AstType::AssignmentAstType)
{
}

QString AssignmentAst::dump() const
{
    QString r = QStringLiteral("Assignment(targets=[%1], value=").arg(targets.size());
    if (value) r += value->dump();
    r += QStringLiteral(")");
    return r;
}

ForAst::ForAst(Ast* parent)
    : StatementAst(parent, AstType::ForAstType)
{
}

QString ForAst::dump() const
{
    return QStringLiteral("For(target=%1, iter=%2, body=[%3])")
        .arg(target ? target->dump() : QStringLiteral("null"))
        .arg(iterator ? iterator->dump() : QStringLiteral("null"))
        .arg(body.size());
}

WhileAst::WhileAst(Ast* parent)
    : StatementAst(parent, AstType::WhileAstType)
{
}

QString WhileAst::dump() const
{
    return QStringLiteral("While(condition=%1, body=[%2])")
        .arg(condition ? condition->dump() : QStringLiteral("null"))
        .arg(body.size());
}

IfAst::IfAst(Ast* parent)
    : StatementAst(parent, AstType::IfAstType)
{
}

QString IfAst::dump() const
{
    return QStringLiteral("If(condition=%1, body=[%2], orelse=[%3])")
        .arg(condition ? condition->dump() : QStringLiteral("null"))
        .arg(body.size())
        .arg(orelse.size());
}

TryAst::TryAst(Ast* parent)
    : StatementAst(parent, AstType::TryAstType)
{
}

QString TryAst::dump() const
{
    return QStringLiteral("Try(body=[%1], handlers=[%2], finally=[%3])")
        .arg(body.size())
        .arg(handlers.size())
        .arg(finally.size());
}

ImportAst::ImportAst(Ast* parent)
    : StatementAst(parent, AstType::ImportAstType)
{
}

QString ImportAst::dump() const
{
    return QStringLiteral("Import(module=%1, names=[%2])").arg(module).arg(names.size());
}

GlobalAst::GlobalAst(Ast* parent)
    : StatementAst(parent, AstType::GlobalAstType)
{
}

QString GlobalAst::dump() const
{
    return QStringLiteral("Global(names=[%1])").arg(names.size());
}

BreakAst::BreakAst(Ast* parent)
    : StatementAst(parent, AstType::BreakAstType)
{
}

ContinueAst::ContinueAst(Ast* parent)
    : StatementAst(parent, AstType::ContinueAstType)
{
}

// Expression implementations
ExpressionAst::ExpressionAst(Ast* parent, AstType type)
    : Ast(parent, type), context(Context::Load)
{
}

CallAst::CallAst(Ast* parent)
    : ExpressionAst(parent, AstType::CallAstType)
{
}

QString CallAst::dump() const
{
    QString r = QStringLiteral("Call(function=");
    if (function) r += function->dump();
    r += QStringLiteral(", args=[%1])").arg(arguments.size());
    return r;
}

BinaryOperationAst::BinaryOperationAst(Ast* parent)
    : ExpressionAst(parent, AstType::BinaryOperationAstType), op(Operator::Add)
{
}

QString BinaryOperationAst::dump() const
{
    return QStringLiteral("BinaryOp(left=%1, right=%2)")
        .arg(left ? left->dump() : QStringLiteral("null"))
        .arg(right ? right->dump() : QStringLiteral("null"));
}

UnaryOperationAst::UnaryOperationAst(Ast* parent)
    : ExpressionAst(parent, AstType::UnaryOperationAstType), op(Operator::UAdd)
{
}

QString UnaryOperationAst::dump() const
{
    return QStringLiteral("UnaryOp(operand=%1)").arg(operand ? operand->dump() : QStringLiteral("null"));
}

NumberAst::NumberAst(Ast* parent, AstType type)
    : ExpressionAst(parent, type), isInt(false)
{
}

QString NumberAst::dump() const
{
    return QStringLiteral("Number(value=%1, isInt=%2)").arg(value).arg(isInt);
}

StringAst::StringAst(Ast* parent, AstType type)
    : ExpressionAst(parent, type)
{
}

ListAst::ListAst(Ast* parent)
    : ExpressionAst(parent, AstType::ListAstType)
{
}

QString ListAst::dump() const
{
    return QStringLiteral("List(elements=[%1])").arg(elements.size());
}

TupleAst::TupleAst(Ast* parent)
    : ExpressionAst(parent, AstType::TupleAstType)
{
}

QString TupleAst::dump() const
{
    return QStringLiteral("Tuple(elements=[%1])").arg(elements.size());
}

DictAst::DictAst(Ast* parent)
    : ExpressionAst(parent, AstType::DictAstType)
{
}

QString DictAst::dump() const
{
    return QStringLiteral("Dict(keys=[%1], values=[%2])").arg(keys.size()).arg(values.size());
}

SubscriptAst::SubscriptAst(Ast* parent)
    : ExpressionAst(parent, AstType::SubscriptAstType)
{
}

QString SubscriptAst::dump() const
{
    return QStringLiteral("Subscript(value=%1, slice=%2)")
        .arg(value ? value->dump() : QStringLiteral("null"))
        .arg(slice ? slice->dump() : QStringLiteral("null"));
}

AttributeAst::AttributeAst(Ast* parent)
    : ExpressionAst(parent, AstType::AttributeAstType), depth(0)
{
}

QString AttributeAst::dump() const
{
    return QStringLiteral("Attribute(value=%1, attr=%2)")
        .arg(value ? value->dump() : QStringLiteral("null"))
        .arg(attribute ? attribute->dump() : QStringLiteral("null"));
}

StarredAst::StarredAst(Ast* parent)
    : ExpressionAst(parent, AstType::StarredAstType)
{
}

QString StarredAst::dump() const
{
    return QStringLiteral("Starred(value=%1)").arg(value ? value->dump() : QStringLiteral("null"));
}

LambdaAst::LambdaAst(Ast* parent)
    : ExpressionAst(parent, AstType::LambdaAstType)
{
}

QString LambdaAst::dump() const
{
    return QStringLiteral("Lambda(args=%1, body=%2)")
        .arg(arguments ? QStringLiteral("...") : QStringLiteral("null"))
        .arg(body ? QStringLiteral("...") : QStringLiteral("null"));
}

IfExpressionAst::IfExpressionAst(Ast* parent)
    : ExpressionAst(parent, AstType::IfExpressionAstType)
{
}

QString IfExpressionAst::dump() const
{
    return QStringLiteral("IfExp(test=%1, body=%2, orelse=%3)")
        .arg(condition ? condition->dump() : QStringLiteral("null"))
        .arg(body ? body->dump() : QStringLiteral("null"))
        .arg(orelse ? orelse->dump() : QStringLiteral("null"));
}


// Pattern implementations


MatchCaseAst::MatchCaseAst(Ast* parent)
    : Ast(parent, AstType::PatternAstType)
{
}

QString MatchCaseAst::dump() const
{
    return QStringLiteral("MatchCase(pattern=%1, guard=%2, body=[%3])")
        .arg(pattern ? QStringLiteral("...") : QStringLiteral("null"))
        .arg(guard ? QStringLiteral("...") : QStringLiteral("null"))
        .arg(body.size());
}

MatchAst::MatchAst(Ast* parent)
    : Ast(parent, AstType::PatternAstType)
{
}

QString MatchAst::dump() const
{
    return QStringLiteral("Match(subject=%1, cases=[%2])")
        .arg(subject ? subject->dump() : QStringLiteral("null"))
        .arg(cases.size());
}


// Other AST implementations


ArgumentsAst::ArgumentsAst(Ast* parent)
    : Ast(parent, AstType::StatementAstType)
{
}

QString ArgumentsAst::dump() const
{
    return QStringLiteral("Arguments(args=[%1], kwonly=[%2])")
        .arg(arguments.size())
        .arg(kwonlyargs.size());
}

ArgAst::ArgAst(Ast* parent)
    : Ast(parent, AstType::ArgAstType)
{
}

QString ArgAst::dump() const
{
    QString r = QStringLiteral("Arg(name=");
    if (argumentName) r += argumentName->dump();
    r += QStringLiteral(")");
    return r;
}

KeywordAst::KeywordAst(Ast* parent)
    : Ast(parent, AstType::StatementAstType)
{
}

QString KeywordAst::dump() const
{
    QString r = QStringLiteral("Keyword(arg=");
    if (argumentName) r += argumentName->dump();
    r += QStringLiteral(", value=%1)");
    return r;
}

AliasAst::AliasAst(Ast* parent)
    : Ast(parent, AstType::StatementAstType)
{
}

QString AliasAst::dump() const
{
    QString r = QStringLiteral("Alias(name=");
    if (name) r += name->dump();
    r += QStringLiteral(", as=");
    if (asName) r += asName->dump();
    r += QStringLiteral(")");
    return r;
}

ExceptionHandlerAst::ExceptionHandlerAst(Ast* parent)
    : Ast(parent, AstType::StatementAstType)
{
}

QString ExceptionHandlerAst::dump() const
{
    return QStringLiteral("ExceptionHandler(type=%1, name=%2, body=[%3])")
        .arg(type ? QStringLiteral("...") : QStringLiteral("null"))
        .arg(name ? name->dump() : QStringLiteral("null"))
        .arg(body.size());
}

ComprehensionAst::ComprehensionAst(Ast* parent)
    : Ast(parent, AstType::ExpressionAstType)
{
}

QString ComprehensionAst::dump() const
{
    return QStringLiteral("Comprehension(target=%1, iter=%2)")
        .arg(target ? target->dump() : QStringLiteral("null"))
        .arg(iterator ? iterator->dump() : QStringLiteral("null"));
}

SliceAst::SliceAst(Ast* parent)
    : ExpressionAst(parent, AstType::SliceAstType)
{
}

QString SliceAst::dump() const
{
    return QStringLiteral("Slice(lower=%1, upper=%2, step=%3)")
        .arg(lower ? lower->dump() : QStringLiteral("null"))
        .arg(upper ? upper->dump() : QStringLiteral("null"))
        .arg(step ? step->dump() : QStringLiteral("null"));
}


// CodeAst implementation


CodeAst::CodeAst()
    : Ast(nullptr, AstType::TopLevelAstType), name(nullptr)
{
}

CodeAst::~CodeAst()
{
    free_ast_recursive(this);
}

QString CodeAst::dump() const
{
    QString r;
    r.append(QStringLiteral("Module("));
    dumpNode(r, QStringLiteral("name="), name);
    dumpList(r, QStringLiteral(", body="), body, QStringLiteral(",\n  "));
    r.append(QLatin1Char(')'));
    return r;
    return QStringLiteral("Code(body=[%1])").arg(body.size());
}


// Julia-specific statement implementations


ModuleAst::ModuleAst(Ast* parent)
    : StatementAst(parent, AstType::ModuleAstType)
{
}

QString ModuleAst::dump() const
{
    QString r = QStringLiteral("Module(name=");
    if (name) r += name->dump();
    r += QStringLiteral(", body=[%1])").arg(body.size());
    return r;
}

BaremoduleAst::BaremoduleAst(Ast* parent)
    : StatementAst(parent, AstType::BaremoduleAstType)
{
}

QString BaremoduleAst::dump() const
{
    QString r = QStringLiteral("Baremodule(name=");
    if (name) r += name->dump();
    r += QStringLiteral(", body=[%1])").arg(body.size());
    return r;
}

StructAst::StructAst(Ast* parent)
    : StatementAst(parent, AstType::StructAstType)
{
}

QString StructAst::dump() const
{
    QString r = QStringLiteral("Struct(mutable=%1, name=").arg(isMutable ? "true" : "false");
    if (name) r += name->dump();
    r += QStringLiteral(", body=[%1])").arg(body.size());
    return r;
}

AbstractAst::AbstractAst(Ast* parent)
    : StatementAst(parent, AstType::AbstractAstType)
{
}

QString AbstractAst::dump() const
{
    QString r = QStringLiteral("Abstract(name=");
    if (name) r += name->dump();
    r += QStringLiteral(")");
    return r;
}

PrimitiveAst::PrimitiveAst(Ast* parent)
    : StatementAst(parent, AstType::PrimitiveAstType)
{
}

QString PrimitiveAst::dump() const
{
    QString r = QStringLiteral("Primitive(name=");
    if (name) r += name->dump();
    r += QStringLiteral(")");
    return r;
}

MacroAst::MacroAst(Ast* parent)
    : StatementAst(parent, AstType::MacroAstType)
{
}

QString MacroAst::dump() const
{
    QString r = QStringLiteral("Macro(name=");
    if (name) r += name->dump();
    r += QStringLiteral(", body=[%1])").arg(body.size());
    return r;
}

UsingAst::UsingAst(Ast* parent)
    : StatementAst(parent, AstType::UsingAstType)
{
}

QString UsingAst::dump() const
{
    return QStringLiteral("Using(names=[%1])").arg(names.size());
}

ExportAst::ExportAst(Ast* parent)
    : StatementAst(parent, AstType::ExportAstType)
{
}

QString ExportAst::dump() const
{
    return QStringLiteral("Export(names=[%1])").arg(names.size());
}

ConstAst::ConstAst(Ast* parent)
    : StatementAst(parent, AstType::ConstAstType)
{
}

QString ConstAst::dump() const
{
    QString r = QStringLiteral("Const(target=");
    if (target) r += target->dump();
    r += QStringLiteral(")");
    return r;
}

LocalAst::LocalAst(Ast* parent)
    : StatementAst(parent, AstType::LocalAstType)
{
}

QString LocalAst::dump() const
{
    return QStringLiteral("Local(names=[%1])").arg(names.size());
}

LetAst::LetAst(Ast* parent)
    : StatementAst(parent, AstType::LetAstType)
{
}

QString LetAst::dump() const
{
    return QStringLiteral("Let(bindings=[%1], body=[%2])")
        .arg(bindings.size())
        .arg(body.size());
}

DoAst::DoAst(Ast* parent)
    : StatementAst(parent, AstType::DoAstType)
{
}

QString DoAst::dump() const
{
    return QStringLiteral("Do(call=%1, body=[%2])")
        .arg(call ? QStringLiteral("...") : QStringLiteral("null"))
        .arg(body.size());
}


// Julia-specific expression implementations


ParameterAst::ParameterAst(Ast* parent)
    : ExpressionAst(parent, AstType::ParameterAstType)
{
}

QString ParameterAst::dump() const
{
    QString r = QStringLiteral("Parameter(name=");
    if (name) r += name->dump();
    r += QStringLiteral(")");
    return r;
}

TypeAnnotationAst::TypeAnnotationAst(Ast* parent)
    : ExpressionAst(parent, AstType::TypeAnnotationAstType)
{
}

QString TypeAnnotationAst::dump() const
{
    return QStringLiteral("TypeAnnotation(value=%1, type=%2)")
        .arg(value ? value->dump() : QStringLiteral("null"))
        .arg(type ? type->dump() : QStringLiteral("null"));
}

CurlyAst::CurlyAst(Ast* parent)
    : ExpressionAst(parent, AstType::CurlyAstType)
{
}

QString CurlyAst::dump() const
{
    return QStringLiteral("Curly(name=%1, params=[%2])")
        .arg(name ? name->dump() : QStringLiteral("null"))
        .arg(parameters.size());
}

ImportPathAst::ImportPathAst(Ast* parent)
    : ExpressionAst(parent, AstType::ImportPathAstType)
{
}

QString ImportPathAst::dump() const
{
    return QStringLiteral("ImportPath(names=[%1], as=%2)")
        .arg(names.size())
        .arg(asName ? asName->dump() : QStringLiteral("null"));
}

GeneratorAst::GeneratorAst(Ast* parent)
    : ExpressionAst(parent, AstType::GeneratorAstType)
{
}

QString GeneratorAst::dump() const
{
    return QStringLiteral("Generator(expr=%1, iter=%2)")
        .arg(expression ? expression->dump() : QStringLiteral("null"))
        .arg(iterator ? iterator->dump() : QStringLiteral("null"));
}

InterpolatedStringAst::InterpolatedStringAst(Ast* parent)
    : ExpressionAst(parent, AstType::InterpolatedStringAstType)
{
}

QString InterpolatedStringAst::dump() const
{
    return QStringLiteral("InterpolatedString(parts=[%1])").arg(parts.size());
}

MacroCallAst::MacroCallAst(Ast* parent)
    : ExpressionAst(parent, AstType::MacroCallAstType)
{
}

QString MacroCallAst::dump() const
{
    QString r = QStringLiteral("MacroCall(name=");
    if (name) r += name->dump();
    r += QStringLiteral(", args=[%1])").arg(arguments.size());
    return r;
}

RefAst::RefAst(Ast* parent)
    : ExpressionAst(parent, AstType::RefAstType)
{
}

QString RefAst::dump() const
{
    return QStringLiteral("Ref(value=%1, indices=[%2])")
        .arg(value ? value->dump() : QStringLiteral("null"))
        .arg(indices.size());
}

KwArgAst::KwArgAst(Ast* parent)
    : ExpressionAst(parent, AstType::KwArgAstType)
{
}

QString KwArgAst::dump() const
{
    QString r = QStringLiteral("KwArg(key=");
    if (key) r += key->dump();
    r += QStringLiteral(", value=%1)").arg(value ? value->dump() : QStringLiteral("null"));
    return r;
}

} // namespace Julia
