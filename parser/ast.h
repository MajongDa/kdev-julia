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

enum class NodeKind {
    TopLevel,
    Block,
    
    Function,
    Struct,
    Module,
    Abstract,
    Primitive,
    Macro,
    MacroCall,
    
    Assignment,
    Return,
    If,
    While,
    For,
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
    
    Equals,
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
    
    virtual QString dump(int indent = 0) const;
    
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

class FunctionNode : public AstNode
{
public:
    FunctionNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QString functionName() const;
    AstNode* arguments() const;
    AstNode* body() const;
    AstNode* returnType() const;
    
    bool hasReturnType() const;
    int argumentCount() const;
    QList<AstNode*> parameters() const;
    QList<AstNode*> typeParameters() const;
    
    QString dump(int indent = 0) const override;
};

class StructNode : public AstNode
{
public:
    StructNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QString structName() const;
    QList<AstNode*> fields() const;
    QList<AstNode*> supertypes() const;
    
    QString dump(int indent = 0) const override;
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
    
    QString dump(int indent = 0) const override;
};

class CurlyNode : public AstNode
{
public:
    CurlyNode(const QString& functionName, const KDevelop::RangeInRevision& range);
    
    QString functionName() const;
    QList<AstNode*> arguments() const;
    
    int argumentCount() const;
    
    QString dump(int indent = 0) const override;
};

class AssignmentNode : public AstNode
{
public:
    AssignmentNode(const KDevelop::RangeInRevision& range);
    
    AstNode* leftHandSide() const;
    AstNode* rightHandSide() const;
    
    QString dump(int indent = 0) const override;
};

class IdentifierNode : public AstNode
{
public:
    IdentifierNode(const QString& name, const KDevelop::RangeInRevision& range);
    
    QString identifier() const;
    
    QString dump(int indent = 0) const override;
};

class StringNode : public AstNode
{
public:
    StringNode(const QString& value, const KDevelop::RangeInRevision& range);
    
    QString value() const;
    
    QString dump(int indent = 0) const override;
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
    
    QString dump(int indent = 0) const override;
    
private:
    bool m_isFloat;
};

}

#endif
