#include "ast.h"
#include "juliaastdefaultvisitor.h"

#include "juliadebug.h"

namespace Julia {

static void dumpNode(QString &r, QString prefix, const Ast* node)
{
    r.append(prefix);
    r.append(node ? node->dump(): QLatin1String("None"));
}

template<class T>
static void dumpList(QString &r, QString prefix, const T list,QString sep=QLatin1String(", "))
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
    return QLatin1String("Identifier(value='") + value + QLatin1String("')");
}


// Statement implementations
FunctionSignatureAst::FunctionSignatureAst(Ast* parent)
: Ast(parent)
{
}

QString FunctionSignatureAst::dump() const
{
    QString r = QLatin1String("FunctionSignature(\n");
    dumpNode(r, QLatin1String("name="), name);
    dumpList(r, QLatin1String("\npostional="), positionalArgs);
    dumpList(r, QLatin1String("\nkwargs="), keywordArgs);
    dumpNode(r, QLatin1String("\nannotation="), returnType);
    dumpList(r, QLatin1String("constraint="), whereConstraints);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")\n"));

    return r;
}

FunctionDefinitionAst::FunctionDefinitionAst(Ast* parent)
    : Ast(parent, AstType::FunctionDefinitionAstType), signature(nullptr), block(nullptr)
{
}

QString FunctionDefinitionAst::dump() const
{
    QString r = QLatin1String("FunctionDefinition(\nsignature=");
    dumpNode(r, QString(), signature);
    dumpNode(r, QLatin1String("\nbody="), block);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")\n"));

    return r;
}

ReturnAst::ReturnAst(Ast* parent)
    : Ast(parent, AstType::ReturnAstType)
{
}

