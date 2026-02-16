#include "ast.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QRegularExpression>

namespace Julia {

NodeKind stringToNodeKind(const QString& kindStr)
{
    static const QHash<QString, NodeKind> kindMap = {
        {QStringLiteral("toplevel"), NodeKind::TopLevel},
        {QStringLiteral("block"), NodeKind::Block},
        {QStringLiteral("function"), NodeKind::Function},
        {QStringLiteral("struct"), NodeKind::Struct},
        {QStringLiteral("module"), NodeKind::Module},
        {QStringLiteral("abstract"), NodeKind::Abstract},
        {QStringLiteral("primitive"), NodeKind::Primitive},
        {QStringLiteral("macro"), NodeKind::Macro},
        {QStringLiteral("macrocall"), NodeKind::MacroCall},
        {QStringLiteral("assignment"), NodeKind::Assignment},
        {QStringLiteral("return"), NodeKind::Return},
        {QStringLiteral("if"), NodeKind::If},
        {QStringLiteral("while"), NodeKind::While},
        {QStringLiteral("for"), NodeKind::For},
        {QStringLiteral("break"), NodeKind::Break},
        {QStringLiteral("continue"), NodeKind::Continue},
        {QStringLiteral("call"), NodeKind::Call},
        {QStringLiteral("Identifier"), NodeKind::Identifier},
        {QStringLiteral("string"), NodeKind::String},
        {QStringLiteral("Float"), NodeKind::Float},
        {QStringLiteral("Integer"), NodeKind::Integer},
        {QStringLiteral("Bool"), NodeKind::Bool},
        {QStringLiteral("::"), NodeKind::TypeAnnotation},
        {QStringLiteral("using"), NodeKind::Using},
        {QStringLiteral("import"), NodeKind::Import},
        {QStringLiteral("export"), NodeKind::Export},
        {QStringLiteral("="), NodeKind::Equals},
        {QStringLiteral(":="), NodeKind::ColonEquals},
        {QStringLiteral("."), NodeKind::Dot},
        {QStringLiteral(":"), NodeKind::Colon},
        {QStringLiteral(";"), NodeKind::Semicolon},
        {QStringLiteral(","), NodeKind::Comma},
        {QStringLiteral("comment"), NodeKind::Comment},
        {QStringLiteral("Error"), NodeKind::Error}
    };
    
    return kindMap.value(kindStr, NodeKind::Unknown);
}

QString nodeKindToString(NodeKind kind)
{
    switch (kind) {
        case NodeKind::TopLevel: return QStringLiteral("toplevel");
        case NodeKind::Block: return QStringLiteral("block");
        case NodeKind::Function: return QStringLiteral("function");
        case NodeKind::Struct: return QStringLiteral("struct");
        case NodeKind::Module: return QStringLiteral("module");
        case NodeKind::Abstract: return QStringLiteral("abstract");
        case NodeKind::Primitive: return QStringLiteral("primitive");
        case NodeKind::Macro: return QStringLiteral("macro");
        case NodeKind::MacroCall: return QStringLiteral("macrocall");
        case NodeKind::Assignment: return QStringLiteral("assignment");
        case NodeKind::Return: return QStringLiteral("return");
        case NodeKind::If: return QStringLiteral("if");
        case NodeKind::While: return QStringLiteral("while");
        case NodeKind::For: return QStringLiteral("for");
        case NodeKind::Break: return QStringLiteral("break");
        case NodeKind::Continue: return QStringLiteral("continue");
        case NodeKind::Call: return QStringLiteral("call");
        case NodeKind::Identifier: return QStringLiteral("Identifier");
        case NodeKind::String: return QStringLiteral("string");
        case NodeKind::Float: return QStringLiteral("Float");
        case NodeKind::Integer: return QStringLiteral("Integer");
        case NodeKind::Bool: return QStringLiteral("Bool");
        case NodeKind::TypeAnnotation: return QStringLiteral("::");
        case NodeKind::Using: return QStringLiteral("using");
        case NodeKind::Import: return QStringLiteral("import");
        case NodeKind::Export: return QStringLiteral("export");
        case NodeKind::Equals: return QStringLiteral("=");
        case NodeKind::ColonEquals: return QStringLiteral(":=");
        case NodeKind::Dot: return QStringLiteral(".");
        case NodeKind::Colon: return QStringLiteral(":");
        case NodeKind::Semicolon: return QStringLiteral(";");
        case NodeKind::Comma: return QStringLiteral(",");
        case NodeKind::Comment: return QStringLiteral("comment");
        case NodeKind::Error: return QStringLiteral("Error");
        default: return QStringLiteral("Unknown");
    }
}

AstNode::AstNode(NodeKind kind, const QString& text, const KDevelop::RangeInRevision& range)
    : m_kind(kind)
    , m_text(text)
    , m_range(range)
    , m_isLeaf(true)
    , m_parent(nullptr)
{
}

AstNode::AstNode(NodeKind kind)
    : m_kind(kind)
    , m_isLeaf(false)
    , m_parent(nullptr)
{
}

AstNode::~AstNode()
{
    qDeleteAll(m_children);
}

NodeKind AstNode::kind() const
{
    return m_kind;
}

QString AstNode::text() const
{
    return m_text;
}

KDevelop::RangeInRevision AstNode::range() const
{
    return m_range;
}

bool AstNode::isLeaf() const
{
    return m_isLeaf;
}

void AstNode::setIsLeaf(bool leaf)
{
    m_isLeaf = leaf;
}

QList<AstNode*> AstNode::children() const
{
    return m_children;
}

void AstNode::addChild(AstNode* child)
{
    if (child) {
        m_children.append(child);
        child->setParent(this);
        m_isLeaf = false;
    }
}

AstNode* AstNode::parent() const
{
    return m_parent;
}

void AstNode::setParent(AstNode* parent)
{
    m_parent = parent;
}

AstNode* AstNode::firstChild() const
{
    return m_children.isEmpty() ? nullptr : m_children.first();
}

AstNode* AstNode::lastChild() const
{
    return m_children.isEmpty() ? nullptr : m_children.last();
}

AstNode* AstNode::nextSibling() const
{
    if (!m_parent) {
        return nullptr;
    }
    
    const auto& siblings = m_parent->children();
    int index = siblings.indexOf(const_cast<AstNode*>(this));
    if (index >= 0 && index < siblings.size() - 1) {
        return siblings[index + 1];
    }
    return nullptr;
}

AstNode* AstNode::previousSibling() const
{
    if (!m_parent) {
        return nullptr;
    }
    
    const auto& siblings = m_parent->children();
    int index = siblings.indexOf(const_cast<AstNode*>(this));
    if (index > 0) {
        return siblings[index - 1];
    }
    return nullptr;
}

bool AstNode::isDeclaration() const
{
    return m_kind == NodeKind::Function || 
           m_kind == NodeKind::Struct || 
           m_kind == NodeKind::Module ||
           m_kind == NodeKind::Abstract ||
           m_kind == NodeKind::Primitive ||
           m_kind == NodeKind::Macro;
}

bool AstNode::isExpression() const
{
    return m_kind == NodeKind::Call ||
           m_kind == NodeKind::Identifier ||
           m_kind == NodeKind::String ||
           m_kind == NodeKind::Float ||
           m_kind == NodeKind::Integer ||
           m_kind == NodeKind::Bool ||
           m_kind == NodeKind::Tuple ||
           m_kind == NodeKind::Array ||
           m_kind == NodeKind::Dict;
}

bool AstNode::isStatement() const
{
    return m_kind == NodeKind::Assignment ||
           m_kind == NodeKind::Return ||
           m_kind == NodeKind::If ||
           m_kind == NodeKind::While ||
           m_kind == NodeKind::For ||
           m_kind == NodeKind::Break ||
           m_kind == NodeKind::Continue ||
           m_kind == NodeKind::Call;
}

bool AstNode::isType() const
{
    return m_kind == NodeKind::Struct ||
           m_kind == NodeKind::Abstract ||
           m_kind == NodeKind::Primitive ||
           m_kind == NodeKind::TypeAnnotation;
}

QString AstNode::dump(int indent) const
{
    QString indentStr = QString(QLatin1Char(' ')).repeated(indent * 2);
    QString result = indentStr + nodeKindToString(m_kind);
    
    if (!m_text.isEmpty()) {
        result += QStringLiteral(" (text: %1)").arg(m_text.left(50));
    }
    
    if (!m_range.isEmpty()) {
        result += QStringLiteral(" [%1:%2-%3:%4]")
            .arg(m_range.start.line + 1)
            .arg(m_range.start.column + 1)
            .arg(m_range.end.line + 1)
            .arg(m_range.end.column + 1);
    }
    
    result += QLatin1String("\n");
    
    for (AstNode* child : m_children) {
        result += child->dump(indent + 1);
    }
    
    return result;
}

AstNode* AstNode::fromJson(const QJsonObject& json, AstNode* parent)
{
    QString kindStr = json.value(QLatin1String("kind")).toString();
    NodeKind kind = stringToNodeKind(kindStr);
    
    QString text = json.value(QLatin1String("text")).toString();
    bool isLeaf = json.value(QLatin1String("is_leaf")).toBool();
    
    KDevelop::RangeInRevision range;
    if (json.contains(QLatin1String("range"))) {
        QJsonObject rangeObj = json.value(QLatin1String("range")).toObject();
        int startLine = rangeObj.value(QLatin1String("start_line")).toInt()-1;
        int endLine = rangeObj.value(QLatin1String("end_line")).toInt()-1;
        int startColumn = rangeObj.value(QLatin1String("start_column")).toInt()-1;
        int endColumn = rangeObj.value(QLatin1String("end_column")).toInt()-1;

        range = KDevelop::RangeInRevision(startLine, startColumn, endLine, endColumn);
    }
    
    AstNode* node = nullptr;
    
    switch (kind) {
        case NodeKind::Function:
            node = new FunctionNode(text, range);
            break;
        case NodeKind::Struct:
            node = new StructNode(text, range);
            break;
        case NodeKind::Call:
            node = new CallNode(text, range);
            break;
        case NodeKind::Assignment:
            node = new AssignmentNode(range);
            break;
        case NodeKind::Identifier:
            node = new IdentifierNode(text, range);
            break;
        case NodeKind::String:
            node = new StringNode(text, range);
            break;
        case NodeKind::Float:
        case NodeKind::Integer:
            node = new NumberNode(text, kind == NodeKind::Float, range);
            break;
        default:
            node = new AstNode(kind, text, range);
            break;
    }
    
    node->setParent(parent);
    
    if (!isLeaf && json.contains(QLatin1String("children"))) {
        QJsonArray children = json.value(QLatin1String("children")).toArray();
        for (const QJsonValue& childValue : children) {
            if (childValue.isObject()) {
                AstNode* childNode = fromJson(childValue.toObject(), node);
                if (childNode) {
                    node->addChild(childNode);
                }
            }
        }
    }
    
    return node;
}

AstNode* AstNode::parseJson(const QByteArray& json)
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
    
