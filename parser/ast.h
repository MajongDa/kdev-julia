#ifndef JULIA_AST_H
#define JULIA_AST_H

#include <QList>
#include <QString>
#include <QSharedPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <language/editor/rangeinrevision.h>

namespace KDevelop { class DUContext; }

namespace Julia {

enum class AstType {
    AstType, //0
    
    IdentifierAstType, //1

    // Statements (Julia-specific)
    StatementAstType, //2
    FunctionDefinitionAstType, //3
    ReturnAstType, //4
    AssignmentAstType, //5
    ForAstType, //6
    WhileAstType, //7
    IfAstType, //8
    TryAstType, //9
    ImportAstType, //10
    ImportFromAstType, //11
    GlobalAstType,  //12
    BreakAstType, //13
    ContinueAstType, //14
    
    // Julia-specific statements
    ModuleAstType, //15
    BaremoduleAstType, //16
    StructAstType,  //17
    AbstractAstType, //18
    PrimitiveAstType, //19
    MacroAstType, //20
    UsingAstType, //21
    ExportAstType, //22
    ConstAstType, //23
    LocalAstType, //24
    LetAstType, //25
    DoAstType, //26
    
    // Expressions
    ExpressionAstType, //27
    CallAstType, //28
    AttributeAstType,  //29
    BinaryOperationAstType, //30
    UnaryOperationAstType, //31
    LambdaAstType, //32
    IfExpressionAstType, //33
    DictAstType, //34
    SetAstType, //35
    ListAstType, //36
    TupleAstType, //37
    NumberAstType, //38
    StringAstType, //39
    EllipsisAstType, //40
    SubscriptAstType, //41
    SliceAstType, //42
    StarredAstType, //43
    
    // Julia-specific expressions
    ParameterAstType, //44
    TypeAnnotationAstType, //45
    CurlyAstType, //46
    ImportPathAstType, //47
    GeneratorAstType, //48
    InterpolatedStringAstType, //49
    MacroCallAstType, //50
    RefAstType, //51
    KwArgAstType, //52
    
    // Top-level code
    TopLevelAstType, //53
    
    // Pattern (for match statement)
    PatternAstType, //54
    MatchAstType, //55
    MatchCaseAstType, //56
    
    // Additional types for AST construction
    ArgumentsAstType, //57
    ArgAstType, //58
    KeywordAstType, //59
    AliasAstType, //60
    ExceptionHandlerAstType, //62
    ComprehensionAstType, //63
    
    LastAstType  //64
};

class Ast
{
public:
    Ast(Ast* parent = nullptr, AstType type = AstType::AstType);
    virtual ~Ast();
    
    Ast* parent = nullptr;
    AstType astType = AstType::AstType;
    
    bool isExpression() const { 
        return astType >= AstType::ExpressionAstType && astType < AstType::TopLevelAstType; 
    }
    bool isStatement() const { 
        return astType >= AstType::StatementAstType && astType < AstType::ExpressionAstType;
    }
    
    void copyRange(const Ast* other) {
        startCol = other->startCol;
        endCol = other->endCol;
        startLine = other->startLine;
        endLine = other->endLine;
    }
    
    bool appearsBefore(const Ast* other) {
        return startLine < other->startLine || (startLine == other->startLine && startCol < other->startCol);
    }
    
    KDevelop::RangeInRevision range() const {
        return KDevelop::RangeInRevision(startLine, startCol, endLine, endCol);
    }
    
    KDevelop::CursorInRevision start() const {
        return KDevelop::CursorInRevision(startLine, startCol);
    }
    
    KDevelop::CursorInRevision end() const {
        return KDevelop::CursorInRevision(endLine, endCol);
    }
    
    bool isChildOf(Ast* other) const {
        const Ast* p = this;
        while (p) {
            if (p == other) return true;
            p = p->parent;
        }
        return false;
    }
    
    virtual QString dump() const {
        QString r = QStringLiteral("Ast(astType=");
        r.append(QString::number(static_cast<int>(astType)));
        r.append(QStringLiteral(", startLine="));
        r.append(QString::number(startLine));
        r.append(QStringLiteral(", startCol="));
        r.append(QString::number(startCol));
        r.append(QStringLiteral(", endCol="));
        r.append(QString::number(endCol));
        r.append(QStringLiteral(", endLine="));
        r.append(QString::number(endLine));
        r.append(QStringLiteral(")"));
        return r;
    };
    
    int startCol = 0;
    int startLine = 0;
    int endCol = 0;
    int endLine = 0;
    
