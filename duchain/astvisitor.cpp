#include "astvisitor.h"
#include "../parser/ast.h"
#include "juliadebug.h"

namespace Julia {

void AstVisitor::visit(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << ">>> AstVisitor::visit:" << nodeKindToString(node->kind()) 
                        << "text:" << node->text().left(40) << "range:" << node->range();
    node->accept(this);
    qCDebug(KDEV_JULIA) << "<<< AstVisitor::visit DONE:" << nodeKindToString(node->kind());
}

void AstVisitor::visitNode(AstNode* node)
{
    if (!node) {
        return;
    }
    qCDebug(KDEV_JULIA) << "AstVisitor::visitNode called for:" << nodeKindToString(node->kind());
    switch (node->kind()) {
        case NodeKind::TopLevel:
            qCDebug(KDEV_JULIA) << "  -> calling visitTopLevel";
            visitTopLevel(node);
            qCDebug(KDEV_JULIA) << "  -> visitTopLevel returned";
            break;
        case NodeKind::Block:
            visitBlock(node);
            break;
        case NodeKind::Function:
            visitFunction(static_cast<FunctionNode*>(node));
            break;
        case NodeKind::Struct:
            visitStruct(static_cast<StructNode*>(node));
            break;
        case NodeKind::Module:
            visitModule(node);
            break;
        case NodeKind::Baremodule:
            visitBaremodule(static_cast<BaremoduleNode*>(node));
            break;
        case NodeKind::Abstract:
            visitAbstract(node);
            break;
        case NodeKind::Primitive:
            visitPrimitive(node);
            break;
        case NodeKind::Macro:
            visitMacro(node);
            break;
        case NodeKind::MacroCall:
            visitMacroCall(node);
            break;
        case NodeKind::Return:
            visitReturn(node);
            break;
        case NodeKind::If:
            visitIf(node);
            break;
        case NodeKind::ElseIf:
            visitElseIf(node);
            break;
        case NodeKind::Else:
            visitElse(node);
            break;
        case NodeKind::While:
            visitWhile(node);
            break;
        case NodeKind::For:
            visitFor(node);
            break;
        case NodeKind::Try:
            visitTry(static_cast<TryNode*>(node));
            break;
        case NodeKind::Catch:
            visitCatch(node);
            break;
        case NodeKind::Finally:
            visitFinally(node);
            break;
        case NodeKind::Break:
            visitBreak(node);
            break;
        case NodeKind::Continue:
            visitContinue(node);
            break;
        case NodeKind::Call:
            visitCall(static_cast<CallNode*>(node));
            break;
        case NodeKind::Identifier:
            visitIdentifier(node);
            break;
        case NodeKind::TypeAnnotation:
            visitTypeAnnotation(node);
            break;
        case NodeKind::Curly:
            visitCurly(static_cast<CurlyNode*>(node));
            break;
        case NodeKind::Where:
            visitWhere(node);
            break;
        case NodeKind::Parameters:
            visitParameters(node);
            break;
        case NodeKind::String:
            visitString(node);
            break;
        case NodeKind::Float:
            visitFloat(node);
            break;
        case NodeKind::Integer:
            visitInteger(node);
            break;
        case NodeKind::Bool:
            visitBool(node);
            break;
        case NodeKind::Tuple:
            visitTuple(node);
            break;
        case NodeKind::Array:
            visitArray(node);
            break;
        case NodeKind::Dict:
            visitDict(node);
            break;
        case NodeKind::Ref:
            visitRef(node);
            break;
        case NodeKind::Vect:
            visitVect(node);
            break;
        case NodeKind::Generator:
            visitGenerator(node);
            break;
        case NodeKind::ImportPath:
            visitImportPath(node);
            break;
        case NodeKind::MacroName:
            visitMacroName(node);
            break;
        case NodeKind::Using:
            visitUsing(node);
            break;
        case NodeKind::Import:
            visitImport(node);
            break;
        case NodeKind::Export:
            visitExport(node);
            break;
        case NodeKind::Assignment:
            visitAssignment(static_cast<AssignmentNode*>(node));
            break;
        case NodeKind::Const:
            visitConst(static_cast<ConstNode*>(node));
            break;
        case NodeKind::Global:
            visitGlobal(static_cast<GlobalNode*>(node));
            break;
        case NodeKind::Local:
            visitLocal(static_cast<LocalNode*>(node));
            break;
        case NodeKind::Let:
            visitLet(static_cast<LetNode*>(node));
            break;
        case NodeKind::Do:
            visitDo(static_cast<DoNode*>(node));
            break;
        case NodeKind::Quote:
            visitQuote(static_cast<QuoteNode*>(node));
            break;
        case NodeKind::End:
            visitEnd(node);
            break;
        case NodeKind::ColonEquals:
            visitColonEquals(node);
            break;
        case NodeKind::Dot:
            visitDot(node);
            break;
        case NodeKind::Colon:
            visitColon(node);
            break;
        case NodeKind::Semicolon:
            visitSemicolon(node);
            break;
        case NodeKind::Comma:
            visitComma(node);
            break;
        case NodeKind::Comment:
            visitComment(node);
            break;
        case NodeKind::Whitespace:
            visitWhitespace(node);
            break;
        case NodeKind::Newline:
            visitNewline(node);
            break;
        case NodeKind::Error:
            visitError(node);
            break;
        case NodeKind::Unknown:
            visitUnknown(node);
            break;
        case NodeKind::Operator:
            visitOperator(node);
            break;
        default:
            break;
    }
    qCDebug(KDEV_JULIA) << "<<< AstVisitor::visitNode DONE:" << nodeKindToString(node->kind());
}

}
