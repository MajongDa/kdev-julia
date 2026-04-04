#include "ast.h"
#include "../duchain/astvisitor.h"
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
        {QStringLiteral("baremodule"), NodeKind::Baremodule},
        {QStringLiteral("abstract"), NodeKind::Abstract},
        {QStringLiteral("primitive"), NodeKind::Primitive},
        {QStringLiteral("macro"), NodeKind::Macro},
        {QStringLiteral("macrocall"), NodeKind::MacroCall},
        {QStringLiteral("assignment"), NodeKind::Assignment},
        {QStringLiteral("return"), NodeKind::Return},
        {QStringLiteral("if"), NodeKind::If},
        {QStringLiteral("elseif"), NodeKind::ElseIf},
        {QStringLiteral("else"), NodeKind::Else},
        {QStringLiteral("while"), NodeKind::While},
        {QStringLiteral("for"), NodeKind::For},
        {QStringLiteral("try"), NodeKind::Try},
        {QStringLiteral("catch"), NodeKind::Catch},
        {QStringLiteral("finally"), NodeKind::Finally},
        {QStringLiteral("break"), NodeKind::Break},
        {QStringLiteral("continue"), NodeKind::Continue},
        {QStringLiteral("call"), NodeKind::Call},
        {QStringLiteral("curly"), NodeKind::Curly},
        {QStringLiteral("where"), NodeKind::Where},
        {QStringLiteral("parameters"), NodeKind::Parameters},
        {QStringLiteral("Identifier"), NodeKind::Identifier},
        {QStringLiteral("string"), NodeKind::String},
        {QStringLiteral("Float"), NodeKind::Float},
        {QStringLiteral("Integer"), NodeKind::Integer},
        {QStringLiteral("Bool"), NodeKind::Bool},
        {QStringLiteral("Operator"), NodeKind::Operator},
        {QStringLiteral("::"), NodeKind::TypeAnnotation},
        {QStringLiteral("using"), NodeKind::Using},
        {QStringLiteral("import"), NodeKind::Import},
        {QStringLiteral("export"), NodeKind::Export},
        {QStringLiteral("="), NodeKind::Assignment},
        {QStringLiteral("ref"), NodeKind::Ref},
        {QStringLiteral("vect"), NodeKind::Vect},
        {QStringLiteral("generator"), NodeKind::Generator},
        {QStringLiteral("comprehension"), NodeKind::Generator},
        {QStringLiteral("importpath"), NodeKind::ImportPath},
        {QStringLiteral("macro_name"), NodeKind::MacroName},
        {QStringLiteral(":="), NodeKind::ColonEquals},
        {QStringLiteral("."), NodeKind::Dot},
        {QStringLiteral(":"), NodeKind::Colon},
        {QStringLiteral(";"), NodeKind::Semicolon},
        {QStringLiteral(","), NodeKind::Comma},
        {QStringLiteral("comment"), NodeKind::Comment},
        {QStringLiteral("Error"), NodeKind::Error},
        // Additional keywords
        {QStringLiteral("const"), NodeKind::Const},
        {QStringLiteral("global"), NodeKind::Global},
        {QStringLiteral("local"), NodeKind::Local},
        {QStringLiteral("let"), NodeKind::Let},
        {QStringLiteral("do"), NodeKind::Do},
        {QStringLiteral("quote"), NodeKind::Quote},
        {QStringLiteral("end"), NodeKind::End},
        // Contextual keywords
        {QStringLiteral("abstract"), NodeKind::Abstract},
        {QStringLiteral("as"), NodeKind::Identifier},
        {QStringLiteral("mutable"), NodeKind::Identifier},
        {QStringLiteral("outer"), NodeKind::Identifier},
        {QStringLiteral("primitive"), NodeKind::Primitive},
        {QStringLiteral("public"), NodeKind::Identifier},
        {QStringLiteral("var"), NodeKind::Identifier},
        {QStringLiteral("type"), NodeKind::Identifier},
        // Legacy mappings (for compatibility)
        {QStringLiteral("begin"), NodeKind::Block},
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
        case NodeKind::Baremodule: return QStringLiteral("baremodule");
        case NodeKind::Abstract: return QStringLiteral("abstract");
        case NodeKind::Primitive: return QStringLiteral("primitive");
        case NodeKind::Macro: return QStringLiteral("macro");
        case NodeKind::MacroCall: return QStringLiteral("macrocall");
        case NodeKind::Assignment: return QStringLiteral("=");
        case NodeKind::Return: return QStringLiteral("return");
        case NodeKind::If: return QStringLiteral("if");
        case NodeKind::ElseIf: return QStringLiteral("elseif");
        case NodeKind::Else: return QStringLiteral("else");
        case NodeKind::While: return QStringLiteral("while");
        case NodeKind::For: return QStringLiteral("for");
        case NodeKind::Try: return QStringLiteral("try");
        case NodeKind::Catch: return QStringLiteral("catch");
        case NodeKind::Finally: return QStringLiteral("finally");
        case NodeKind::Break: return QStringLiteral("break");
        case NodeKind::Continue: return QStringLiteral("continue");
        case NodeKind::Call: return QStringLiteral("call");
        case NodeKind::Curly: return QStringLiteral("curly");
        case NodeKind::Where: return QStringLiteral("where");
        case NodeKind::Parameters: return QStringLiteral("parameters");
        case NodeKind::Identifier: return QStringLiteral("Identifier");
        case NodeKind::String: return QStringLiteral("string");
        case NodeKind::Float: return QStringLiteral("Float");
        case NodeKind::Integer: return QStringLiteral("Integer");
        case NodeKind::Bool: return QStringLiteral("Bool");
        case NodeKind::TypeAnnotation: return QStringLiteral("::");
        case NodeKind::Using: return QStringLiteral("using");
        case NodeKind::Import: return QStringLiteral("import");
        case NodeKind::Export: return QStringLiteral("export");
        case NodeKind::ColonEquals: return QStringLiteral(":=");
        case NodeKind::Dot: return QStringLiteral(".");
        case NodeKind::Colon: return QStringLiteral(":");
        case NodeKind::Semicolon: return QStringLiteral(";");
        case NodeKind::Comma: return QStringLiteral(",");
        case NodeKind::Comment: return QStringLiteral("comment");
        case NodeKind::Whitespace: return QStringLiteral("Whitespace");
        case NodeKind::Newline: return QStringLiteral("NewlineWs");
        case NodeKind::Error: return QStringLiteral("Error");
        case NodeKind::Operator: return QStringLiteral("Operator");
        case NodeKind::Tuple: return QStringLiteral("tuple");
        case NodeKind::Array: return QStringLiteral("Array");
        case NodeKind::Dict: return QStringLiteral("Dict");
        case NodeKind::Ref: return QStringLiteral("ref");
        case NodeKind::Vect: return QStringLiteral("vect");
        case NodeKind::Generator: return QStringLiteral("generator");
        case NodeKind::ImportPath: return QStringLiteral("importpath");
        case NodeKind::MacroName: return QStringLiteral("macro_name");
        case NodeKind::Const: return QStringLiteral("const");
        case NodeKind::Global: return QStringLiteral("global");
        case NodeKind::Local: return QStringLiteral("local");
        case NodeKind::Let: return QStringLiteral("let");
        case NodeKind::Do: return QStringLiteral("do");
        case NodeKind::Quote: return QStringLiteral("quote");
        case NodeKind::End: return QStringLiteral("end");
        case NodeKind::Unknown: return QStringLiteral("Unknown");
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