    return fromJson(doc.object());
}

void AstNode::parseChildren(const QJsonArray& children)
{
    for (const QJsonValue& childValue : children) {
        if (childValue.isObject()) {
            AstNode* child = fromJson(childValue.toObject(), this);
            if (child) {
                addChild(child);
            }
        }
    }
}

FunctionNode::FunctionNode(const QString& name, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Function, name, range)
{
    setIsLeaf(false);
}

QString FunctionNode::functionName() const
{
    if (children().isEmpty()) return QString();
    
    AstNode* header = firstChild();
    if (!header) return QString();
    
    for (AstNode* child : header->children()) {
        if (child->kind() == NodeKind::Identifier) {
            return child->text();
        }
    }
    
    return QString();
}

AstNode* FunctionNode::arguments() const
{
    if (children().isEmpty()) return nullptr;
    
    AstNode* header = firstChild();
    if (!header) return nullptr;
    
    for (AstNode* child : header->children()) {
        if (child->kind() == NodeKind::TypeAnnotation || child->kind() == NodeKind::Call) {
            return child;
        }
    }
    
    return nullptr;
}

AstNode* FunctionNode::body() const
{
    if (children().size() < 2) return nullptr;
    return children().at(1);
}

AstNode* FunctionNode::returnType() const
{
    AstNode* argsNode = arguments();
    if (!argsNode || argsNode->kind() != NodeKind::TypeAnnotation) return nullptr;
    
    const auto& children = argsNode->children();
    if (children.size() < 2) return nullptr;
    
    return children.at(1);
}

