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
    return QStringLiteral("Identifier(value='") + value + QStringLiteral("')");
}


// Statement implementations
FunctionDefinitionAst::FunctionDefinitionAst(Ast* parent)
    : BodyAst(parent, AstType::FunctionDefinitionAstType), callSignature(nullptr), body(nullptr)
{
}

QString FunctionDefinitionAst::dump() const
{
    QString r = QStringLiteral("FunctionDefinition(\ncallSignature=");
    dumpNode(r, QString(), callSignature);
    dumpNode(r, QStringLiteral("\nbody="), body);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")\n"));

    return r;
}

ReturnAst::ReturnAst(Ast* parent)
    : BodyAst(parent, AstType::ReturnAstType)
{
}

QString ReturnAst::dump() const
{
    QString r = QStringLiteral("Return(\n");
    dumpNode(r, QStringLiteral("value="), value);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

AssignmentAst::AssignmentAst(Ast* parent)
    : BodyAst(parent, AstType::AssignmentAstType)
{
}

QString AssignmentAst::dump() const
{
    QString r = QStringLiteral("Assignment(\n");
    dumpList(r, QStringLiteral("targets="), targets, QStringLiteral(",\n"));
    dumpNode(r, QStringLiteral("\nvalue="), value);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

ForAst::ForAst(Ast* parent)
    : BodyAst(parent, AstType::ForAstType)
{
}

QString ForAst::dump() const
{
    QString r = QStringLiteral("For(\n");
    dumpNode(r, QStringLiteral("target="), target);
    dumpNode(r, QStringLiteral("\niterator="), iterator);
    dumpNode(r, QStringLiteral("\nbody="), body);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

WhileAst::WhileAst(Ast* parent)
    : BodyAst(parent, AstType::WhileAstType)
{
}

QString WhileAst::dump() const
{
    QString r = QStringLiteral("While(\n");
    dumpNode(r, QStringLiteral("condition="), condition);
    dumpNode(r, QStringLiteral("\nbody="), body);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

IfAst::IfAst(Ast* parent)
    : BodyAst(parent, AstType::IfAstType)
{
}

QString IfAst::dump() const
{
    QString r = QStringLiteral("If(\n");
    dumpNode(r, QStringLiteral("condition="), condition);
    dumpNode(r, QStringLiteral("\nbody="), body);
    dumpNode(r, QStringLiteral("\norelse="), orelse);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

TryAst::TryAst(Ast* parent)
    : BodyAst(parent, AstType::TryAstType)
{
}

QString TryAst::dump() const
{
    QString r = QStringLiteral("Try(\n");
    dumpNode(r, QStringLiteral("body="), body);
    dumpList(r, QStringLiteral("\nhandlers="), handlers, QStringLiteral(",\n  "));
    dumpNode(r, QStringLiteral("\norelse="), orelse);
    dumpNode(r, QStringLiteral("\nfinally="), finally);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

ImportAst::ImportAst(Ast* parent)
    : BodyAst(parent, AstType::ImportAstType)
{
}

QString ImportAst::dump() const
{
    QString r = QStringLiteral("Import(\n");
    r.append(QStringLiteral("module=%1,\n").arg(module));
    dumpList(r, QStringLiteral("names="), names, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

GlobalAst::GlobalAst(Ast* parent)
    : BodyAst(parent, AstType::GlobalAstType)
{
}

QString GlobalAst::dump() const
{
    QString r = QStringLiteral("Global(\n");
    dumpList(r, QStringLiteral("names="), names, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

BreakAst::BreakAst(Ast* parent)
    : BodyAst(parent, AstType::BreakAstType)
{
}

ContinueAst::ContinueAst(Ast* parent)
    : BodyAst(parent, AstType::ContinueAstType)
{
}

QString BreakAst::dump() const
{
    return QStringLiteral("Break()");
}

QString ContinueAst::dump() const
{
    return QStringLiteral("Continue()");
}

// Control flow implementations
WhereAst::WhereAst(Ast* parent)
    : Ast(parent, AstType::WhereAstType)
{
}

QString WhereAst::dump() const
{
    QString r = QStringLiteral("Where(\n");
    dumpList(r, QStringLiteral("constraints="), constraints, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

ElseIfAst::ElseIfAst(Ast* parent)
    : BodyAst(parent, AstType::ElseIfAstType)
{
}

QString ElseIfAst::dump() const
{
    QString r = QStringLiteral("ElseIf(\n");
    dumpNode(r, QStringLiteral("condition="), condition);
    dumpNode(r, QStringLiteral("\nbody="), body);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

InAst::InAst(Ast* parent)
    : Ast(parent, AstType::InAstType)
{
}

QString InAst::dump() const
{
    QString r = QStringLiteral("In(\n");
    dumpNode(r, QStringLiteral("target="), target);
    dumpNode(r, QStringLiteral("\niter="), iter);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

IterationAst::IterationAst(Ast* parent)
    : Ast(parent, AstType::IterationAstType)
{
}

QString IterationAst::dump() const
{
    QString r = QStringLiteral("Iteration(\n");
    dumpList(r, QStringLiteral("iterators="), iterators, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
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
    QString r = QStringLiteral("Call(\n");
    dumpNode(r, QStringLiteral("name="), name);
    dumpList(r, QStringLiteral("\narguments="), arguments, QStringLiteral(",\n"));
    dumpNode(r, QStringLiteral("\nvararg="), vararg);
    dumpNode(r, QStringLiteral("\nkwarg="), kwarg);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

BinaryOperationAst::BinaryOperationAst(Ast* parent)
    : ExpressionAst(parent, AstType::BinaryOperationAstType), op(Operator::Add)
{
}

QString BinaryOperationAst::dump() const
{
    QString r = QStringLiteral("BinaryOperation(\n");
    dumpNode(r, QStringLiteral("left="), left);
    dumpNode(r, QStringLiteral("\nright="), right);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

UnaryOperationAst::UnaryOperationAst(Ast* parent)
    : ExpressionAst(parent, AstType::UnaryOperationAstType), op(Operator::UAdd)
{
}

QString UnaryOperationAst::dump() const
{
    QString r = QStringLiteral("UnaryOperation(\n");
    dumpNode(r, QStringLiteral("operand="), operand);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

NumberAst::NumberAst(Ast* parent, AstType type)
    : ExpressionAst(parent, type), isInt(false)
{
}

QString NumberAst::dump() const
{
    QString r = QStringLiteral("Number(\n");
    r.append(QStringLiteral("value=%1,\n").arg(value));
    r.append(QStringLiteral("isInt=%1\n").arg(isInt ? "true" : "false"));
    r.append(QStringLiteral(")"));
    return r;
}

StringAst::StringAst(Ast* parent, AstType type)
    : ExpressionAst(parent, type)
{
}

QString StringAst::dump() const
{
    return QStringLiteral("String(value='") + value + QStringLiteral("')");
}

ListAst::ListAst(Ast* parent)
    : ExpressionAst(parent, AstType::ListAstType)
{
}

QString ListAst::dump() const
{
    QString r = QStringLiteral("List(\n");
    dumpList(r, QStringLiteral("elements="), elements, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

TupleAst::TupleAst(Ast* parent)
    : ExpressionAst(parent, AstType::TupleAstType)
{
}

QString TupleAst::dump() const
{
    QString r = QStringLiteral("Tuple(\n");
    dumpList(r, QStringLiteral("elements="), elements, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

DictAst::DictAst(Ast* parent)
    : ExpressionAst(parent, AstType::DictAstType)
{
}

QString DictAst::dump() const
{
    QString r = QStringLiteral("Dict(\n");
    dumpList(r, QStringLiteral("keys="), keys, QStringLiteral(",\n"));
    dumpList(r, QStringLiteral("\nvalues="), values, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

SubscriptAst::SubscriptAst(Ast* parent)
    : ExpressionAst(parent, AstType::SubscriptAstType)
{
}

QString SubscriptAst::dump() const
{
    QString r = QStringLiteral("Subscript(\n");
    dumpNode(r, QStringLiteral("value="), value);
    dumpNode(r, QStringLiteral("\nslice="), slice);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

AttributeAst::AttributeAst(Ast* parent)
    : ExpressionAst(parent, AstType::AttributeAstType), depth(0)
{
}

QString AttributeAst::dump() const
{
    QString r = QStringLiteral("Attribute(\n");
    dumpNode(r, QStringLiteral("value="), value);
    dumpNode(r, QStringLiteral("\nattribute="), attribute);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

StarredAst::StarredAst(Ast* parent)
    : ExpressionAst(parent, AstType::StarredAstType)
{
}

QString StarredAst::dump() const
{
    QString r = QStringLiteral("Starred(\n");
    dumpNode(r, QStringLiteral("value="), value);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

LambdaAst::LambdaAst(Ast* parent)
    : ExpressionAst(parent, AstType::LambdaAstType)
{
}

QString LambdaAst::dump() const
{
    QString r = QStringLiteral("Lambda(\n");
    dumpNode(r, QStringLiteral("arguments="), arguments);
    dumpNode(r, QStringLiteral("\nbody="), body);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

IfExpressionAst::IfExpressionAst(Ast* parent)
    : ExpressionAst(parent, AstType::IfExpressionAstType)
{
}

QString IfExpressionAst::dump() const
{
    QString r = QStringLiteral("IfExpression(\n");
    dumpNode(r, QStringLiteral("condition="), condition);
    dumpNode(r, QStringLiteral("\nbody="), body);
    dumpNode(r, QStringLiteral("\norelse="), orelse);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}


// Pattern implementations


MatchCaseAst::MatchCaseAst(Ast* parent)
    : Ast(parent, AstType::PatternAstType)
{
}

QString MatchCaseAst::dump() const
{
    QString r = QStringLiteral("MatchCase(\n");
    dumpNode(r, QStringLiteral("pattern="), pattern);
    dumpNode(r, QStringLiteral("\nguard="), guard);
    dumpList(r, QStringLiteral("\nbody="), body, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

MatchAst::MatchAst(Ast* parent)
    : Ast(parent, AstType::PatternAstType)
{
}

QString MatchAst::dump() const
{
    QString r = QStringLiteral("Match(\n");
    dumpNode(r, QStringLiteral("subject="), subject);
    dumpList(r, QStringLiteral("\ncases="), cases, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}


// Other AST implementations


ArgumentsAst::ArgumentsAst(Ast* parent)
    : Ast(parent, AstType::BodyAstType)
{
}

QString ArgumentsAst::dump() const
{
    return QStringLiteral("Arguments()");
}

ArgAst::ArgAst(Ast* parent)
    : Ast(parent, AstType::ArgAstType)
{
}

QString ArgAst::dump() const
{
    QString r = QStringLiteral("Arg(\n");
    dumpNode(r, QStringLiteral("name="), argumentName);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

KeywordAst::KeywordAst(Ast* parent)
    : Ast(parent, AstType::BodyAstType)
{
}

QString KeywordAst::dump() const
{
    QString r = QStringLiteral("Keyword(\n");
    dumpNode(r, QStringLiteral("argumentName="), argumentName);
    dumpNode(r, QStringLiteral("\nvalue="), value);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

AliasAst::AliasAst(Ast* parent)
    : Ast(parent, AstType::BodyAstType)
{
}

QString AliasAst::dump() const
{
    QString r = QStringLiteral("Alias(\n");
    dumpNode(r, QStringLiteral("name="), name);
    dumpNode(r, QStringLiteral("\nasName="), asName);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

ExceptionHandlerAst::ExceptionHandlerAst(Ast* parent)
    : Ast(parent, AstType::BodyAstType)
{
}

QString ExceptionHandlerAst::dump() const
{
    QString r = QStringLiteral("ExceptionHandler(\n");
    dumpNode(r, QStringLiteral("type="), type);
    dumpNode(r, QStringLiteral("\nname="), name);
    dumpList(r, QStringLiteral("\nbody="), body, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

ComprehensionAst::ComprehensionAst(Ast* parent)
    : Ast(parent, AstType::ExpressionAstType)
{
}

QString ComprehensionAst::dump() const
{
    QString r = QStringLiteral("Comprehension(\n");
    dumpNode(r, QStringLiteral("target="), target);
    dumpNode(r, QStringLiteral("\niterator="), iterator);
    dumpList(r, QStringLiteral("\nconditions="), conditions, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

SliceAst::SliceAst(Ast* parent)
    : ExpressionAst(parent, AstType::SliceAstType)
{
}

QString SliceAst::dump() const
{
    QString r = QStringLiteral("Slice(\n");
    dumpNode(r, QStringLiteral("lower="), lower);
    dumpNode(r, QStringLiteral("\nupper="), upper);
    dumpNode(r, QStringLiteral("\nstep="), step);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
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
    dumpList(r, QStringLiteral(", children="), children, QStringLiteral(",\n  "));
    r.append(QStringLiteral(")\n"));
    return r;
}


// Julia-specific statement implementations
BodyAst::BodyAst(Ast* parent, AstType type = AstType::BodyAstType)
    : Ast(parent, type)
{
}

QString BodyAst::dump() const
{
    QString r;
    r.append(QStringLiteral("Block("));
    dumpList(r, QStringLiteral("body="), body, QStringLiteral(",\n  "));
    r.append(QLatin1Char(')'));
    return r;
}



ModuleAst::ModuleAst(Ast* parent)
    : BodyAst(parent, AstType::ModuleAstType)
{
}

QString ModuleAst::dump() const
{
    QString r = QStringLiteral("Module(\n");
    dumpNode(r, QStringLiteral("name="), name);
    dumpNode(r, QStringLiteral("\nbody="), body);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

BaremoduleAst::BaremoduleAst(Ast* parent)
    : BodyAst(parent, AstType::BaremoduleAstType)
{
}

QString BaremoduleAst::dump() const
{
    QString r = QStringLiteral("Baremodule(\n");
    dumpNode(r, QStringLiteral("name="), name);
    dumpNode(r, QStringLiteral("\nbody="), body);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

StructAst::StructAst(Ast* parent)
    : BodyAst(parent, AstType::StructAstType)
{
}

QString StructAst::dump() const
{
    QString r = QStringLiteral("Struct(mutable=%1,\n").arg(isMutable ? "true" : "false");
    dumpNode(r, QStringLiteral("name="), name);
    dumpNode(r, QStringLiteral("\ntypeParameters="), typeParameters);
    dumpNode(r, QStringLiteral("\nbody="), body);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

AbstractAst::AbstractAst(Ast* parent)
    : BodyAst(parent, AstType::AbstractAstType)
{
}

QString AbstractAst::dump() const
{
    QString r = QStringLiteral("Abstract(\n");
    dumpNode(r, QStringLiteral("name="), name);
    dumpNode(r, QStringLiteral("\ntypeParameters="), typeParameters);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

PrimitiveAst::PrimitiveAst(Ast* parent)
    : BodyAst(parent, AstType::PrimitiveAstType)
{
}

QString PrimitiveAst::dump() const
{
    QString r = QStringLiteral("Primitive(\n");
    dumpNode(r, QStringLiteral("name="), name);
    dumpNode(r, QStringLiteral("\ntypeParameters="), typeParameters);
    dumpNode(r, QStringLiteral("\nunderlyingType="), underlyingType);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

MacroAst::MacroAst(Ast* parent)
    : BodyAst(parent, AstType::MacroAstType)
{
}

QString MacroAst::dump() const
{
    QString r = QStringLiteral("Macro(\n");
    dumpNode(r, QStringLiteral("name="), name);
    dumpNode(r, QStringLiteral("\nparameters="), parameters);
    dumpNode(r, QStringLiteral("\nbody="), body);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

UsingAst::UsingAst(Ast* parent)
    : BodyAst(parent, AstType::UsingAstType)
{
}

QString UsingAst::dump() const
{
    QString r = QStringLiteral("Using(\n");
    dumpList(r, QStringLiteral("names="), names, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

ExportAst::ExportAst(Ast* parent)
    : BodyAst(parent, AstType::ExportAstType)
{
}

QString ExportAst::dump() const
{
    QString r = QStringLiteral("Export(\n");
    dumpList(r, QStringLiteral("names="), names, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

ConstAst::ConstAst(Ast* parent)
    : BodyAst(parent, AstType::ConstAstType)
{
}

QString ConstAst::dump() const
{
    QString r = QStringLiteral("Const(\n");
    dumpNode(r, QStringLiteral("target="), target);
    dumpNode(r, QStringLiteral("\nvalue="), value);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

LocalAst::LocalAst(Ast* parent)
    : BodyAst(parent, AstType::LocalAstType)
{
}

QString LocalAst::dump() const
{
    QString r = QStringLiteral("Local(\n");
    dumpList(r, QStringLiteral("names="), names, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

LetAst::LetAst(Ast* parent)
    : BodyAst(parent, AstType::LetAstType)
{
}

QString LetAst::dump() const
{
    QString r = QStringLiteral("Let(\n");
    dumpList(r, QStringLiteral("bindings="), bindings, QStringLiteral(",\n"));
    dumpList(r, QStringLiteral("\nbody="), body, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

DoAst::DoAst(Ast* parent)
    : BodyAst(parent, AstType::DoAstType)
{
}

QString DoAst::dump() const
{
    QString r = QStringLiteral("Do(\n");
    dumpNode(r, QStringLiteral("call="), call);
    dumpList(r, QStringLiteral("\nbody="), body, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}


// Julia-specific expression implementations
EllipsisAst::EllipsisAst(Ast* parent)
    : ExpressionAst(parent, AstType::EllipsisAstType)
{
}

QString EllipsisAst::dump() const
{
    QString r = QStringLiteral("Ellipsis( ");
    dumpNode(r, QStringLiteral("name="), name);
    r.append(QStringLiteral(")\n"));
    return r;
}

ParameterAst::ParameterAst(Ast* parent)
    : ExpressionAst(parent, AstType::ParameterAstType)
{
}

QString ParameterAst::dump() const
{
    QString r = QStringLiteral("Parameter(\n");
    dumpList(r, QStringLiteral("kwargs="), kwargs, QStringLiteral(",\n"));
    dumpNode(r, QStringLiteral("\nellipsis="), ellipsis);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

TypeAnnotationAst::TypeAnnotationAst(Ast* parent)
    : ExpressionAst(parent, AstType::TypeAnnotationAstType)
{
}

QString TypeAnnotationAst::dump() const
{
    QString r = QStringLiteral("TypeAnnotation(\n");
    dumpNode(r, QStringLiteral("value="), value);
    dumpNode(r, QStringLiteral("\ntype="), type);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

CurlyAst::CurlyAst(Ast* parent)
    : ExpressionAst(parent, AstType::CurlyAstType)
{
}

QString CurlyAst::dump() const
{
    QString r = QStringLiteral("Curly(\n");
    dumpNode(r, QStringLiteral("name="), name);
    dumpList(r, QStringLiteral("\nparameters="), parameters, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

ImportPathAst::ImportPathAst(Ast* parent)
    : ExpressionAst(parent, AstType::ImportPathAstType)
{
}

QString ImportPathAst::dump() const
{
    QString r = QStringLiteral("ImportPath(\n");
    dumpList(r, QStringLiteral("names="), names, QStringLiteral(",\n"));
    dumpNode(r, QStringLiteral("\nasName="), asName);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

GeneratorAst::GeneratorAst(Ast* parent)
    : ExpressionAst(parent, AstType::GeneratorAstType)
{
}

QString GeneratorAst::dump() const
{
    QString r = QStringLiteral("Generator(\n");
    dumpNode(r, QStringLiteral("expression="), expression);
    dumpNode(r, QStringLiteral("\niterator="), iterator);
    dumpList(r, QStringLiteral("\nfilters="), filters, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

InterpolatedStringAst::InterpolatedStringAst(Ast* parent)
    : ExpressionAst(parent, AstType::InterpolatedStringAstType)
{
}

QString InterpolatedStringAst::dump() const
{
    QString r = QStringLiteral("InterpolatedString(\n");
    dumpList(r, QStringLiteral("parts="), parts, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

MacroCallAst::MacroCallAst(Ast* parent)
    : ExpressionAst(parent, AstType::MacroCallAstType)
{
}

QString MacroCallAst::dump() const
{
    QString r = QStringLiteral("MacroCall(\n");
    dumpNode(r, QStringLiteral("name="), name);
    dumpList(r, QStringLiteral("\narguments="), arguments, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

RefAst::RefAst(Ast* parent)
    : ExpressionAst(parent, AstType::RefAstType)
{
}

QString RefAst::dump() const
{
    QString r = QStringLiteral("Ref(\n");
    dumpNode(r, QStringLiteral("value="), value);
    dumpList(r, QStringLiteral("\nindices="), indices, QStringLiteral(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

KwArgAst::KwArgAst(Ast* parent)
    : ExpressionAst(parent, AstType::KwArgAstType)
{
}

QString KwArgAst::dump() const
{
    QString r = QStringLiteral("KwArg(\n");
    dumpNode(r, QStringLiteral("key="), key);
    dumpNode(r, QStringLiteral("\nvalue="), value);
    r.append(QLatin1Char('\n'));
    r.append(QStringLiteral(")"));
    return r;
}

} // namespace Julia