QString ReturnAst::dump() const
{
    QString r = QLatin1String("Return(\n");
    dumpNode(r, QLatin1String("value="), value);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

AssignmentAst::AssignmentAst(Ast* parent, AstType type = AstType::AssignmentAstType)
    : Ast(parent, type)
{
}

QString AssignmentAst::dump() const
{
    QString r = QLatin1String("Assignment(\n");
    dumpNode(r, QLatin1String("target="), target);
    dumpNode(r, QLatin1String("\nvalue="), value);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

CompoundAssignmentAst::CompoundAssignmentAst(Ast* parent)
: AssignmentAst(parent, AstType::CompoundAssignmentAstType)
{
}

QString CompoundAssignmentAst::dump() const
{
    QString r = QLatin1String("CompoundAssignment(\n");
    dumpNode(r, QLatin1String("target="), target);
    dumpNode(r, QLatin1String("\noperator="), compoundOperator);
    dumpNode(r, QLatin1String("\nvalue="), value);
    r.append(QLatin1String("\n)"));
    return r;
}

ForAst::ForAst(Ast* parent)
    : Ast(parent, AstType::ForAstType)
{
}

QString ForAst::dump() const
{
    QString r = QLatin1String("For(\n");
    dumpNode(r, QLatin1String("iterator="), iterator);
    dumpNode(r, QLatin1String("\nbody="), block);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

WhileAst::WhileAst(Ast* parent)
    : Ast(parent, AstType::WhileAstType)
{
}

QString WhileAst::dump() const
{
    QString r = QLatin1String("While(\n");
    dumpNode(r, QLatin1String("condition="), condition);
    dumpNode(r, QLatin1String("\nbody="), block);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

IfAst::IfAst(Ast* parent)
    : Ast(parent, AstType::IfAstType)
{
}

QString IfAst::dump() const
{
    QString r = QLatin1String("If(\n");
    dumpNode(r, QLatin1String("condition="), condition);
    dumpNode(r, QLatin1String("\nbody="), block);
    dumpNode(r, QLatin1String("\norelse="), orelse);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

TryAst::TryAst(Ast* parent)
    : Ast(parent, AstType::TryAstType)
{
}

QString TryAst::dump() const
{
    QString r = QLatin1String("Try(\n");
    dumpNode(r, QLatin1String("block="), block);
    dumpNode(r, QLatin1String("\nhandler="), handler);
    dumpNode(r, QLatin1String("\norelse="), orelse);
    dumpNode(r, QLatin1String("\nfinally="), finally);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

ImportAst::ImportAst(Ast* parent)
    : Ast(parent, AstType::ImportAstType)
{
}

QString ImportAst::dump() const
{
    QString r = QLatin1String("Import(\n");
    dumpNode(r, QLatin1String("module="), module);
    dumpList(r, QLatin1String("\nnames="), names, QLatin1String(",\n"));
    r.append(QLatin1String("\n)"));
    return r;
}

SelectiveImportAst::SelectiveImportAst(Ast* parent)
: ImportAst(parent)
{
    astType = AstType::SelectiveImportAstType;
}
QString SelectiveImportAst::dump() const
{
    QString r = QLatin1String("SelectiveImport(\n");
    dumpNode(r, QLatin1String("module="), module);
    dumpList(r, QLatin1String("\nnames="), names);
    r.append(QLatin1String("\n)"));
    return r;
}

GlobalAst::GlobalAst(Ast* parent)
    : Ast(parent, AstType::GlobalAstType)
{
}

QString GlobalAst::dump() const
{
    QString r = QLatin1String("Global(\n");
    dumpList(r, QLatin1String("names="), names, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

BreakAst::BreakAst(Ast* parent)
    : Ast(parent, AstType::BreakAstType)
{
}

ContinueAst::ContinueAst(Ast* parent)
    : Ast(parent, AstType::ContinueAstType)
{
}

QString BreakAst::dump() const
{
    return QLatin1String("Break()");
}

QString ContinueAst::dump() const
{
    return QLatin1String("Continue()");
}

// Control flow implementations
WhereAst::WhereAst(Ast* parent)
    : Ast(parent, AstType::WhereAstType)
{
}

QString WhereAst::dump() const
{
    QString r = QLatin1String("Where(\n");
    dumpNode(r, QLatin1String("signature="), signature);
    dumpList(r, QLatin1String("\nconstraints="), constraints, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

InAst::InAst(Ast* parent)
    : Ast(parent, AstType::InAstType)
{
}

QString InAst::dump() const
{
    QString r = QLatin1String("In(\n");
    dumpNode(r, QLatin1String("target="), target);
    dumpNode(r, QLatin1String("\niter="), iter);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

IterationAst::IterationAst(Ast* parent)
    : Ast(parent, AstType::IterationAstType)
{
}

QString IterationAst::dump() const
{
    QString r = QLatin1String("Iteration(\n");
    dumpList(r, QLatin1String("iterators="), iterators, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
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
    QString r = QLatin1String("Call(\n");
    dumpNode(r, QLatin1String("name="), name);
    dumpList(r, QLatin1String("\narguments="), arguments, QLatin1String(",\n"));
    // dumpNode(r, QLatin1String("\nvararg="), vararg);
    // dumpNode(r, QLatin1String("\nkwarg="), kwarg);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

NumberAst::NumberAst(Ast* parent, AstType type)
    : ExpressionAst(parent, type), isInt(false)
{
}

QString NumberAst::dump() const
{
    QString r = QLatin1String("Number(\n");
    r.append(QLatin1String("value=%1,\n").arg(value));
    r.append(QLatin1String("isInt=%1\n").arg(isInt ? "true" : "false"));
    r.append(QLatin1String(")"));
    return r;
}

StringAst::StringAst(Ast* parent, AstType type)
    : ExpressionAst(parent, type)
{
}

QString StringAst::dump() const
{
    return QLatin1String("String(value='") + value + QLatin1String("')");
}

ListAst::ListAst(Ast* parent)
    : ExpressionAst(parent, AstType::ListAstType)
{
}

QString ListAst::dump() const
{
    QString r = QLatin1String("List(\n");
    dumpList(r, QLatin1String("elements="), elements, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

TupleAst::TupleAst(Ast* parent)
    : ExpressionAst(parent, AstType::TupleAstType)
{
}

QString TupleAst::dump() const
{
    QString r = QLatin1String("Tuple(\n");
    dumpList(r, QLatin1String("elements="), elements, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

DictAst::DictAst(Ast* parent)
    : ExpressionAst(parent, AstType::DictAstType)
{
}

QString DictAst::dump() const
{
    QString r = QLatin1String("Dict(\n");
    dumpList(r, QLatin1String("keys="), keys, QLatin1String(",\n"));
    dumpList(r, QLatin1String("\nvalues="), values, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

AttributeAst::AttributeAst(Ast* parent)
    : ExpressionAst(parent, AstType::AttributeAstType), depth(0)
{
}

QString AttributeAst::dump() const
{
    QString r = QLatin1String("Attribute(\n");
    dumpNode(r, QLatin1String("value="), value);
    dumpNode(r, QLatin1String("\nattribute="), attribute);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

StarredAst::StarredAst(Ast* parent)
    : ExpressionAst(parent, AstType::StarredAstType)
{
}

QString StarredAst::dump() const
{
    QString r = QLatin1String("Starred(\n");
    dumpNode(r, QLatin1String("value="), value);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

LambdaAst::LambdaAst(Ast* parent)
    : ExpressionAst(parent, AstType::LambdaAstType)
{
}

QString LambdaAst::dump() const
{
    QString r = QLatin1String("Lambda(\n");
    dumpNode(r, QLatin1String("arguments="), arguments);
    dumpNode(r, QLatin1String("\nbody="), block);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

IfExpressionAst::IfExpressionAst(Ast* parent)
    : ExpressionAst(parent, AstType::IfExpressionAstType)
{
}

QString IfExpressionAst::dump() const
{
    QString r = QLatin1String("IfExpression(\n");
    dumpNode(r, QLatin1String("condition="), condition);
    dumpNode(r, QLatin1String("\nbody="), block);
    dumpNode(r, QLatin1String("\norelse="), orelse);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

// Other AST implementations
AliasAst::AliasAst(Ast* parent)
    : Ast(parent, AstType::BlockAstType)
{
}

QString AliasAst::dump() const
{
    QString r = QLatin1String("Alias(\n");
    dumpNode(r, QLatin1String("name="), name);
    dumpNode(r, QLatin1String("\nasName="), asName);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

CatchAst::CatchAst(Ast* parent)
    : Ast(parent, AstType::CatchAstType)
{
}

QString CatchAst::dump() const
{
    QString r = QLatin1String("ExceptionHandler(\n");
    dumpNode(r, QLatin1String("type="), type);
    dumpNode(r, QLatin1String("\nname="), name);
    dumpNode(r, QLatin1String("\nbody="), block);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

ComprehensionAst::ComprehensionAst(Ast* parent)
    : Ast(parent, AstType::ExpressionAstType)
{
}

QString ComprehensionAst::dump() const
{
    QString r = QLatin1String("Comprehension(\n");
    dumpNode(r, QLatin1String("target="), target);
    dumpNode(r, QLatin1String("\niterator="), iterator);
    dumpList(r, QLatin1String("\nconditions="), conditions, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

SliceAst::SliceAst(Ast* parent)
    : ExpressionAst(parent, AstType::SliceAstType)
{
}

QString SliceAst::dump() const
{
    QString r = QLatin1String("Slice(\n");
    dumpNode(r, QLatin1String("lower="), lower);
    dumpNode(r, QLatin1String("\nupper="), upper);
    dumpNode(r, QLatin1String("\nstep="), step);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}


// TopLevel implementation
TopLevelAst::TopLevelAst()
    : Ast(nullptr, AstType::TopLevelAstType), name(nullptr)
{
}

TopLevelAst::~TopLevelAst()
{
    free_ast_recursive(this);
}

QString TopLevelAst::dump() const
{
    QString r;
    r.append(QLatin1String("Module("));
    dumpNode(r, QLatin1String("name="), name);
    dumpList(r, QLatin1String(", children="), children, QLatin1String(",\n  "));
    r.append(QLatin1String(")\n"));
    return r;
}


// Julia-specific statement implementations
BlockAst::BlockAst(Ast* parent, AstType type = AstType::BlockAstType)
    : Ast(parent, type)
{
}

QString BlockAst::dump() const
{
    QString r;
    r.append(QLatin1String("Block("));
    dumpList(r, QLatin1String("block="), block, QLatin1String(",\n  "));
    r.append(QLatin1Char(')'));
    return r;
}



ModuleAst::ModuleAst(Ast* parent)
    : Ast(parent, AstType::ModuleAstType)
{
}

QString ModuleAst::dump() const
{
    QString r = QLatin1String("Module(\n");
    dumpNode(r, QLatin1String("name="), name);
    dumpNode(r, QLatin1String("\nbody="), block);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

BaremoduleAst::BaremoduleAst(Ast* parent)
    : Ast(parent, AstType::BaremoduleAstType)
{
}

QString BaremoduleAst::dump() const
{
    QString r = QLatin1String("Baremodule(\n");
    dumpNode(r, QLatin1String("name="), name);
    dumpNode(r, QLatin1String("\nbody="), block);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

StructAst::StructAst(Ast* parent)
    : Ast(parent, AstType::StructAstType)
{
}

QString StructAst::dump() const
{
    QString r = QLatin1String("Struct(mutable=%1,\n").arg(isMutable ? "true" : "false");
    dumpNode(r, QLatin1String("signature="), signature);
    dumpNode(r, QLatin1String("\nbody="), block);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

AbstractAst::AbstractAst(Ast* parent)
    : Ast(parent, AstType::AbstractAstType)
{
}

QString AbstractAst::dump() const
{
    QString r = QLatin1String("Abstract(\n");
    dumpNode(r, QLatin1String("signature="), signature);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

PrimitiveAst::PrimitiveAst(Ast* parent)
    : Ast(parent, AstType::PrimitiveAstType)
{
}

QString PrimitiveAst::dump() const
{
    QString r = QLatin1String("Primitive(\n");
    dumpNode(r, QLatin1String("signature="), signature);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

MacroAst::MacroAst(Ast* parent)
    : Ast(parent, AstType::MacroAstType)
{
}

QString MacroAst::dump() const
{
    QString r = QLatin1String("Macro(\n");
    dumpNode(r, QLatin1String("signature="), signature);
    dumpNode(r, QLatin1String("\nbody="), block);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

UsingAst::UsingAst(Ast* parent)
    : Ast(parent, AstType::UsingAstType)
{
}

QString UsingAst::dump() const
{
    QString r = QLatin1String("Using(\n");
    dumpList(r, QLatin1String("names="), names, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

ExportAst::ExportAst(Ast* parent)
    : Ast(parent, AstType::ExportAstType)
{
}

QString ExportAst::dump() const
{
    QString r = QLatin1String("Export(\n");
    dumpList(r, QLatin1String("names="), names, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

ConstAst::ConstAst(Ast* parent)
    : Ast(parent, AstType::ConstAstType)
{
}

QString ConstAst::dump() const
{
    QString r = QLatin1String("Const(\n");
    dumpNode(r, QLatin1String("target="), target);
    dumpNode(r, QLatin1String("\nvalue="), value);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

LocalAst::LocalAst(Ast* parent)
    : Ast(parent, AstType::LocalAstType)
{
}

QString LocalAst::dump() const
{
    QString r = QLatin1String("Local(\n");
    dumpList(r, QLatin1String("names="), names, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

LetAst::LetAst(Ast* parent)
    : Ast(parent, AstType::LetAstType)
{
}

QString LetAst::dump() const
{
    QString r = QLatin1String("Let(\n");
    dumpList(r, QLatin1String("bindings="), bindings, QLatin1String(",\n"));
    dumpList(r, QLatin1String("\nbody="), block, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

DoAst::DoAst(Ast* parent)
    : Ast(parent, AstType::DoAstType)
{
}

QString DoAst::dump() const
{
    QString r = QLatin1String("Do(\n");
    dumpNode(r, QLatin1String("call="), call);
    dumpList(r, QLatin1String("\nbody="), block, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}


// Julia-specific expression implementations
EllipsisAst::EllipsisAst(Ast* parent)
    : ExpressionAst(parent, AstType::EllipsisAstType)
{
}

QString EllipsisAst::dump() const
{
    QString r = QLatin1String("Ellipsis( ");
    dumpNode(r, QLatin1String("name="), name);
    r.append(QLatin1String(")\n"));
    return r;
}

ParameterAst::ParameterAst(Ast* parent)
    : ExpressionAst(parent, AstType::ParameterAstType)
{
}

QString ParameterAst::dump() const
{
    QString r = QLatin1String("Parameter(\n");
    dumpList(r, QLatin1String("kwargs="), kwargs, QLatin1String(",\n"));
    // dumpNode(r, QLatin1String("\nellipsis="), ellipsis);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

TypeAnnotationAst::TypeAnnotationAst(Ast* parent)
    : ExpressionAst(parent, AstType::TypeAnnotationAstType)
{
}

QString TypeAnnotationAst::dump() const
{
    QString r = QLatin1String("TypeAnnotation(\n");
    dumpNode(r, QLatin1String("value="), value);
    dumpNode(r, QLatin1String("\ntype="), type);
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

SubtypeAst::SubtypeAst(Ast* parent)
: ExpressionAst(parent, AstType::SubtypeAstType)
{
}

QString SubtypeAst::dump() const
{
    QString r = QLatin1String("Subtype(\n");
    dumpNode(r, QLatin1String("left="), left);
    dumpNode(r, QLatin1String("\nright="), right);
    r.append(QLatin1String("\n)"));
    return r;
}

CurlyAst::CurlyAst(Ast* parent)
    : ExpressionAst(parent, AstType::CurlyAstType)
{
}

QString CurlyAst::dump() const
{
    QString r = QLatin1String("Curly(\n");
    dumpNode(r, QLatin1String("name="), name);
    dumpList(r, QLatin1String("\nparameters="), parameters, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

ImportPathAst::ImportPathAst(Ast* parent)
    : ExpressionAst(parent, AstType::ImportPathAstType)
{
}

QString ImportPathAst::dump() const
{
    QString r = QLatin1String("ImportPath(\n");
    dumpList(r, QLatin1String("names="), names, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

GeneratorAst::GeneratorAst(Ast* parent)
    : ExpressionAst(parent, AstType::GeneratorAstType)
{
}

QString GeneratorAst::dump() const
{
    QString r = QLatin1String("Generator(\n");
    dumpNode(r, QLatin1String("expression="), expression);
    dumpNode(r, QLatin1String("\niterator="), iterator);
    dumpList(r, QLatin1String("\nfilters="), filters, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

FilterAst::FilterAst(Ast* parent)
: ExpressionAst(parent, AstType::FilterAstType)
{
}
QString FilterAst::dump() const
{
    QString r = QLatin1String("Filter(\n");
    dumpNode(r, QLatin1String("iterator="), iterator);
    dumpNode(r, QLatin1String("\ncondition="), condition);
    r.append(QLatin1String("\n)"));
    return r;
}

InterpolatedStringAst::InterpolatedStringAst(Ast* parent)
    : ExpressionAst(parent, AstType::InterpolatedStringAstType)
{
}

QString InterpolatedStringAst::dump() const
{
    QString r = QLatin1String("InterpolatedString(\n");
    dumpList(r, QLatin1String("parts="), parts, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

MacroCallAst::MacroCallAst(Ast* parent)
    : ExpressionAst(parent, AstType::MacroCallAstType)
{
}

QString MacroCallAst::dump() const
{
    QString r = QLatin1String("MacroCall(\n");
    dumpNode(r, QLatin1String("name="), name);
    dumpList(r, QLatin1String("\narguments="), arguments, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}

RefAst::RefAst(Ast* parent)
    : ExpressionAst(parent, AstType::RefAstType)
{
}

QString RefAst::dump() const
{
    QString r = QLatin1String("Ref(\n");
    dumpNode(r, QLatin1String("value="), value);
    dumpList(r, QLatin1String("\nindices="), indices, QLatin1String(",\n"));
    r.append(QLatin1Char('\n'));
    r.append(QLatin1String(")"));
    return r;
}
} // namespace Julia
