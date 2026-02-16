#include <QObject>
#include <QTest>
#include <QJsonDocument>
#include <QJsonObject>

#include <ast.h>

using namespace Julia;

class TestAst : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testNodeKindStringConversion();
    void testParseJsonBasic();
    void testParseJsonWithChildren();
    void testAstNodeTree();
    void testFunctionNode();
    void testStructNode();
};

void TestAst::testNodeKindStringConversion()
{
    QString funcStr = nodeKindToString(NodeKind::Function);
    QCOMPARE(funcStr, QStringLiteral("function"));

    QString structStr = nodeKindToString(NodeKind::Struct);
    QCOMPARE(structStr, QStringLiteral("struct"));

    QString unknownStr = nodeKindToString(NodeKind::Unknown);
    QCOMPARE(unknownStr, QStringLiteral("Unknown"));

    NodeKind funcKind = stringToNodeKind(QStringLiteral("function"));
    QCOMPARE(funcKind, NodeKind::Function);

    NodeKind structKind = stringToNodeKind(QStringLiteral("struct"));
    QCOMPARE(structKind, NodeKind::Struct);

    NodeKind invalidKind = stringToNodeKind(QStringLiteral("invalid_kind"));
    QCOMPARE(invalidKind, NodeKind::Unknown);
}

void TestAst::testParseJsonBasic()
{
    QJsonObject json;
    json[QStringLiteral("kind")] = QStringLiteral("Identifier");
    json[QStringLiteral("text")] = QStringLiteral("myvar");
    json[QStringLiteral("is_leaf")] = true;
    json[QStringLiteral("range")] = QJsonObject{
        {QStringLiteral("start_line"), 1},
        {QStringLiteral("start_column"), 0},
        {QStringLiteral("end_line"), 1},
        {QStringLiteral("end_column"), 6}
    };

    AstNode* node = AstNode::fromJson(json);
    QVERIFY(node != nullptr);
    QVERIFY(node->kind() == NodeKind::Identifier);
    QVERIFY(node->text() == QStringLiteral("myvar"));
    QVERIFY(node->isLeaf());
    QVERIFY(node->range().start.line == 0);
    QVERIFY(node->range().end.column == 5);

    delete node;
}

void TestAst::testParseJsonWithChildren()
{
    QJsonObject callJson;
    callJson[QStringLiteral("kind")] = QStringLiteral("call");
    callJson[QStringLiteral("text")] = QStringLiteral("foo");
    callJson[QStringLiteral("is_leaf")] = false;

    QJsonObject funcNameJson;
    funcNameJson[QStringLiteral("kind")] = QStringLiteral("Identifier");
    funcNameJson[QStringLiteral("text")] = QStringLiteral("foo");
    funcNameJson[QStringLiteral("is_leaf")] = true;

    QJsonArray children;
    children.append(funcNameJson);
    callJson[QStringLiteral("children")] = children;

    AstNode* node = AstNode::fromJson(callJson);
    QVERIFY(node != nullptr);
    QVERIFY(node->kind() == NodeKind::Call);
    QVERIFY(!node->isLeaf());
    QVERIFY(node->children().size() == 1);
    QVERIFY(node->firstChild()->kind() == NodeKind::Identifier);
    QVERIFY(node->firstChild()->text() == QStringLiteral("foo"));

    delete node;
}

void TestAst::testAstNodeTree()
{
    QJsonObject rootJson;
    rootJson[QStringLiteral("kind")] = QStringLiteral("toplevel");
    rootJson[QStringLiteral("text")] = QStringLiteral("");
    rootJson[QStringLiteral("is_leaf")] = false;

    QJsonObject funcJson;
    funcJson[QStringLiteral("kind")] = QStringLiteral("function");
    funcJson[QStringLiteral("text")] = QStringLiteral("");
    funcJson[QStringLiteral("is_leaf")] = false;

    QJsonObject identJson;
    identJson[QStringLiteral("kind")] = QStringLiteral("Identifier");
    identJson[QStringLiteral("text")] = QStringLiteral("test_func");
    identJson[QStringLiteral("is_leaf")] = true;

    QJsonArray funcChildren;
    funcChildren.append(identJson);
    funcJson[QStringLiteral("children")] = funcChildren;

    QJsonArray rootChildren;
    rootChildren.append(funcJson);
    rootJson[QStringLiteral("children")] = rootChildren;

    AstNode* root = AstNode::fromJson(rootJson);
    QVERIFY(root != nullptr);
    QVERIFY(root->kind() == NodeKind::TopLevel);
    QVERIFY(root->children().size() == 1);

    AstNode* funcNode = root->firstChild();
    QVERIFY(funcNode->kind() == NodeKind::Function);
    QVERIFY(funcNode->parent() == root);

    AstNode* identNode = funcNode->firstChild();
    QVERIFY(identNode->kind() == NodeKind::Identifier);
    QVERIFY(identNode->text() == QStringLiteral("test_func"));

    QVERIFY(identNode->nextSibling() == nullptr);
    QVERIFY(identNode->previousSibling() == nullptr);

    delete root;
}

void TestAst::testFunctionNode()
{
    KDevelop::RangeInRevision range(0, 0, 10, 5);
    FunctionNode funcNode(QStringLiteral("test_func"), range);

    QVERIFY(funcNode.kind() == NodeKind::Function);
    QVERIFY(funcNode.text() == QStringLiteral("test_func"));
    QVERIFY(funcNode.isLeaf() == false);
    QVERIFY(funcNode.range().start.line == 0);
    QVERIFY(funcNode.range().end.column == 5);
}

void TestAst::testStructNode()
{
    KDevelop::RangeInRevision range(0, 0, 20, 10);
    StructNode structNode(QStringLiteral("MyStruct"), range);

    QVERIFY(structNode.kind() == NodeKind::Struct);
    QVERIFY(structNode.text() == QStringLiteral("MyStruct"));
    QVERIFY(structNode.isLeaf() == false);
    QVERIFY(structNode.range().start.line == 0);
    QVERIFY(structNode.range().end.column == 10);

    QVERIFY(structNode.fields().isEmpty());
    QVERIFY(structNode.supertypes().isEmpty());
}

QTEST_MAIN(TestAst)
#include "test_ast.moc"