void AstNode::accept(AstVisitor* visitor)
{
    switch (m_kind) {
        case NodeKind::TopLevel: visitor->visitTopLevel(this); break;
        case NodeKind::Block: visitor->visitBlock(this); break;
        case NodeKind::Function: visitor->visitFunction(static_cast<FunctionNode*>(this)); break;
        case NodeKind::Struct: visitor->visitStruct(static_cast<StructNode*>(this)); break;
        case NodeKind::Module: visitor->visitModule(this); break;
        case NodeKind::Baremodule: visitor->visitBaremodule(static_cast<BaremoduleNode*>(this)); break;
        case NodeKind::Abstract: visitor->visitAbstract(this); break;
        case NodeKind::Primitive: visitor->visitPrimitive(this); break;
        case NodeKind::Macro: visitor->visitMacroCall(this); break;
        case NodeKind::MacroCall: visitor->visitMacroCall(this); break;
        case NodeKind::Return: visitor->visitReturn(static_cast<ReturnNode*>(this)); break;
        case NodeKind::If: visitor->visitIf(this); break;
        case NodeKind::ElseIf: visitor->visitElseIf(this); break;
        case NodeKind::Else: visitor->visitElse(this); break;
        case NodeKind::While: visitor->visitWhile(static_cast<WhileNode*>(this)); break;
        case NodeKind::For: visitor->visitFor(static_cast<ForNode*>(this)); break;
        case NodeKind::Try: visitor->visitTry(static_cast<TryNode*>(this)); break;
        case NodeKind::Catch: visitor->visitCatch(this); break;
        case NodeKind::Finally: visitor->visitFinally(this); break;
        case NodeKind::Break: visitor->visitBreak(this); break;
        case NodeKind::Continue: visitor->visitContinue(this); break;
        case NodeKind::Call: visitor->visitCall(static_cast<CallNode*>(this)); break;
        case NodeKind::Curly: visitor->visitCurly(static_cast<CurlyNode*>(this)); break;
        case NodeKind::Where: visitor->visitWhere(this); break;
        case NodeKind::Identifier: visitor->visitIdentifier(this); break;
        case NodeKind::String: visitor->visitString(this); break;
        case NodeKind::Float: visitor->visitFloat(this); break;
        case NodeKind::Integer: visitor->visitInteger(this); break;
        case NodeKind::Bool: visitor->visitBool(this); break;
        case NodeKind::Operator: visitor->visitOperator(this); break;
        case NodeKind::Tuple: visitor->visitTuple(this); break;
        case NodeKind::Array: visitor->visitArray(this); break;
        case NodeKind::Dict: visitor->visitDict(this); break;
        case NodeKind::Ref: visitor->visitRef(this); break;
        case NodeKind::Vect: visitor->visitVect(this); break;
        case NodeKind::Generator: visitor->visitGenerator(this); break;
        case NodeKind::ImportPath: visitor->visitImportPath(this); break;
        case NodeKind::MacroName: visitor->visitMacroName(this); break;
        case NodeKind::TypeAnnotation: visitor->visitTypeAnnotation(this); break;
        case NodeKind::Using: visitor->visitUsing(this); break;
        case NodeKind::Import: visitor->visitImport(this); break;
        case NodeKind::Export: visitor->visitExport(this); break;
        case NodeKind::Assignment:
            visitor->visitAssignment(static_cast<AssignmentNode*>(this));
            break;
        case NodeKind::Const: visitor->visitConst(static_cast<ConstNode*>(this)); break;
        case NodeKind::Global: visitor->visitGlobal(static_cast<GlobalNode*>(this)); break;
        case NodeKind::Local: visitor->visitLocal(static_cast<LocalNode*>(this)); break;
        case NodeKind::Let: visitor->visitLet(static_cast<LetNode*>(this)); break;
        case NodeKind::Do: visitor->visitDo(static_cast<DoNode*>(this)); break;
        case NodeKind::Quote: visitor->visitQuote(static_cast<QuoteNode*>(this)); break;
        case NodeKind::End: visitor->visitEnd(this); break;
        case NodeKind::ColonEquals: visitor->visitColonEquals(this); break;
        case NodeKind::Dot: visitor->visitDot(this); break;
        case NodeKind::Colon: visitor->visitColon(this); break;
        case NodeKind::Semicolon: visitor->visitSemicolon(this); break;
        case NodeKind::Comma: visitor->visitComma(this); break;
        case NodeKind::Comment: visitor->visitComment(this); break;
        case NodeKind::Whitespace: visitor->visitWhitespace(this); break;
        case NodeKind::Newline: visitor->visitNewline(this); break;
        case NodeKind::Error: visitor->visitError(this); break;
        case NodeKind::Unknown: visitor->visitUnknown(this); break;
        default:
            // Handle any unhandled node kinds by visiting as generic node
            visitor->visitNode(this);
            break;
    }
}

