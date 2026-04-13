#include "astvisitor.h"

#include <QDebug>

#include "ast.h"
#include "juliadebug.h"

namespace Julia {

void AstVisitor::visitNode(Ast* node)
{
    if (!node) {
        return;
    }
    
    qCDebug(KDEV_JULIA) << "AstVisitor::visitNode called for:" << static_cast<int>(node->astType);
    
    switch (node->astType) {
        // Statements - core
        case AstType::StatementAstType:
            visitStatement(static_cast<StatementAst*>(node));
            break;
        case AstType::FunctionDefinitionAstType:
            visitFunctionDefinition(static_cast<FunctionDefinitionAst*>(node));
            break;
        case AstType::ReturnAstType:
            visitReturn(static_cast<ReturnAst*>(node));
            break;
        case AstType::AssignmentAstType:
            visitAssignment(static_cast<AssignmentAst*>(node));
            break;
        case AstType::ForAstType:
            visitFor(static_cast<ForAst*>(node));
            break;
        case AstType::WhileAstType:
            visitWhile(static_cast<WhileAst*>(node));
            break;
        case AstType::IfAstType:
            visitIf(static_cast<IfAst*>(node));
            break;
        case AstType::TryAstType:
            visitTry(static_cast<TryAst*>(node));
            break;
        case AstType::ImportAstType:
            visitImport(static_cast<ImportAst*>(node));
            break;
        case AstType::GlobalAstType:
            visitGlobal(static_cast<GlobalAst*>(node));
            break;
        case AstType::BreakAstType:
            visitBreak(static_cast<BreakAst*>(node));
            break;
        case AstType::ContinueAstType:
            visitContinue(static_cast<ContinueAst*>(node));
            break;

            
        // Julia-specific statements
        case AstType::ModuleAstType:
            visitModule(static_cast<ModuleAst*>(node));
            break;
        case AstType::BaremoduleAstType:
            visitBaremodule(static_cast<BaremoduleAst*>(node));
            break;
        case AstType::StructAstType:
            visitStruct(static_cast<StructAst*>(node));
            break;
        case AstType::AbstractAstType:
            visitAbstract(static_cast<AbstractAst*>(node));
            break;
        case AstType::PrimitiveAstType:
            visitPrimitive(static_cast<PrimitiveAst*>(node));
            break;
        case AstType::MacroAstType:
            visitMacro(static_cast<MacroAst*>(node));
            break;
        case AstType::UsingAstType:
            visitUsing(static_cast<UsingAst*>(node));
            break;
        case AstType::ExportAstType:
            visitExport(static_cast<ExportAst*>(node));
            break;
        case AstType::ConstAstType:
            visitConst(static_cast<ConstAst*>(node));
            break;
        case AstType::LocalAstType:
            visitLocal(static_cast<LocalAst*>(node));
            break;
        case AstType::LetAstType:
            visitLet(static_cast<LetAst*>(node));
            break;
        case AstType::DoAstType:
            visitDo(static_cast<DoAst*>(node));
            break;
            
        // Expressions - core
        case AstType::ExpressionAstType:
            visitExpression(static_cast<ExpressionAst*>(node));
            break;
        case AstType::IdentifierAstType:
            visitIdentifier(static_cast<IdentifierAst*>(node));
            break;
        case AstType::CallAstType:
            visitCall(static_cast<CallAst*>(node));
            break;
        case AstType::AttributeAstType:
            visitAttribute(static_cast<AttributeAst*>(node));
            break;
        case AstType::BinaryOperationAstType:
            visitBinaryOperation(static_cast<BinaryOperationAst*>(node));
            break;
        case AstType::UnaryOperationAstType:
            visitUnaryOperation(static_cast<UnaryOperationAst*>(node));
            break;
        case AstType::NumberAstType:
            visitNumber(static_cast<NumberAst*>(node));
            break;
        case AstType::StringAstType:
            visitString(static_cast<StringAst*>(node));
            break;
        case AstType::ListAstType:
            visitList(static_cast<ListAst*>(node));
            break;
        case AstType::TupleAstType:
            visitTuple(static_cast<TupleAst*>(node));
            break;
        case AstType::DictAstType:
            visitDict(static_cast<DictAst*>(node));
            break;
        case AstType::SubscriptAstType:
            visitSubscript(static_cast<SubscriptAst*>(node));
            break;
        case AstType::StarredAstType:
            visitStarred(static_cast<StarredAst*>(node));
            break;
        case AstType::LambdaAstType:
            visitLambda(static_cast<LambdaAst*>(node));
            break;
        case AstType::IfExpressionAstType:
            visitIfExpression(static_cast<IfExpressionAst*>(node));
            break;
            
        // Julia-specific expressions
        case AstType::ParameterAstType:
            visitParameter(static_cast<ParameterAst*>(node));
            break;
        case AstType::TypeAnnotationAstType:
            visitTypeAnnotation(static_cast<TypeAnnotationAst*>(node));
            break;
        case AstType::CurlyAstType:
            visitCurly(static_cast<CurlyAst*>(node));
            break;
        case AstType::ImportPathAstType:
            visitImportPath(static_cast<ImportPathAst*>(node));
            break;
        case AstType::GeneratorAstType:
            visitGenerator(static_cast<GeneratorAst*>(node));
            break;
        case AstType::InterpolatedStringAstType:
            visitInterpolatedString(static_cast<InterpolatedStringAst*>(node));
            break;
        case AstType::MacroCallAstType:
            visitMacroCall(static_cast<MacroCallAst*>(node));
            break;
        case AstType::RefAstType:
            visitRef(static_cast<RefAst*>(node));
            break;
        case AstType::KwArgAstType:
            visitKwArg(static_cast<KwArgAst*>(node));
            break;
            
        // Top-level
        case AstType::TopLevelAstType:
            visitCode(static_cast<CodeAst*>(node));
            break;
            
        // Pattern
        case AstType::PatternAstType:
            visitPattern(static_cast<PatternAst*>(node));
            break;
        case AstType::MatchAstType:
            visitMatch(static_cast<MatchAst*>(node));
            break;
            
        // Additional AST types
        case AstType::ArgumentsAstType:
            visitArguments(static_cast<ArgumentsAst*>(node));
            break;
        case AstType::ArgAstType:
            visitArg(static_cast<ArgAst*>(node));
            break;
        case AstType::KeywordAstType:
            visitKeyword(static_cast<KeywordAst*>(node));
            break;
        case AstType::AliasAstType:
            visitAlias(static_cast<AliasAst*>(node));
            break;
        case AstType::ExceptionHandlerAstType:
            visitExceptionHandler(static_cast<ExceptionHandlerAst*>(node));
            break;
        case AstType::ComprehensionAstType:
            visitComprehension(static_cast<ComprehensionAst*>(node));
            break;
        case AstType::SliceAstType:
            visitSlice(static_cast<SliceAst*>(node));
            break;
        case AstType::EllipsisAstType:
            visitNode(static_cast<Ast*>(node));
            break;
        case AstType::MatchCaseAstType:
            visitMatchCase(static_cast<MatchCaseAst*>(node));
            break;

        default:
            break;
    }
}

} // namespace Julia
