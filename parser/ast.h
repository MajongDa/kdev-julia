#ifndef JULIA_AST_H
#define JULIA_AST_H

#include <QList>
#include <QString>
#include <QSharedPointer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <language/editor/rangeinrevision.h>
/*
 * TODO add FilterAst and IterationAst
 */
namespace KDevelop { class DUContext; }

namespace Julia {

enum class AstType {
    AstType, //0
    
    IdentifierAstType, //1

    // Statements (Julia-specific)
    BlockAstType, //2
    FunctionDefinitionAstType, //3
    ReturnAstType, //4
    AssignmentAstType, //5
    CompoundAssignmentAstType, //
    ForAstType, //6
    WhileAstType, //7
    IfAstType, //8
    TryAstType, //9
    ImportAstType, //10
    ImportFromAstType, //11
    SelectiveImportAstType, //11b for import A: a, b
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
    SubtypeAstType, //28  for <: and :> type bounds
    InAstType, //29
    IterationAstType, //30
    FilterAstType, //31   // ADD - for comprehension filters
    
    // Expressions
    ExpressionAstType, //31
    CallAstType, //32
    AttributeAstType,  //33
    LambdaAstType, //36
    IfExpressionAstType, //37
    DictAstType, //38
    SetAstType, //39
    ListAstType, //40
    TupleAstType, //41
    NumberAstType, //42
    StringAstType, //43
    EllipsisAstType, //44 varargs before ;
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
    
    // Top-level code
    TopLevelAstType, //57

    
    // Additional types for AST construction
    AliasAstType, //61
    CatchAstType, //62
    ElseAstType, //63
    FinallyAstType, //64
    ComprehensionAstType, //65
    
    LastAstType  //66
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
        return astType >= AstType::BlockAstType && astType < AstType::ExpressionAstType;
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
class BlockAst : public Ast
{
public:
    /* TODO Statement is block of function, struct etc.
     * we need QList<Ast*> block inside it for Identifiers etc
     * also correct visitor methods should be implemented
     */
    BlockAst(Ast* parent, AstType type);

    QList<Ast*> block;

    QString dump() const override;
};


class ReturnAst : public Ast
{
public:
    ReturnAst(Ast* parent);
    
    Ast* value = nullptr;
    
    QString dump() const override;
};

class AssignmentAst : public Ast
{
public:
    AssignmentAst(Ast* parent, AstType type);
    
    Ast* target = nullptr;
    Ast* value = nullptr;
    
    QString dump() const override;
};

class CompoundAssignmentAst : public AssignmentAst
{
public:
    CompoundAssignmentAst(Ast* parent);
    Ast* compoundOperator = nullptr;  // the operator (+, -, *, /, etc.)
    QString dump() const override;
};

class ForAst : public Ast
{
public:
    ForAst(Ast* parent);
    
    // Iterator - IterationAst containing list of InAst nodes
    // JSON: children[0]=iteration, children[1]=block-block
    Ast* iterator = nullptr;
    BlockAst* block = nullptr;
    
    QString dump() const override;
};

class WhileAst : public Ast
{
public:
    WhileAst(Ast* parent);
    
    Ast* condition = nullptr;
    BlockAst* block = nullptr;
    
    QString dump() const override;
};

class IfAst : public Ast
{
public:
    IfAst(Ast* parent);
    
    Ast* condition = nullptr;
    BlockAst* block = nullptr;
    Ast* orelse = nullptr;
    
    QString dump() const override;
};

class CatchAst : public Ast
{
public:
    CatchAst(Ast* parent);

    Ast* type = nullptr; // could be Identifier or Placeholder
    IdentifierAst* name = nullptr; // exception variable
    BlockAst* block = nullptr;

    QString dump() const override;
};

class TryAst : public Ast
{
public:
    TryAst(Ast* parent);
    
    BlockAst* block = nullptr;
    CatchAst* handler = nullptr;  // ExceptionHandlerAst
    BlockAst* orelse = nullptr;
    BlockAst* finally = nullptr;
    
    QString dump() const override;
};

class GlobalAst : public Ast
{
public:
    GlobalAst(Ast* parent);
    
    QList<Ast*> names;
    
    QString dump() const override;
};

class BreakAst : public Ast
{
public:
    BreakAst(Ast* parent);
    QString dump() const override;
};

class ContinueAst : public Ast
{
public:
    ContinueAst(Ast* parent);
    QString dump() const override;
};

// Julia-specific statement classes
class ModuleAst : public Ast
{
public:
    ModuleAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    BlockAst* block = nullptr;
    
    QString dump() const override;
};

class BaremoduleAst : public Ast
{
public:
    BaremoduleAst(Ast* parent);
    
    IdentifierAst* name = nullptr;
    BlockAst* block = nullptr;
    
    QString dump() const override;
};

//TODO Struct contains Body node, not inherits from it
class StructAst : public Ast
{
public:
    StructAst(Ast* parent);


    bool isMutable = false;
    // for parametric structs: Foo{T} where T <: SomeAbstractType
    Ast* signature = nullptr;
    BlockAst* block = nullptr;
    
    QString dump() const override;
};

class AbstractAst : public Ast
{
public:
    AbstractAst(Ast* parent);
    
    // Signature - can be IdentifierAst, CurlyAst, or WhereAst
    // JSON: children[0]=signature
    Ast* signature = nullptr;
    
    QString dump() const override;
};

class PrimitiveAst : public Ast
{
public:
    PrimitiveAst(Ast* parent);
    