QString AstNode::dump() const
{
    QString result = nodeKindToString(m_kind);
    
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
        result += child->dump();
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
        case NodeKind::Module:
        case NodeKind::Baremodule:
            node = new BaremoduleNode(text, range);
            break;
        case NodeKind::Call:
            node = new CallNode(text, range);
            break;
        case NodeKind::Curly:
            node = new CurlyNode(text, range);
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
        case NodeKind::Tuple:
            node = new TupleNode(text, range);
            break;
        case NodeKind::Array:
            node = new ArrayNode(text, range);
            break;
        case NodeKind::Parameters:
            node = new ParametersNode(range);
            break;
        case NodeKind::Try:
            node = new TryNode(range);
            break;
        case NodeKind::Const:
            node = new ConstNode(range);
            break;
        case NodeKind::Let:
            node = new LetNode(range);
            break;
        case NodeKind::Do:
            node = new DoNode(range);
            break;
        case NodeKind::Quote:
            node = new QuoteNode(range);
            break;
        case NodeKind::Global:
            node = new GlobalNode(range);
            break;
        case NodeKind::Local:
            node = new LocalNode(range);
            break;
        case NodeKind::Return:
            node = new ReturnNode(range);
            break;
        case NodeKind::While:
            node = new WhileNode(range);
            break;
        case NodeKind::For:
            node = new ForNode(range);
            break;
        case NodeKind::If:
            node = new IfNode(range);
            break;
        case NodeKind::ElseIf:
            node = new ElseIfNode(range);
            break;
        case NodeKind::Else:
            node = new ElseNode(range);
            break;
        case NodeKind::End:
            node = new EndNode(range);
            break;
        case NodeKind::Export:
            node = new ExportNode(range);
            break;
        case NodeKind::Import:
            node = new ImportNode(range);
            break;
        case NodeKind::Macro:
            node = new MacroNode(text, range);
            break;
        case NodeKind::Abstract:
            node = new AbstractNode(text, range);
            break;
        case NodeKind::Primitive:
            node = new PrimitiveNode(text, range);
            break;
        case NodeKind::Catch:
            node = new CatchNode(range);
            break;
        case NodeKind::Finally:
            node = new FinallyNode(range);
            break;
        case NodeKind::Break:
            node = new BreakNode(range);
            break;
        case NodeKind::Continue:
            node = new ContinueNode(range);
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
    
    // Handle where clause: where is the header
    if (header->kind() == NodeKind::Where) {
        AstNode* inner = header->firstChild();
        if (!inner) return QString();
        
        // Inner could be :: (with return type) or call (without return type)
        if (inner->kind() == NodeKind::TypeAnnotation) {
            inner = inner->firstChild();
        }
        if (inner && inner->kind() == NodeKind::Call) {
            AstNode* nameNode = inner->firstChild();
            if (nameNode && nameNode->kind() == NodeKind::Identifier) {
                return nameNode->text();
            }
        }
        return QString();
    }
    
    // Handle return type wrapper: header is :: 
    if (header->kind() == NodeKind::TypeAnnotation) {
        AstNode* callNode = header->firstChild();
        if (!callNode) return QString();
        if (callNode->kind() == NodeKind::Call) {
            AstNode* nameNode = callNode->firstChild();
            if (nameNode && nameNode->kind() == NodeKind::Identifier) {
                return nameNode->text();
            }
        }
        return QString();
    }
    
    // Handle simple call (no return type, no where)
    if (header->kind() == NodeKind::Call) {
        AstNode* nameNode = header->firstChild();
        if (nameNode && nameNode->kind() == NodeKind::Identifier) {
            return nameNode->text();
        }
    }
    
    return QString();
}

