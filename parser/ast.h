#ifndef JULIA_AST_H
#define JULIA_AST_H

#include <QList>
#include <QString>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <language/editor/rangeinrevision.h>

namespace KDevelop { class DUContext; }

namespace Julia {

class AstNode;
class AstVisitor;

// Forward declarations for class-based AST nodes (Python-style)
class FunctionDefinitionAst;
class ReturnValueAst;
class NameReferenceAst;
class AssignmentValueAst;
class BlockAst;
class IfBranchAst;
class WhileLoopAst;
class ForLoopAst;
class TryCatchAst;

enum class NodeKind {
    TopLevel,
    Block,
    
    Function,
    Struct,
    Module,
    Baremodule,
    Abstract,
    Primitive,
    Macro,
    MacroCall,
    
    Assignment,
    Return,
    If,
    ElseIf,
    Else,
    While,
    For,
    Try,
    Catch,
    Finally,
    Break,
    Continue,
    
    Call,
    Curly,
    Where,
    Parameters,
    Identifier,
    String,
    Float,
    Integer,
    Bool,
    Operator,
    Tuple,
    Array,
    Dict,
    Ref,
    Vect,
    Generator,
    ImportPath,
    MacroName,
    
    TypeAnnotation,
    
    Using,
    Import,
    Export,
    
    Const,
    Global,
    Local,
    Let,
    Do,
    Quote,
    End,
    
    ColonEquals,
    Dot,
    Colon,
    Semicolon,
    Comma,
    
    Comment,
    Whitespace,
    Newline,
    Error,
    Unknown
};

NodeKind stringToNodeKind(const QString& kindStr);
QString nodeKindToString(NodeKind kind);

class AstNode
{
public:
    AstNode(NodeKind kind, const QString& text, const KDevelop::RangeInRevision& range);
    virtual ~AstNode();
    
    NodeKind kind() const;
    QString text() const;
    KDevelop::RangeInRevision range() const;
    
    bool isLeaf() const;
    void setIsLeaf(bool leaf);
    QList<AstNode*> children() const;
    void addChild(AstNode* child);
    AstNode* parent() const;
    void setParent(AstNode* parent);
    
    AstNode* firstChild() const;
    AstNode* lastChild() const;
    AstNode* nextSibling() const;
    AstNode* previousSibling() const;
    
    bool isDeclaration() const;
    bool isExpression() const;
    bool isStatement() const;
    bool isType() const;
    
    virtual QString dump() const;
    
    void accept(AstVisitor* visitor);
    
    static AstNode* fromJson(const QJsonObject& json, AstNode* parent = nullptr);
    static AstNode* parseJson(const QByteArray& json);

    KDevelop::DUContext* context = nullptr;

protected:
    AstNode(NodeKind kind);
    
private:
    NodeKind m_kind;
    QString m_text;
    KDevelop::RangeInRevision m_range;
    bool m_isLeaf;
    QList<AstNode*> m_children;
    AstNode* m_parent;
    
    void parseChildren(const QJsonArray& children);
};

class FunctionDefinitionAst : public AstNode
{
public:
    FunctionDefinitionAst(const QString& name, const KDevelop::RangeInRevision& range);
    
    QString functionName() const;
    AstNode* functionNameNode() const;
    AstNode* callNode() const;
    AstNode* body() const;
    AstNode* returnType() const;
    
    bool hasReturnType() const;
    int argumentCount() const;
    QList<AstNode*> parameters() const;
    QList<AstNode*> typeParameters() const;
    
    QString dump() const override;
};

class ReturnValueAst : public AstNode
{
public:
    ReturnValueAst(const KDevelop::RangeInRevision& range);
    
    AstNode* value() const;
    
    QString dump() const override;
};

class NameReferenceAst : public AstNode
{
public:
    NameReferenceAst(const QString& name, const KDevelop::RangeInRevision& range);
    
    enum class Context { Load = 1, Store = 2, Invalid = -1 };
    
    QString identifier() const;
    Context context() const;
    void setContext(Context c);
    
    QString dump() const override;
    
private:
    Context m_context;
};

class AssignmentValueAst : public AstNode
{
public:
    AssignmentValueAst(const KDevelop::RangeInRevision& range);
    
    AstNode* leftHandSide() const;
    AstNode* rightHandSide() const;
    
    QString dump() const override;
};

class BlockAst : public AstNode
{
public:
    BlockAst(const KDevelop::RangeInRevision& range);
    
    QList<AstNode*> statements() const;
    
    QString dump() const override;
};

class IfBranchAst : public AstNode
{
public:
    IfBranchAst(const KDevelop::RangeInRevision& range);
    
    AstNode* condition() const;
    AstNode* thenBranch() const;
    AstNode* elseBranch() const;
    QList<AstNode*> elseifBranches() const;
    