    KDevelop::DUContext* context = nullptr;
};

// Used by AbstractUseBuilder as NameT parameter
class IdentifierAst : public Ast
{
public:
    IdentifierAst(Ast* parent = nullptr, const QString& value = QString());
    
    QString value;
    
    QString dump() const override;

    //TODO Make code prettier, this is obvios reimplemntation
    // do we need this?
    enum Context { Load = 1, Store = 2, Delete = 3, Invalid = -1 };
    Context context = Context::Load;
    bool operator!=(Context c) const { return context != c; }
    
    operator QString() const { return value; }
    
    bool operator==(const IdentifierAst& rhs) const { return value == rhs.value; }
    bool operator==(const QString& rhs) const { return value == rhs; }
};


// Statement classes


class StatementAst : public Ast
{
public:
    /* TODO Statement is body of function etc.
     * we need QList<Ast*> body inside it for Identifiers etc
     * also correct visitor methods should be implemented
    */
    StatementAst(Ast* parent, AstType type) : Ast(parent, type) {}
};

class ArgumentsAst : public Ast
{
public:
    ArgumentsAst(Ast* parent);
    
    QList<Ast*> arguments;       // Regular positional arguments
    QList<Ast*> posonlyargs;  // Positional-only arguments (before ;)
    QList<Ast*> kwonlyargs;  // Keyword-only arguments (after ;)
    QList<Ast*> defaultValues;
    Ast* vararg = nullptr;    // *args
    Ast* kwarg = nullptr;   // **kwargs
    
    QString dump() const override;
};

class FunctionDefinitionAst : public StatementAst
{
public:
    FunctionDefinitionAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    ArgumentsAst* arguments = nullptr;  // Function arguments
    QList<Ast*> decorators;
    QList<Ast*> body;
    Ast* returns = nullptr;
    
    QString dump() const override;
};

class ReturnAst : public StatementAst
{
public:
    ReturnAst(Ast* parent);
    
    Ast* value = nullptr;
    
    QString dump() const override;
};

class AssignmentAst : public StatementAst
{
public:
    AssignmentAst(Ast* parent);
    
    QList<Ast*> targets;
    Ast* value = nullptr;
    
    QString dump() const override;
};

class ForAst : public StatementAst
{
public:
    ForAst(Ast* parent);
    
    Ast* target = nullptr;
    Ast* iterator = nullptr;
    QList<Ast*> body;
    QList<Ast*> orelse;
    
    QString dump() const override;
};

class WhileAst : public StatementAst
{
public:
    WhileAst(Ast* parent);
    
    Ast* condition = nullptr;
    QList<Ast*> body;
    QList<Ast*> orelse;
    
    QString dump() const override;
};

class IfAst : public StatementAst
{
public:
    IfAst(Ast* parent);
    
    Ast* condition = nullptr;
    QList<Ast*> body;
    QList<Ast*> orelse;
    
    QString dump() const override;
};

class TryAst : public StatementAst
{
public:
    TryAst(Ast* parent);
    
    QList<Ast*> body;
    QList<Ast*> handlers;  // catch blocks
    QList<Ast*> orelse;
    QList<Ast*> finally;
    
    QString dump() const override;
};

class ImportAst : public StatementAst
{
public:
    ImportAst(Ast* parent);
    
    QList<Ast*> names;  // alias nodes
    QString module;
    int level = 0;
    
    QString dump() const override;
};

class GlobalAst : public StatementAst
{
public:
    GlobalAst(Ast* parent);
    
    QList<IdentifierAst*> names;
    
    QString dump() const override;
};

class BreakAst : public StatementAst
{
public:
    BreakAst(Ast* parent);
    QString dump() const override { return QStringLiteral("Break()"); }
};

class ContinueAst : public StatementAst
{
public:
    ContinueAst(Ast* parent);
    QString dump() const override { return QStringLiteral("Continue()"); }
};

// Julia-specific statement classes
class ModuleAst : public StatementAst
{
public:
    ModuleAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    QList<Ast*> body;
    
    QString dump() const override;
};

class BaremoduleAst : public StatementAst
{
public:
    BaremoduleAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    QList<Ast*> body;
    
    QString dump() const override;
};

class StructAst : public StatementAst
{
public:
    StructAst(Ast* parent);
    
    bool isMutable = false;
    IdentifierAst* name = nullptr;
    Ast* typeParameters = nullptr;  // for parametric structs: Foo{T}
    QList<Ast*> body;
    
    QString dump() const override;
};

class AbstractAst : public StatementAst
{
public:
    AbstractAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    Ast* typeParameters = nullptr;
    
    QString dump() const override;
};

class PrimitiveAst : public StatementAst
{
public:
    PrimitiveAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    Ast* typeParameters = nullptr;
    Ast* underlyingType = nullptr;
    
