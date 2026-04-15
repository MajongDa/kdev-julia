#ifndef JULIA_AST_H
#define JULIA_AST_H

#include <QList>
#include <QString>
#include <QSharedPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <language/editor/rangeinrevision.h>
/*TODO
 * некорректная структура данных - путаница наследования и композиции
 * отсюда некорретные визиторы с грязным кодом
 * отсюда грязный код в asttransformer
 * contextbuilder context
 * usebuilder context
 * highlighting
 */
namespace KDevelop { class DUContext; }

namespace Julia {

enum class AstType {
    AstType, //0
    
    IdentifierAstType, //1

    // Statements (Julia-specific)
    BodyAstType, //2
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

    // Control flow
    WhereAstType, //27
    ElseIfAstType, //28
    InAstType, //29
    IterationAstType, //30
    
    // Expressions
    ExpressionAstType, //31
    CallAstType, //32
    AttributeAstType,  //33
    BinaryOperationAstType, //34
    UnaryOperationAstType, //35
    LambdaAstType, //36
    IfExpressionAstType, //37
    DictAstType, //38
    SetAstType, //39
    ListAstType, //40
    TupleAstType, //41
    NumberAstType, //42
    StringAstType, //43
    EllipsisAstType, //44 varargs before ;
    SubscriptAstType, //45
    SliceAstType, //46
    StarredAstType, //47
    
    // Julia-specific expressions
    ParameterAstType, //48
    TypeAnnotationAstType, //49
    CurlyAstType, //50
    ImportPathAstType, //51
    GeneratorAstType, //52
    InterpolatedStringAstType, //53
    MacroCallAstType, //54
    RefAstType, //55
    KwArgAstType, //56
    
    // Top-level code
    TopLevelAstType, //57
    
    // Pattern (for match statement)
    PatternAstType, //58
    MatchAstType, //59
    MatchCaseAstType, //60
    
    // Additional types for AST construction
    ArgumentsAstType, //61
    ArgAstType, //62
    KeywordAstType, //63
    AliasAstType, //64
    ExceptionHandlerAstType, //65
    ComprehensionAstType, //66
    
    LastAstType  //67
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
        return astType >= AstType::BodyAstType && astType < AstType::ExpressionAstType;
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
class BodyAst : public Ast
{
public:
    /* TODO Statement is body of function, struct etc.
     * we need QList<Ast*> body inside it for Identifiers etc
     * also correct visitor methods should be implemented
     */
    BodyAst(Ast* parent, AstType type);

    QList<Ast*> body;

    QString dump() const override;
};


class ReturnAst : public BodyAst
{
public:
    ReturnAst(Ast* parent);
    
    Ast* value = nullptr;
    
    QString dump() const override;
};

class AssignmentAst : public BodyAst
{
public:
    AssignmentAst(Ast* parent);
    
    QList<Ast*> targets;
    Ast* value = nullptr;
    
    QString dump() const override;
};

class ForAst : public BodyAst
{
public:
    ForAst(Ast* parent);
    
    Ast* target = nullptr;
    Ast* iterator = nullptr;
    BodyAst* body = nullptr;
    BodyAst* orelse = nullptr;
    
    QString dump() const override;
};

class WhileAst : public BodyAst
{
public:
    WhileAst(Ast* parent);
    
    Ast* condition = nullptr;
    BodyAst* body = nullptr;
    BodyAst* orelse = nullptr;
    
    QString dump() const override;
};

class IfAst : public BodyAst
{
public:
    IfAst(Ast* parent);
    
    Ast* condition = nullptr;
    BodyAst* body = nullptr;
    BodyAst* orelse = nullptr;
    
    QString dump() const override;
};

class TryAst : public BodyAst
{
public:
    TryAst(Ast* parent);
    
    BodyAst* body = nullptr;
    QList<Ast*> handlers;  // ExceptionHandlerAst
    BodyAst* orelse = nullptr;
    BodyAst* finally = nullptr;
    
    QString dump() const override;
};

class ImportAst : public BodyAst
{
public:
    ImportAst(Ast* parent);
    
    QList<Ast*> names;  // alias nodes
    QString module;
    int level = 0;
    
    QString dump() const override;
};

class GlobalAst : public BodyAst
{
public:
    GlobalAst(Ast* parent);
    
    QList<IdentifierAst*> names;
    
    QString dump() const override;
};

class BreakAst : public BodyAst
{
public:
    BreakAst(Ast* parent);
    QString dump() const override;
};

class ContinueAst : public BodyAst
{
public:
    ContinueAst(Ast* parent);
    QString dump() const override;
};

// Julia-specific statement classes
class ModuleAst : public BodyAst
{
public:
    ModuleAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    BodyAst* body = nullptr;
    
    QString dump() const override;
};

class BaremoduleAst : public BodyAst
{
public:
    BaremoduleAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    BodyAst* body = nullptr;
    
    QString dump() const override;
};

//TODO Struct contains Body node, not inherits from it
class StructAst : public BodyAst
{
public:
    StructAst(Ast* parent);

    bool isMutable = false;
    IdentifierAst* name = nullptr;
    Ast* typeParameters = nullptr;  // for parametric structs: Foo{T}
    BodyAst* body = nullptr;
    