    QString dump() const override;
};

class WhileLoopAst : public AstNode
{
public:
    WhileLoopAst(const KDevelop::RangeInRevision& range);
    
    AstNode* condition() const;
    AstNode* body() const;
    
    QString dump() const override;
};

class ForLoopAst : public AstNode
{
public:
    ForLoopAst(const KDevelop::RangeInRevision& range);
    
    AstNode* iterator() const;
    AstNode* body() const;
    
    QString dump() const override;
};

class TryCatchAst : public AstNode
{
public:
    TryCatchAst(const KDevelop::RangeInRevision& range);
    
    AstNode* tryBody() const;
    AstNode* catchVariable() const;
    AstNode* catchBody() const;
    AstNode* finallyBody() const;
    
    QString dump() const override;
};

class ParametersNode : public AstNode
{
public:
    ParametersNode(const KDevelop::RangeInRevision& range);

    QList<AstNode*> parameters() const;

    QString dump() const override;
};

class FunctionNode : public AstNode
{
public:
    FunctionNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QString functionName() const;
    AstNode* functionNameNode() const;
    // callNode() returns the Call node representing the function signature (name + positional args)
    // This is the same node kind as regular function calls, but context determines it's a function definition
    AstNode* callNode() const;
    AstNode* body() const;
    AstNode* returnType() const;
    
    bool hasReturnType() const;
    int argumentCount() const;
    // parameters() extracts typed parameters (TypeAnnotation nodes) from the callNode's children
    QList<AstNode*> parameters() const;
    QList<AstNode*> typeParameters() const;
    
    QString dump() const override;
};

class ParametersNode;
class StructNode : public AstNode
{
public:
    StructNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QString structName() const;
    QList<AstNode*> fields() const;
    QList<AstNode*> supertypes() const;
    
    QString dump() const override;
};

class CallNode : public AstNode
{
public:
    CallNode(const QString& functionName, const KDevelop::RangeInRevision& range);
    
    QString functionName() const;
    QList<AstNode*> arguments() const;
    
    int argumentCount() const;
    bool isFunctionCall() const;
    bool isMacroCall() const;
    
    QString dump() const override;
};

class CurlyNode : public AstNode
{
public:
    CurlyNode(const QString& functionName, const KDevelop::RangeInRevision& range);
    
    QString functionName() const;
    QList<AstNode*> arguments() const;
    
    int argumentCount() const;
    
    QString dump() const override;
};

class AssignmentNode : public AstNode
{
public:
    AssignmentNode(const KDevelop::RangeInRevision& range);
    
    AstNode* leftHandSide() const;
    AstNode* rightHandSide() const;
    
    QString dump() const override;
};

class IdentifierNode : public AstNode
{
public:
    IdentifierNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QString identifier() const;
    
    QString dump() const override;
};

class StringNode : public AstNode
{
public:
    StringNode(const QString& value, const KDevelop::RangeInRevision& range);
    
    QString value() const;
    
    QString dump() const override;
};

class NumberNode : public AstNode
{
public:
    NumberNode(const QString& value, bool isFloat, const KDevelop::RangeInRevision& range);
    
    QString value() const;
    bool isFloat() const;
    bool isInteger() const;
    double asFloat() const;
    qint64 asInteger() const;
    
    QString dump() const override;
    
private:
    bool m_isFloat;
};

class TupleNode : public AstNode
{
public:
    TupleNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QList<AstNode*> elements() const;
    
    QString dump() const override;
};

class ArrayNode : public AstNode
{
public:
    ArrayNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QList<AstNode*> elements() const;
    
    QString dump() const override;
};



class TryNode : public AstNode
{
public:
    TryNode(const KDevelop::RangeInRevision& range);
    
    AstNode* tryBody() const;
    AstNode* catchVariable() const;
    AstNode* catchBody() const;
    AstNode* finallyBody() const;
    
    QString dump() const override;
};

class ConstNode : public AstNode
{
public:
    ConstNode(const KDevelop::RangeInRevision& range);
    
    AstNode* target() const;
    AstNode* value() const;
    
    QString dump() const override;
};

class LetNode : public AstNode
{
public:
    LetNode(const KDevelop::RangeInRevision& range);
    
    QList<AstNode*> bindings() const;
    AstNode* body() const;
    
    QString dump() const override;
};

class DoNode : public AstNode
{
public:
    DoNode(const KDevelop::RangeInRevision& range);
    
    QList<AstNode*> arguments() const;
    AstNode* body() const;
    
    QString dump() const override;
};

class QuoteNode : public AstNode
{
public:
    QuoteNode(const KDevelop::RangeInRevision& range);
    
    QList<AstNode*> body() const;
    
    QString dump() const override;
};

class GlobalNode : public AstNode
{
public:
    GlobalNode(const KDevelop::RangeInRevision& range);
    
    QList<AstNode*> identifiers() const;
    