    QString dump() const override;
};

class MacroAst : public StatementAst
{
public:
    MacroAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    Ast* parameters = nullptr;
    QList<Ast*> body;
    
    QString dump() const override;
};

class UsingAst : public StatementAst
{
public:
    UsingAst(Ast* parent);
    
    QList<Ast*> names;  // ImportPath nodes
    
    QString dump() const override;
};

class ExportAst : public StatementAst
{
public:
    ExportAst(Ast* parent);
    
    QList<IdentifierAst*> names;
    
    QString dump() const override;
};

class ConstAst : public StatementAst
{
public:
    ConstAst(Ast* parent);
    
    Ast* target = nullptr;
    Ast* value = nullptr;
    
    QString dump() const override;
};

class LocalAst : public StatementAst
{
public:
    LocalAst(Ast* parent);
    
    QList<IdentifierAst*> names;
    
    QString dump() const override;
};

class LetAst : public StatementAst
{
public:
    LetAst(Ast* parent);
    
    QList<Ast*> bindings;
    QList<Ast*> body;
    
    QString dump() const override;
};

class DoAst : public StatementAst
{
public:
    DoAst(Ast* parent);
    
    Ast* call = nullptr;
    QList<Ast*> body;
    
    QString dump() const override;
};


// Expression AST - must be defined before expression classes


class ExpressionAst : public Ast
{
public:
    ExpressionAst(Ast* parent, AstType type = AstType::ExpressionAstType);
    
    enum Context { Load = 1, Store = 2, Delete = 3, Invalid = -1 };
    Context context = Context::Load;
};


// Julia-specific expression classes
class ParameterAst : public ExpressionAst
{
public:
    ParameterAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    Ast* defaultValue = nullptr;
    Ast* annotation = nullptr;

    
    QString dump() const override;
};

class TypeAnnotationAst : public ExpressionAst
{
public:
    TypeAnnotationAst(Ast* parent);
    
    Ast* value = nullptr;
    Ast* type = nullptr;
    
    QString dump() const override;
};

class CurlyAst : public ExpressionAst
{
public:
    CurlyAst(Ast* parent);
    
    Ast* name = nullptr;
    QList<Ast*> parameters;
    
    QString dump() const override;
};

class ImportPathAst : public ExpressionAst
{
public:
    ImportPathAst(Ast* parent);
    
    QList<IdentifierAst*> names;
    IdentifierAst* asName = nullptr;
    int dotCount = 0;
    
    QString dump() const override;
};

class GeneratorAst : public ExpressionAst
{
public:
    GeneratorAst(Ast* parent);
    
    Ast* expression = nullptr;
    Ast* iterator = nullptr;
    QList<Ast*> filters;
    
    QString dump() const override;
};

class InterpolatedStringAst : public ExpressionAst
{
public:
    InterpolatedStringAst(Ast* parent);
    
    QList<Ast*> parts;
    
    QString dump() const override;
};

class MacroCallAst : public ExpressionAst
{
public:
    MacroCallAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    QList<Ast*> arguments;
    
    QString dump() const override;
};

class RefAst : public ExpressionAst
{
public:
    RefAst(Ast* parent);
    
    Ast* value = nullptr;
    QList<Ast*> indices;
    
    QString dump() const override;
};

class KwArgAst : public ExpressionAst
{
public:
    KwArgAst(Ast* parent);
    
    IdentifierAst* key = nullptr;
    Ast* value = nullptr;
    
    QString dump() const override;
};

// Expression classes
class CallAst : public ExpressionAst
{
public:
    CallAst(Ast* parent);
    
    Ast* function = nullptr;
    QList<Ast*> arguments;
    QList<Ast*> keywords;  // keyword arguments
    
    QString dump() const override;
};

class BinaryOperationAst : public ExpressionAst
{
public:
    enum class Operator {
        Add, Sub, Mul, Div, FloorDiv, Mod, Pow,
        LShift, RShift, BitAnd, BitOr, BitXor,
        And, Or,
        Eq, Ne, Lt, Le, Gt, Ge
    };
    
    BinaryOperationAst(Ast* parent);
    
    Operator op = Operator::Add;
    Ast* left = nullptr;
    Ast* right = nullptr;
    
    QString dump() const override;
};

class UnaryOperationAst : public ExpressionAst
{
public:
    enum class Operator { Invert, Not, UAdd, USub };
    
    UnaryOperationAst(Ast* parent);
    
    Operator op = Operator::UAdd;
    Ast* operand = nullptr;
    