    // Signature - call-like with name and bit count
    // JSON: children[0]=call(name, bitCount)
    Ast* signature = nullptr;
    
    QString dump() const override;
};

class MacroAst : public Ast
{
public:
    MacroAst(Ast* parent);
    
    // Signature - call-like with macro name and parameters
    // JSON: children[0]=call, children[1]=block
    Ast* signature = nullptr;
    BlockAst* block = nullptr;
    
    QString dump() const override;
};

class UsingAst : public Ast
{
public:
    UsingAst(Ast* parent);
    
    QList<Ast*> names;  // ImportPath nodes
    
    QString dump() const override;
};

class ExportAst : public Ast
{
public:
    ExportAst(Ast* parent);
    
    QList<IdentifierAst*> names;
    
    QString dump() const override;
};

class ConstAst : public Ast
{
public:
    ConstAst(Ast* parent);
    
    Ast* target = nullptr;
    Ast* value = nullptr;
    
    QString dump() const override;
};

class LocalAst : public Ast
{
public:
    LocalAst(Ast* parent);
    
    QList<IdentifierAst*> names;
    
    QString dump() const override;
};

class LetAst : public Ast
{
public:
    LetAst(Ast* parent);
    
    QList<Ast*> bindings;
    QList<Ast*> block;
    
    QString dump() const override;
};

class DoAst : public Ast
{
public:
    DoAst(Ast* parent);
    
    Ast* call = nullptr;
    QList<Ast*> block;
    
    QString dump() const override;
};


// Control flow

class WhereAst : public Ast
{
public:
    WhereAst(Ast* parent);
    
    // children[0] = signature (CallAst or TypeAnnotationAst)
    // children[1..] = constraints (list of <: nodes)
    Ast* signature = nullptr;
    QList<Ast*> constraints;
    
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
    QList<Ast*> kwargs;
    // EllipsisAst* ellipsis = nullptr;
    
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

class SubtypeAst : public ExpressionAst
{
public:
    SubtypeAst(Ast* parent);

    Ast* left = nullptr;   // e.g., T in "T <: Number"
    Ast* right = nullptr;  // e.g., Number

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
    int dotCount = 0;
    
    QString dump() const override;
};

class ImportAst : public Ast
{
public:
    ImportAst(Ast* parent);

    ImportPathAst* module = nullptr;
    QList<Ast*> names;  // alias nodes

    QString dump() const override;
};

class SelectiveImportAst : public ImportAst
{
public:
    SelectiveImportAst(Ast* parent);

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

class FilterAst : public ExpressionAst
{
public:
    FilterAst(Ast* parent);

    Ast* iterator = nullptr;    // iteration clause (x in items)
    Ast* condition = nullptr;  // filter condition (x > 0)

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


// Expression classes
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
    Ast* block = nullptr;
    
    QString dump() const override;
};

class IfExpressionAst : public ExpressionAst
{
public:
    IfExpressionAst(Ast* parent);
    
    Ast* condition = nullptr;
    Ast* block = nullptr;
    Ast* orelse = nullptr;
    
    QString dump() const override;
};

// Other AST classes
class AliasAst : public Ast
{
public:
    AliasAst(Ast* parent);
    
    Ast* name = nullptr;
    IdentifierAst* asName = nullptr;
    
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

class CallAst : public ExpressionAst
{
public:
    CallAst(Ast* parent);

    // Function name from children[0]
    // All args from children[1..] - can be Identifier, :: (type annotation), ... (vararg), parameters
    IdentifierAst* name = nullptr;
    // under that everything parameters, named parameters, vararg; kwarg vararg
    QList<Ast*> arguments;
    // The ... vararg arg from children (kind=...)
    // EllipsisAst* vararg = nullptr;
    // The parameters node in children[N] (kind=parameters)
    // ParameterAst* kwarg = nullptr;

    QString dump() const override;
};

class FunctionSignatureAst : public Ast {
public:
    FunctionSignatureAst(Ast* parent);

    Ast* rawSignature = nullptr;                 // Original nested: where → :: → call
    IdentifierAst* name = nullptr;               // Extracted function name
    QList<Ast*> positionalArgs;                  // x, x::T, x=1, x...
    QList<Ast*> keywordArgs;                     // ; kw=1, ...
    Ast* returnType = nullptr;                   // Extracted ::ReturnType
    QList<Ast*> whereConstraints;                // Extracted where constraints

    QString dump() const override;
};

//TODO Function contains Body node, not inherits from it
class FunctionDefinitionAst : public Ast
{
public:
    FunctionDefinitionAst(Ast* parent);

    // Function signature - can be CallAst, or TypeAnnotationAst, or WhereAst
    // WhereAst: children[0]=CallAst/TypeAnnotationAst, children[1..]=<: constraints
    // TypeAnnotationAst: children[0]=CallAst/WhereAst, children[1]=return type
    // JSON: children[0]=signature, children[1]=block
    Ast* raw = nullptr;
    FunctionSignatureAst* signature = nullptr;
    
    // Function block
    BlockAst* block = nullptr;

    QString dump() const override;
};

// CodeAst - Top-level code
class TopLevelAst : public Ast
{
public:
    TopLevelAst();
    ~TopLevelAst();

    // Module block - list of top-level statements (functions, structs, imports, etc.)
    QList<Ast*> children;
    IdentifierAst* name;  // module name

    QString dump() const override;
};


// Type definitions
typedef QSharedPointer<TopLevelAst> CodeAstPtr;

} // namespace Julia

#endif // JULIA_AST_H