    QString dump() const override;
};

class LocalNode : public AstNode
{
public:
    LocalNode(const KDevelop::RangeInRevision& range);
    
    QList<AstNode*> identifiers() const;
    
    QString dump() const override;
};

class BaremoduleNode : public AstNode
{
public:
    BaremoduleNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QString moduleName() const;
    AstNode* body() const;
    
    QString dump() const override;
};

class ModuleNode : public AstNode
{
public:
    ModuleNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QString moduleName() const;
    AstNode* body() const;
    
    QString dump() const override;
};

class BeginNode : public AstNode
{
public:
    BeginNode(const KDevelop::RangeInRevision& range);
    
    AstNode* body() const;
    
    QString dump() const override;
};

class BreakNode : public AstNode
{
public:
    BreakNode(const KDevelop::RangeInRevision& range);
    
    QString dump() const override;
};

class ContinueNode : public AstNode
{
public:
    ContinueNode(const KDevelop::RangeInRevision& range);
    
    QString dump() const override;
};

class ReturnNode : public AstNode
{
public:
    ReturnNode(const KDevelop::RangeInRevision& range);
    
    AstNode* value() const;
    
    QString dump() const override;
};

class WhileNode : public AstNode
{
public:
    WhileNode(const KDevelop::RangeInRevision& range);
    
    AstNode* condition() const;
    AstNode* body() const;
    
    QString dump() const override;
};

class ForNode : public AstNode
{
public:
    ForNode(const KDevelop::RangeInRevision& range);
    
    AstNode* iterator() const;
    AstNode* body() const;
    
    QString dump() const override;
};

class IfNode : public AstNode
{
public:
    IfNode(const KDevelop::RangeInRevision& range);
    
    AstNode* condition() const;
    AstNode* thenBranch() const;
    AstNode* elseBranch() const;
    QList<AstNode*> elseifBranches() const;
    
    QString dump() const override;
};

class ElseIfNode : public AstNode
{
public:
    ElseIfNode(const KDevelop::RangeInRevision& range);
    
    AstNode* condition() const;
    AstNode* body() const;
    
    QString dump() const override;
};

class ElseNode : public AstNode
{
public:
    ElseNode(const KDevelop::RangeInRevision& range);
    
    AstNode* body() const;
    
    QString dump() const override;
};

class EndNode : public AstNode
{
public:
    EndNode(const KDevelop::RangeInRevision& range);
    
    QString dump() const override;
};

class ExportNode : public AstNode
{
public:
    ExportNode(const KDevelop::RangeInRevision& range);
    
    QList<AstNode*> identifiers() const;
    
    QString dump() const override;
};

class ImportNode : public AstNode
{
public:
    ImportNode(const KDevelop::RangeInRevision& range);
    
    QList<AstNode*> importPaths() const;
    
    QString dump() const override;
};

class MacroNode : public AstNode
{
public:
    MacroNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QString macroName() const;
    AstNode* parameters() const;
    AstNode* body() const;
    
    QString dump() const override;
};

class AbstractNode : public AstNode
{
public:
    AbstractNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QString typeName() const;
    AstNode* supertype() const;
    
    QString dump() const override;
};

class PrimitiveNode : public AstNode
{
public:
    PrimitiveNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QString typeName() const;
    AstNode* underlyingType() const;
    
    QString dump() const override;
};

class CatchNode : public AstNode
{
public:
    CatchNode(const KDevelop::RangeInRevision& range);
    
    AstNode* variable() const;
    AstNode* body() const;
    
    QString dump() const override;
};

class FinallyNode : public AstNode
{
public:
    FinallyNode(const KDevelop::RangeInRevision& range);
    
    AstNode* body() const;
    
    QString dump() const override;
};

class AsNode : public AstNode
{
public:
    AsNode(const KDevelop::RangeInRevision& range);
    
    AstNode* original() const;
    AstNode* alias() const;
    
    QString dump() const override;
};

class DocNode : public AstNode
{
public:
    DocNode(const KDevelop::RangeInRevision& range);
    
    AstNode* document() const;
    
    QString dump() const override;
};

class MutableNode : public AstNode
{
public:
    MutableNode(const KDevelop::RangeInRevision& range);
    
    QString dump() const override;
};

class OuterNode : public AstNode
{
public:
    OuterNode(const KDevelop::RangeInRevision& range);
    
    QString dump() const override;
};

class PublicNode : public AstNode
{
public:
    PublicNode(const KDevelop::RangeInRevision& range);
    
    QString dump() const override;
};

class VarNode : public AstNode
{
public:
    VarNode(const KDevelop::RangeInRevision& range);
    
    QString dump() const override;
};

class TypeNode : public AstNode
{
public:
    TypeNode(const KDevelop::RangeInRevision& range);
    
    QString dump() const override;
};

}

#endif