    QString dump() const override;
};

class AbstractAst : public BodyAst
{
public:
    AbstractAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    Ast* typeParameters = nullptr;
    
    QString dump() const override;
};

class PrimitiveAst : public BodyAst
{
public:
    PrimitiveAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    Ast* typeParameters = nullptr;
    Ast* underlyingType = nullptr;
    
    QString dump() const override;
};

class MacroAst : public BodyAst
{
public:
    MacroAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    Ast* parameters = nullptr;
    BodyAst* body = nullptr;
    
    QString dump() const override;
};

class UsingAst : public BodyAst
{
public:
    UsingAst(Ast* parent);
    
    QList<Ast*> names;  // ImportPath nodes
    
    QString dump() const override;
};

class ExportAst : public BodyAst
{
public:
    ExportAst(Ast* parent);
    
    QList<IdentifierAst*> names;
    
    QString dump() const override;
};

class ConstAst : public BodyAst
{
public:
    ConstAst(Ast* parent);
    
    Ast* target = nullptr;
    Ast* value = nullptr;
    
    QString dump() const override;
};

class LocalAst : public BodyAst
{
public:
    LocalAst(Ast* parent);
    
    QList<IdentifierAst*> names;
    
    QString dump() const override;
};

class LetAst : public BodyAst
{
public:
    LetAst(Ast* parent);
    
    QList<Ast*> bindings;
    QList<Ast*> body;
    
    QString dump() const override;
};

class DoAst : public BodyAst
{
public:
    DoAst(Ast* parent);
    
    Ast* call = nullptr;
    QList<Ast*> body;
    
    QString dump() const override;
};


// Control flow - after DoAst, before ExpressionAst

class WhereAst : public Ast
{
public:
    WhereAst(Ast* parent);
    
    // children[0] = signature (CallAst or TypeAnnotationAst)
    // children[1..] = constraints (list of <: nodes)
    QList<Ast*> constraints;
    
    QString dump() const override;
};

class ElseIfAst : public BodyAst
{
public:
    ElseIfAst(Ast* parent);
    
    Ast* condition = nullptr;
    BodyAst* body = nullptr;
    
    QString dump() const override;
};

class InAst : public Ast
{
public:
    InAst(Ast* parent);
    
    Ast* target = nullptr;  // iterator variable
    Ast* iter = nullptr;    // iterable expression
    
    QString dump() const override;
};

class IterationAst : public Ast
{
public:
    IterationAst(Ast* parent);
    
    // Contains the in node(s) - multiple iterators in for loop
    QList<Ast*> iterators;
    
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

/* Represents generic ellipsis
 * may be seen in varargs and kwargs statements
 */
class EllipsisAst : public ExpressionAst
{
public:
    EllipsisAst(Ast* parent);

    IdentifierAst* name;

    QString dump() const override;

};
/* Represents function(*; ParameterAst...)
 * may be represented in function call
 * so we should contain list of its children
*/
class ParameterAst : public ExpressionAst
{
public:
    ParameterAst(Ast* parent);
    
    // default kwargs
    QList<AssignmentAst*> kwargs;
    EllipsisAst* ellipsis = nullptr;
    
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
    
    QString dump() const override;
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

class ArgumentsAst : public Ast
{
public:
    ArgumentsAst(Ast* parent);

    // IdentifierAst* name = nullptr;//Julia have name as call children
    // QList<IdentifierAst*> arguments;// Regular positional arguments
    // QList<AssignmentAst*> posonlyargs;// Positional-only arguments (before ;)
    // // (only one ellipsis!) (before ;)
    // EllipsisAst* vararg = nullptr;  // *args
    // // Keyword-only arguments(only one ellipsis!) (after ;)
    // ParameterAst* kwarg = nullptr;  // **kwargs

    QString dump() const override;
};

class CallAst : public ExpressionAst
{
public:
    CallAst(Ast* parent);

    IdentifierAst* name = nullptr;//Julia have name as call children
    QList<Ast*> arguments;// Positional and Positional-only arguments (before ;)
    // (only one ellipsis!) (before ;)
    EllipsisAst* vararg = nullptr;  // *args
    // Keyword-only arguments(only one ellipsis!) (after ;)
    ParameterAst* kwarg = nullptr;  // **kwargs

    QString dump() const override;
};

//TODO Function contains Body node, not inherits from it
class FunctionDefinitionAst : public BodyAst
{
public:
    FunctionDefinitionAst(Ast* parent);

    // Function signature - can be CallAst, TypeAnnotationAst, or WhereAst
    // WhereAst contains: children[0]=TypeAnnotationAst/CallAst, children[1..]=<: constraints
    // TypeAnnotationAst contains: children[0]=CallAst/WhereAst, children[1]=return type
    Ast* callSignature = nullptr;
    
    // Function body
    BodyAst* body = nullptr;

    QString dump() const override;
};

// CodeAst - Top-level code
class CodeAst : public Ast
{
public:
    CodeAst();
    ~CodeAst();

    // Module body - list of top-level statements (functions, structs, imports, etc.)
    QList<Ast*> children;
    IdentifierAst* name;  // module name

    QString dump() const override;
};


// Type definitions


typedef QSharedPointer<CodeAst> CodeAstPtr;

} // namespace Julia

#endif // JULIA_AST_H