bool FunctionNode::hasReturnType() const
{
    return returnType() != nullptr;
}

int FunctionNode::argumentCount() const
{
    AstNode* argsNode = arguments();
    if (!argsNode) return 0;
    
    return argsNode->children().size();
}

QString FunctionNode::dump(int indent) const
{
    QString indentStr = QString(QLatin1Char(' ')).repeated(indent * 2);
    QString result = indentStr + QLatin1String("Function");
    
    QString name = functionName();
    if (!name.isEmpty()) {
        result += QLatin1String(" ") + name;
    }
    
    if (hasReturnType()) {
        result += QLatin1String(" -> ") + returnType()->text();
    }
    
    result += QLatin1String("\n");
    
    for (AstNode* child : children()) {
        result += child->dump(indent + 1);
    }
    
    return result;
}

StructNode::StructNode(const QString& name, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Struct, name, range)
{
    setIsLeaf(false);
}

QString StructNode::structName() const
{
    if (children().isEmpty()) return QString();
    
    AstNode* nameNode = firstChild();
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        return nameNode->text();
    }
    
    return QString();
}

QList<AstNode*> StructNode::fields() const
{
    QList<AstNode*> result;
    
    AstNode* block = lastChild();
    if (!block) return result;
    
    for (AstNode* child : block->children()) {
        if (child->kind() == NodeKind::TypeAnnotation) {
            result.append(child);
        }
    }
    
    return result;
}