AstNode* FunctionNode::functionNameNode() const
{
    AstNode* header = firstChild();
    if (!header) return nullptr;
    
    if (header->kind() == NodeKind::Where) {
        AstNode* inner = header->firstChild();
        if (!inner) return nullptr;
        if (inner->kind() == NodeKind::TypeAnnotation) {
            inner = inner->firstChild();
        }
        if (inner && inner->kind() == NodeKind::Call) {
            return inner->firstChild();
        }
        return nullptr;
    }
    
    if (header->kind() == NodeKind::TypeAnnotation) {
        AstNode* callNode = header->firstChild();
        if (!callNode || callNode->kind() != NodeKind::Call) return nullptr;
        return callNode->firstChild();
    }
    
    if (header->kind() == NodeKind::Call) {
        return header->firstChild();
    }
    
    return nullptr;
}

// callNode() returns the Call node representing the function signature (name + positional args)
// This is NOT a ParametersNode - use parametersNode() for that
AstNode* FunctionNode::callNode() const
{
    if (children().isEmpty()) return nullptr;
    
    AstNode* header = firstChild();
    if (!header) return nullptr;
    
    // Handle where clause: first child of where is the header
    if (header->kind() == NodeKind::Where) {
        AstNode* inner = header->firstChild();
        if (!inner) return nullptr;
        
        // Inner could be :: (with return type) or call (without return type)
        if (inner->kind() == NodeKind::TypeAnnotation) {
            AstNode* callNode = inner->firstChild();
            if (callNode && callNode->kind() == NodeKind::Call) {
                return callNode;  // Return just the Call, not the TypeAnnotation
            }
            return inner;  // Fallback
        }
        if (inner->kind() == NodeKind::Call) {
            return inner;  // Return the call node which contains name + params
        }
        return nullptr;
    }
    
    // Handle return type wrapper: header is ::
    if (header->kind() == NodeKind::TypeAnnotation) {
        AstNode* callNode = header->firstChild();
        if (callNode && callNode->kind() == NodeKind::Call) {
            return callNode;  // Return just the Call, not the TypeAnnotation
        }
        return header;  // Fallback
    }
    
    // Handle simple call (no return type, no where)
    if (header->kind() == NodeKind::Call) {
        return header;
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
    if (children().isEmpty()) return nullptr;
    
    AstNode* header = firstChild();
    if (!header) return nullptr;
    
    // Handle where clause
    if (header->kind() == NodeKind::Where) {
        AstNode* inner = header->firstChild();
        if (!inner) return nullptr;
        
        // With return type: inner is ::, return type is second child
        if (inner->kind() == NodeKind::TypeAnnotation) {
            const auto& children = inner->children();
            if (children.size() >= 2) {
                return children.at(1);  // Second child is return type
            }
        }
        // Without return type: no return type
        return nullptr;
    }
    
    // Handle return type wrapper: header is ::
    if (header->kind() == NodeKind::TypeAnnotation) {
        const auto& children = header->children();
        if (children.size() >= 2) {
            return children.at(1);  // Second child is return type
        }
        return nullptr;
    }
    
    // No return type (simple call)
    return nullptr;
}

bool FunctionNode::hasReturnType() const
{
    return returnType() != nullptr;
}

QList<AstNode*> FunctionNode::parameters() const
{
    QList<AstNode*> params;
    
    AstNode* argsNode = callNode();
    if (!argsNode) return params;
    
    // Case 1: argsNode is a Call (no return type, no where)
    if (argsNode->kind() == NodeKind::Call) {
        const auto& children = argsNode->children();
        for (int i = 1; i < children.size(); ++i) {  // Skip first child (function name)
            AstNode* child = children.at(i);
            if (child && child->kind() == NodeKind::TypeAnnotation) {
                params.append(child);
            }
        }
        return params;
    }
    
    // Case 2: argsNode is TypeAnnotation (has return type, possibly with where)
    if (argsNode->kind() == NodeKind::TypeAnnotation) {
        AstNode* callNode = argsNode->firstChild();
        if (!callNode || callNode->kind() != NodeKind::Call) return params;
        
        const auto& children = callNode->children();
        for (int i = 1; i < children.size(); ++i) {  // Skip first child (function name)
            AstNode* child = children.at(i);
            if (child && child->kind() == NodeKind::TypeAnnotation) {
                params.append(child);
            }
        }
        return params;
    }
    
    return params;
}

QList<AstNode*> FunctionNode::typeParameters() const
{
    QList<AstNode*> typeParams;
    
    if (children().isEmpty()) return typeParams;
    
    AstNode* header = firstChild();
    if (!header) return typeParams;
    
    // Handle where clause: type params are children of where (except first)
    if (header->kind() == NodeKind::Where) {
        const auto& children = header->children();
        for (int i = 1; i < children.size(); ++i) {  // Skip first child (the header)
            AstNode* child = children.at(i);
            if (child && child->kind() == NodeKind::Identifier) {
                typeParams.append(child);
            }
        }
    }
    
    return typeParams;
}

int FunctionNode::argumentCount() const
{
    return parameters().size();
}

QString FunctionNode::dump() const
{
    QString result = QLatin1String("Function: ") + text() + QLatin1String("\n");
    
    for (AstNode* child : children()) {
        result += child->dump();
    }
    
    return result;
}

FunctionDefinitionAst::FunctionDefinitionAst(const QString& name, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Function, name, range)
{
    setIsLeaf(false);
}

QString FunctionDefinitionAst::functionName() const
{
    if (children().isEmpty()) return QString();
    AstNode* nameNode = firstChild();
    return nameNode ? nameNode->text() : QString();
}

AstNode* FunctionDefinitionAst::functionNameNode() const
{
    return firstChild();
}

AstNode* FunctionDefinitionAst::callNode() const
{
    QList<AstNode*> kids = children();
    return kids.size() > 1 ? kids[1] : nullptr;
}

AstNode* FunctionDefinitionAst::body() const
{
    return lastChild();
}

AstNode* FunctionDefinitionAst::returnType() const
{
    return nullptr;
}

bool FunctionDefinitionAst::hasReturnType() const
{
    return false;
}

int FunctionDefinitionAst::argumentCount() const
{
    return parameters().size();
}

QList<AstNode*> FunctionDefinitionAst::parameters() const
{
    QList<AstNode*> result;
    AstNode* call = callNode();
    if (!call) return result;
    for (AstNode* child : call->children()) {
        if (child && child->kind() != NodeKind::Identifier) {
            result.append(child);
        }
    }
    return result;
}

QList<AstNode*> FunctionDefinitionAst::typeParameters() const
{
    return QList<AstNode*>();
}

QString FunctionDefinitionAst::dump() const
{
    return QStringLiteral("FunctionDefinitionAst: ") + functionName() + QStringLiteral("\n");
}

ReturnValueAst::ReturnValueAst(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Return, QString(), range)
{
    setIsLeaf(false);
}

AstNode* ReturnValueAst::value() const
{
    return firstChild();
}

QString ReturnValueAst::dump() const
{
    QString result = QStringLiteral("ReturnValue\n");
    if (value()) {
        result += value()->dump();
    }
    return result;
}

NameReferenceAst::NameReferenceAst(const QString& name, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Identifier, name, range), m_context(Context::Load)
{
    setIsLeaf(true);
}

QString NameReferenceAst::identifier() const
{
    return text();
}

NameReferenceAst::Context NameReferenceAst::context() const
{
    return m_context;
}

void NameReferenceAst::setContext(Context c)
{
    m_context = c;
}

QString NameReferenceAst::dump() const
{
    QString ctxStr = (m_context == Context::Load) ? QStringLiteral("Load") : QStringLiteral("Store");
    return QStringLiteral("NameReference: ") + text() + QStringLiteral(" (") + ctxStr + QStringLiteral(")\n");
}

AssignmentValueAst::AssignmentValueAst(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Assignment, QString(), range)
{
    setIsLeaf(false);
}

AstNode* AssignmentValueAst::leftHandSide() const
{
    return firstChild();
}

AstNode* AssignmentValueAst::rightHandSide() const
{
    QList<AstNode*> kids = children();
    return kids.size() > 1 ? kids[1] : nullptr;
}

QString AssignmentValueAst::dump() const
{
    QString result = QStringLiteral("AssignmentValue\n");
    if (leftHandSide()) {
        result += leftHandSide()->dump();
    }
    if (rightHandSide()) {
        result += rightHandSide()->dump();
    }
    return result;
}

BlockAst::BlockAst(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Block, QString(), range)
{
    setIsLeaf(false);
}

QList<AstNode*> BlockAst::statements() const
{
    return children();
}

QString BlockAst::dump() const
{
    QString result = QStringLiteral("BlockAst\n");
    for (AstNode* stmt : statements()) {
        result += stmt->dump();
    }
    return result;
}

IfBranchAst::IfBranchAst(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::If, QString(), range)
{
    setIsLeaf(false);
}

AstNode* IfBranchAst::condition() const
{
    return firstChild();
}

AstNode* IfBranchAst::thenBranch() const
{
    QList<AstNode*> kids = children();
    return kids.size() > 1 ? kids[1] : nullptr;
}

AstNode* IfBranchAst::elseBranch() const
{
    return lastChild();
}

QList<AstNode*> IfBranchAst::elseifBranches() const
{
    QList<AstNode*> result;
    QList<AstNode*> kids = children();
    for (int i = 2; i < kids.size() - 1; ++i) {
        result.append(kids[i]);
    }
    return result;
}

QString IfBranchAst::dump() const
{
    QString result = QStringLiteral("IfBranchAst\n");
    if (condition()) {
        result += condition()->dump();
    }
    if (thenBranch()) {
        result += thenBranch()->dump();
    }
    return result;
}

WhileLoopAst::WhileLoopAst(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::While, QString(), range)
{
    setIsLeaf(false);
}

AstNode* WhileLoopAst::condition() const
{
    return firstChild();
}

AstNode* WhileLoopAst::body() const
{
    return lastChild();
}

QString WhileLoopAst::dump() const
{
    QString result = QStringLiteral("WhileLoopAst\n");
    if (condition()) {
        result += condition()->dump();
    }
    if (body()) {
        result += body()->dump();
    }
    return result;
}

ForLoopAst::ForLoopAst(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::For, QString(), range)
{
    setIsLeaf(false);
}

AstNode* ForLoopAst::iterator() const
{
    return firstChild();
}

AstNode* ForLoopAst::body() const
{
    return lastChild();
}

QString ForLoopAst::dump() const
{
    QString result = QStringLiteral("ForLoopAst\n");
    if (iterator()) {
        result += iterator()->dump();
    }
    if (body()) {
        result += body()->dump();
    }
    return result;
}

TryCatchAst::TryCatchAst(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Try, QString(), range)
{
    setIsLeaf(false);
}

AstNode* TryCatchAst::tryBody() const
{
    return firstChild();
}

AstNode* TryCatchAst::catchVariable() const
{
    QList<AstNode*> kids = children();
    return kids.size() > 1 ? kids[1] : nullptr;
}

AstNode* TryCatchAst::catchBody() const
{
    QList<AstNode*> kids = children();
    return kids.size() > 2 ? kids[2] : nullptr;
}

AstNode* TryCatchAst::finallyBody() const
{
    return lastChild();
}

QString TryCatchAst::dump() const
{
    QString result = QStringLiteral("TryCatchAst\n");
    if (tryBody()) {
        result += tryBody()->dump();
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

QString StructNode::dump() const
{
    QString result = QLatin1String("Struct");
    
    QString name = structName();
    if (!name.isEmpty()) {
        result += QLatin1String(" ") + name;
    }
    
    result += QLatin1String("\n");
    
    for (AstNode* child : children()) {
        result += child->dump();
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

QString CallNode::dump() const
{
    QString result = QLatin1String("Call");
    
    QString name = functionName();
    if (!name.isEmpty()) {
        result += QLatin1String(" ") + name;
    }
    
    result += QStringLiteral(" (%1 args)\n").arg(argumentCount());
    
    for (AstNode* child : children()) {
        result += child->dump();
    }
    
    return result;
}

CurlyNode::CurlyNode(const QString& functionName, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Curly, functionName, range)
{
    setIsLeaf(false);
}

QString CurlyNode::functionName() const
{
    if (children().isEmpty()) return QString();
    
    AstNode* nameNode = firstChild();
    if (nameNode && nameNode->kind() == NodeKind::Identifier) {
        return nameNode->text();
    }
    
    return text();
}

QList<AstNode*> CurlyNode::arguments() const
{
    if (children().size() <= 1) return QList<AstNode*>();
    
    QList<AstNode*> result;
    for (int i = 1; i < children().size(); ++i) {
        result.append(children().at(i));
    }
    return result;
}

int CurlyNode::argumentCount() const
{
    return qMax(0, children().size() - 1);
}

QString CurlyNode::dump() const
{
    QString result = QLatin1String("Curly");
    
    QString name = functionName();
    if (!name.isEmpty()) {
        result += QLatin1String(" ") + name;
    }
    
    result += QStringLiteral(" (%1 args)\n").arg(argumentCount());
    
    for (AstNode* child : children()) {
        result += child->dump();
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

QString AssignmentNode::dump() const
{
    QString result = QLatin1String("Assignment\n");
    
    for (AstNode* child : children()) {
        result += child->dump();
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

QString IdentifierNode::dump() const
{
    return QLatin1String("Identifier: ") + text() + QLatin1String("\n");
}

StringNode::StringNode(const QString& value, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::String, value, range)
{
}

QString StringNode::value() const
{
    return text();
}

QString StringNode::dump() const
{
    return QLatin1String("String: \"") + text() + QLatin1String("\"\n");
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

QString NumberNode::dump() const
{
    return (m_isFloat ? QLatin1String("Float: ") : QLatin1String("Integer: ")) + text() + QLatin1String("\n");
}

TupleNode::TupleNode(const QString& name, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Tuple, name, range)
{
}

QList<AstNode*> TupleNode::elements() const
{
    return children();
}

QString TupleNode::dump() const
{
    QString result = QLatin1String("Tuple\n");
    
    for (AstNode* child : children()) {
        result += child->dump();
    }
    
    return result;
}

ArrayNode::ArrayNode(const QString& name, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Array, name, range)
{
}

QList<AstNode*> ArrayNode::elements() const
{
    return children();
}

QString ArrayNode::dump() const
{
    QString result = QLatin1String("Array\n");
    
    for (AstNode* child : children()) {
        result += child->dump();
    }
    
    return result;
}

ParametersNode::ParametersNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Parameters, QString(), range)
{
    setIsLeaf(false);
}

QList<AstNode*> ParametersNode::parameters() const
{
    return children();
}

QString ParametersNode::dump() const
{
    QString result = QLatin1String("Parameters\n");
    
    for (AstNode* child : children()) {
        result += child->dump();
    }
    
    return result;
}

TryNode::TryNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Try, QString(), range)
{
}

AstNode* TryNode::tryBody() const
{
    return firstChild();
}

AstNode* TryNode::catchVariable() const
{
    AstNode* catchBlock = nullptr;
    for (AstNode* child : children()) {
        if (child->kind() == NodeKind::Catch) {
            catchBlock = child;
            break;
        }
    }
    if (catchBlock && !catchBlock->children().isEmpty()) {
        return catchBlock->children().first();
    }
    return nullptr;
}

AstNode* TryNode::catchBody() const
{
    for (AstNode* child : children()) {
        if (child->kind() == NodeKind::Catch) {
            if (child->children().size() > 1) {
                return child->children().at(1);
            }
        }
    }
    return nullptr;
}

AstNode* TryNode::finallyBody() const
{
    for (AstNode* child : children()) {
        if (child->kind() == NodeKind::Finally) {
            return child->firstChild();
        }
    }
    return nullptr;
}

QString TryNode::dump() const
{
    QString result = QLatin1String("Try\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

ConstNode::ConstNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Const, QString(), range)
{
}

AstNode* ConstNode::target() const
{
    return firstChild();
}

AstNode* ConstNode::value() const
{
    return children().size() > 1 ? children().at(1) : nullptr;
}

QString ConstNode::dump() const
{
    QString result = QLatin1String("Const\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

LetNode::LetNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Let, QString(), range)
{
}

QList<AstNode*> LetNode::bindings() const
{
    QList<AstNode*> result;
    for (AstNode* child : children()) {
        if (child->kind() == NodeKind::Assignment || child->kind() == NodeKind::ColonEquals) {
            result.append(child);
        }
    }
    return result;
}

AstNode* LetNode::body() const
{
    for (AstNode* child : children()) {
        if (child->kind() == NodeKind::Block) {
            return child;
        }
    }
    return nullptr;
}

QString LetNode::dump() const
{
    QString result = QLatin1String("Let\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

DoNode::DoNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Do, QString(), range)
{
}

QList<AstNode*> DoNode::arguments() const
{
    QList<AstNode*> result;
    for (AstNode* child : children()) {
        if (child->kind() == NodeKind::Parameters) {
            return child->children();
        }
    }
    return result;
}

AstNode* DoNode::body() const
{
    for (AstNode* child : children()) {
        if (child->kind() == NodeKind::Block) {
            return child;
        }
    }
    return nullptr;
}

QString DoNode::dump() const
{
    QString result = QLatin1String("Do\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

QuoteNode::QuoteNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Quote, QString(), range)
{
}

QList<AstNode*> QuoteNode::body() const
{
    return children();
}

QString QuoteNode::dump() const
{
    QString result = QLatin1String("Quote\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

GlobalNode::GlobalNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Global, QString(), range)
{
}

QList<AstNode*> GlobalNode::identifiers() const
{
    return children();
}

QString GlobalNode::dump() const
{
    QString result = QLatin1String("Global\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

LocalNode::LocalNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Local, QString(), range)
{
}

QList<AstNode*> LocalNode::identifiers() const
{
    return children();
}

QString LocalNode::dump() const
{
    QString result = QLatin1String("Local\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

BaremoduleNode::BaremoduleNode(const QString& name, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Baremodule, name, range)
{
}

QString BaremoduleNode::moduleName() const
{
    return text();
}

AstNode* BaremoduleNode::body() const
{
    return firstChild();
}

QString BaremoduleNode::dump() const
{
    QString result = QLatin1String("Baremodule: ") + text() + QLatin1String("\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

ModuleNode::ModuleNode(const QString& name, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Module, name, range)
{
}

QString ModuleNode::moduleName() const
{
    return text();
}

AstNode* ModuleNode::body() const
{
    return firstChild();
}

QString ModuleNode::dump() const
{
    QString result = QLatin1String("Module: ") + text() + QLatin1String("\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

BeginNode::BeginNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Block, QString(), range)
{
}

AstNode* BeginNode::body() const
{
    return firstChild();
}

QString BeginNode::dump() const
{
    QString result = QLatin1String("Begin\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

BreakNode::BreakNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Break, QString(), range)
{
}

QString BreakNode::dump() const
{
    return QLatin1String("Break\n");
}

ContinueNode::ContinueNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Continue, QString(), range)
{
}

QString ContinueNode::dump() const
{
    return QLatin1String("Continue\n");
}

ReturnNode::ReturnNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Return, QString(), range)
{
}

AstNode* ReturnNode::value() const
{
    return firstChild();
}

QString ReturnNode::dump() const
{
    QString result = QLatin1String("Return\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

WhileNode::WhileNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::While, QString(), range)
{
}

AstNode* WhileNode::condition() const
{
    return firstChild();
}

AstNode* WhileNode::body() const
{
    return children().size() > 1 ? children().at(1) : nullptr;
}

QString WhileNode::dump() const
{
    QString result = QLatin1String("While\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

ForNode::ForNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::For, QString(), range)
{
}

AstNode* ForNode::iterator() const
{
    return firstChild();
}

AstNode* ForNode::body() const
{
    return children().size() > 1 ? children().at(1) : nullptr;
}

QString ForNode::dump() const
{
    QString result = QLatin1String("For\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

IfNode::IfNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::If, QString(), range)
{
}

AstNode* IfNode::condition() const
{
    return firstChild();
}

AstNode* IfNode::thenBranch() const
{
    return children().size() > 1 ? children().at(1) : nullptr;
}

AstNode* IfNode::elseBranch() const
{
    for (AstNode* child : children()) {
        if (child->kind() == NodeKind::Else) {
            return child;
        }
    }
    return nullptr;
}

QList<AstNode*> IfNode::elseifBranches() const
{
    QList<AstNode*> result;
    for (AstNode* child : children()) {
        if (child->kind() == NodeKind::ElseIf) {
            result.append(child);
        }
    }
    return result;
}

QString IfNode::dump() const
{
    QString result = QLatin1String("If\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

ElseIfNode::ElseIfNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::ElseIf, QString(), range)
{
}

AstNode* ElseIfNode::condition() const
{
    return firstChild();
}

AstNode* ElseIfNode::body() const
{
    return children().size() > 1 ? children().at(1) : nullptr;
}

QString ElseIfNode::dump() const
{
    QString result = QLatin1String("ElseIf\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

ElseNode::ElseNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Else, QString(), range)
{
}

AstNode* ElseNode::body() const
{
    return firstChild();
}

QString ElseNode::dump() const
{
    QString result = QLatin1String("Else\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

EndNode::EndNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::End, QString(), range)
{
}

QString EndNode::dump() const
{
    return QLatin1String("End\n");
}

ExportNode::ExportNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Export, QString(), range)
{
}

QList<AstNode*> ExportNode::identifiers() const
{
    return children();
}

QString ExportNode::dump() const
{
    QString result = QLatin1String("Export\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

ImportNode::ImportNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Import, QString(), range)
{
}

QList<AstNode*> ImportNode::importPaths() const
{
    return children();
}

QString ImportNode::dump() const
{
    QString result = QLatin1String("Import\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

MacroNode::MacroNode(const QString& name, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Macro, name, range)
{
}

QString MacroNode::macroName() const
{
    return text();
}

AstNode* MacroNode::parameters() const
{
    for (AstNode* child : children()) {
        if (child->kind() == NodeKind::Parameters) {
            return child;
        }
    }
    return nullptr;
}

AstNode* MacroNode::body() const
{
    for (AstNode* child : children()) {
        if (child->kind() == NodeKind::Block) {
            return child;
        }
    }
    return nullptr;
}

QString MacroNode::dump() const
{
    QString result = QLatin1String("Macro: ") + text() + QLatin1String("\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

AbstractNode::AbstractNode(const QString& name, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Abstract, name, range)
{
}

QString AbstractNode::typeName() const
{
    return text();
}

AstNode* AbstractNode::supertype() const
{
    return firstChild();
}

QString AbstractNode::dump() const
{
    QString result = QLatin1String("Abstract: ") + text() + QLatin1String("\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

PrimitiveNode::PrimitiveNode(const QString& name, const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Primitive, name, range)
{
}

QString PrimitiveNode::typeName() const
{
    return text();
}

AstNode* PrimitiveNode::underlyingType() const
{
    return firstChild();
}

QString PrimitiveNode::dump() const
{
    QString result = QLatin1String("Primitive: ") + text() + QLatin1String("\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

CatchNode::CatchNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Catch, QString(), range)
{
}

AstNode* CatchNode::variable() const
{
    return firstChild();
}

AstNode* CatchNode::body() const
{
    return children().size() > 1 ? children().at(1) : nullptr;
}

QString CatchNode::dump() const
{
    QString result = QLatin1String("Catch\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

FinallyNode::FinallyNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Finally, QString(), range)
{
}

AstNode* FinallyNode::body() const
{
    return firstChild();
}

QString FinallyNode::dump() const
{
    QString result = QLatin1String("Finally\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

AsNode::AsNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Identifier, QString(), range)
{
}

AstNode* AsNode::original() const
{
    return firstChild();
}

AstNode* AsNode::alias() const
{
    return children().size() > 1 ? children().at(1) : nullptr;
}

QString AsNode::dump() const
{
    QString result = QLatin1String("As\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

DocNode::DocNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Comment, QString(), range)
{
}

AstNode* DocNode::document() const
{
    return firstChild();
}

QString DocNode::dump() const
{
    QString result = QLatin1String("Doc\n");
    for (AstNode* child : children()) {
        result += child->dump();
    }
    return result;
}

MutableNode::MutableNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Identifier, QString(), range)
{
}

QString MutableNode::dump() const
{
    return QLatin1String("Mutable\n");
}

OuterNode::OuterNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Identifier, QString(), range)
{
}

QString OuterNode::dump() const
{
    return QLatin1String("Outer\n");
}

PublicNode::PublicNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Identifier, QString(), range)
{
}

QString PublicNode::dump() const
{
    return QLatin1String("Public\n");
}

VarNode::VarNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Identifier, QString(), range)
{
}

QString VarNode::dump() const
{
    return QLatin1String("Var\n");
}

TypeNode::TypeNode(const KDevelop::RangeInRevision& range)
    : AstNode(NodeKind::Identifier, QString(), range)
{
}

QString TypeNode::dump() const
{
    return QLatin1String("Type\n");
}

}