    QString dump() const override;
};

class NumberAst : public ExpressionAst
{
public:
    NumberAst(Ast* parent, AstType type = AstType::NumberAstType);
    
    QString value;
    bool isInt = false;
    
    QString dump() const override;
};

class StringAst : public ExpressionAst
{
public:
    StringAst(Ast* parent, AstType type = AstType::StringAstType);
    
    QString value;
    
    QString dump() const override { return QStringLiteral("Str('") + value + QStringLiteral("')"); }
};

class ListAst : public ExpressionAst
{
public:
    ListAst(Ast* parent);
    
    QList<Ast*> elements;
    ExpressionAst::Context context = ExpressionAst::Load;
    
    QString dump() const override;
};

class TupleAst : public ExpressionAst
{
public:
    TupleAst(Ast* parent);
    
    QList<Ast*> elements;
    ExpressionAst::Context context = ExpressionAst::Load;
    
    QString dump() const override;
};

class DictAst : public ExpressionAst
{
public:
    DictAst(Ast* parent);
    
    QList<Ast*> keys;
    QList<Ast*> values;
    
    QString dump() const override;
};

class SubscriptAst : public ExpressionAst
{
public:
    SubscriptAst(Ast* parent);
    
    Ast* value = nullptr;
    Ast* slice = nullptr;
    ExpressionAst::Context context = ExpressionAst::Load;
    
    QString dump() const override;
};

class AttributeAst : public ExpressionAst
{
public:
    AttributeAst(Ast* parent);
    
    Ast* value = nullptr;
    IdentifierAst* attribute = nullptr;
    ExpressionAst::Context context = ExpressionAst::Load;
    int depth = 0;
    
    QString dump() const override;
};

class StarredAst : public ExpressionAst
{
public:
    StarredAst(Ast* parent);
    
    Ast* value = nullptr;
    ExpressionAst::Context context = ExpressionAst::Load;
    
    QString dump() const override;
};

class LambdaAst : public ExpressionAst
{
public:
    LambdaAst(Ast* parent);
    
    Ast* arguments = nullptr;
    Ast* body = nullptr;
    
    QString dump() const override;
};

class IfExpressionAst : public ExpressionAst
{
public:
    IfExpressionAst(Ast* parent);
    
    Ast* condition = nullptr;
    Ast* body = nullptr;
    Ast* orelse = nullptr;
    
    QString dump() const override;
};

// Pattern classes (for match statement)
class PatternAst : public Ast
{
public:
    PatternAst(Ast* parent, AstType type = AstType::PatternAstType) : Ast(parent, type) {}
};

class MatchCaseAst : public Ast
{
public:
    MatchCaseAst(Ast* parent);
    
    PatternAst* pattern = nullptr;
    Ast* guard = nullptr;
    QList<Ast*> body;
    
    QString dump() const override;
};

class MatchAst : public Ast
{
public:
    MatchAst(Ast* parent);
    
    Ast* subject = nullptr;
    QList<MatchCaseAst*> cases;
    
    QString dump() const override;
};

// Other AST classes
class ArgAst : public Ast
{
public:
    ArgAst(Ast* parent);
    
    IdentifierAst* argumentName = nullptr;
    Ast* annotation = nullptr;
    
    QString dump() const override;
};

class KeywordAst : public Ast
{
public:
    KeywordAst(Ast* parent);
    
    IdentifierAst* argumentName = nullptr;
    Ast* value = nullptr;
    
    QString dump() const override;
};

class AliasAst : public Ast
{
public:
    AliasAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    IdentifierAst* asName = nullptr;
    
    QString dump() const override;
};

class ExceptionHandlerAst : public Ast
{
public:
    ExceptionHandlerAst(Ast* parent);
    
    Ast* type = nullptr;
    IdentifierAst* name = nullptr;
    QList<Ast*> body;
    
    QString dump() const override;
};

class ComprehensionAst : public Ast
{
public:
    ComprehensionAst(Ast* parent);
    
    Ast* target = nullptr;
    Ast* iterator = nullptr;
    QList<Ast*> conditions;
    
    QString dump() const override;
};

class SliceAst : public ExpressionAst
{
public:
    SliceAst(Ast* parent);
    
    Ast* lower = nullptr;
    Ast* upper = nullptr;
    Ast* step = nullptr;
    
    QString dump() const override;
};

// CodeAst - Top-level code
class CodeAst : public Ast
{
public:
    CodeAst();
    ~CodeAst();

    QList<Ast*> body;
    IdentifierAst* name;  // module name

    QString dump() const override;
};


// Type definitions


typedef QSharedPointer<CodeAst> CodeAstPtr;

} // namespace Julia

#endif // JULIA_AST_H