QList<AstNode*> StructNode::supertypes() const
{
    QList<AstNode*> result;
    return result;
}

QString StructNode::dump(int indent) const
{
    QString indentStr = QString(QLatin1Char(' ')).repeated(indent * 2);
    QString result = indentStr + QLatin1String("Struct");
    
    QString name = structName();
    if (!name.isEmpty()) {
        result += QLatin1String(" ") + name;
    }
    
    result += QLatin1String("\n");
    
    for (AstNode* child : children()) {
        result += child->dump(indent + 1);
    }
    
    return result;
}

CallNode::CallNode(const QString& functionName, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Call, functionName, range)
{
    setIsLeaf(false);
}

QString CallNode::functionName() const
{
    if (children().isEmpty()) return QString();
    
    AstNode* nameNode = firstChild();
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        return nameNode->text();
    }
    
    return text();
}

QList<AstNode*> CallNode::arguments() const
{
    if (children().size() <= 1) return QList<AstNode*>();
    
    QList<AstNode*> result;
    for (int i = 1; i < children().size(); ++i) {
        result.append(children().at(i));
    }
    return result;
}

int CallNode::argumentCount() const
{
    return qMax(0, children().size() - 1);
}

bool CallNode::isFunctionCall() const
{
    return !children().isEmpty() && 
           firstChild()->kind() == NodeKind::Identifier;
}

bool CallNode::isMacroCall() const
{
    return false;
}

QString CallNode::dump(int indent) const
{
    QString indentStr = QString(QLatin1Char(' ')).repeated(indent * 2);
    QString result = indentStr + QLatin1String("Call");
    
    QString name = functionName();
    if (!name.isEmpty()) {
        result += QLatin1String(" ") + name;
    }
    
    result += QStringLiteral(" (%1 args)\n").arg(argumentCount());
    
    for (AstNode* child : children()) {
        result += child->dump(indent + 1);
    }
    
    return result;
}

AssignmentNode::AssignmentNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Assignment, QString(), range)
{
    setIsLeaf(false);
}

AstNode* AssignmentNode::leftHandSide() const
{
    if (children().isEmpty()) return nullptr;
    return firstChild();
}

AstNode* AssignmentNode::rightHandSide() const
{
    if (children().size() < 2) return nullptr;
    return children().at(1);
}

QString AssignmentNode::dump(int indent) const
{
    QString indentStr = QString(QLatin1Char(' ')).repeated(indent * 2);
    QString result = indentStr + QLatin1String("Assignment\n");
    
    for (AstNode* child : children()) {
        result += child->dump(indent + 1);
    }
    
    return result;
}

IdentifierNode::IdentifierNode(const QString& name, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Identifier, name, range)
{
}

QString IdentifierNode::identifier() const
{
    return text();
}

QString IdentifierNode::dump(int indent) const
{
    QString indentStr = QString(QLatin1Char(' ')).repeated(indent * 2);
    return indentStr + QLatin1String("Identifier: ") + text() + QLatin1String("\n");
}

StringNode::StringNode(const QString& value, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::String, value, range)
{
}

QString StringNode::value() const
{
    return text();
}

QString StringNode::dump(int indent) const
{
    QString indentStr = QString(QLatin1Char(' ')).repeated(indent * 2);
    return indentStr + QLatin1String("String: \"") + text() + QLatin1String("\"\n");
}

NumberNode::NumberNode(const QString& value, bool isFloat, const KDevelop::RangeInRevision& range)
    : AstNode(isFloat ? NodeKind::Float : NodeKind::Integer, value, range)
    , m_isFloat(isFloat)
{
}

QString NumberNode::value() const
{
    return text();
}

bool NumberNode::isFloat() const
{
    return m_isFloat;
}

bool NumberNode::isInteger() const
{
    return !m_isFloat;
}

double NumberNode::asFloat() const
{
    return text().toDouble();
}

qint64 NumberNode::asInteger() const
{
    return text().toLongLong();
}

QString NumberNode::dump(int indent) const
{
    QString indentStr = QString(QLatin1Char(' ')).repeated(indent * 2);
    return indentStr + (m_isFloat ? QLatin1String("Float: ") : QLatin1String("Integer: ")) + text() + QLatin1String("\n");
}

}
