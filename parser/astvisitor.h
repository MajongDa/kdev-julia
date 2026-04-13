#ifndef JULIA_ASTVISITOR_H
#define JULIA_ASTVISITOR_H

#include "ast.h"

namespace Julia {

class AstVisitor {
public:
    virtual ~AstVisitor() = default;
    
    // Main entry point
    virtual void visitNode(Ast* node);
    
    // Helper to visit a list of nodes
    template<typename T> void visitNodeList(const QList<T*>& list) {
        for (T* node : list) {
            visitNode(node);
        }
    }
    
    // Statement visitors - core
    virtual void visitStatement(StatementAst*) {}
    virtual void visitFunctionDefinition(FunctionDefinitionAst*) {}
    virtual void visitReturn(ReturnAst*) {}
    virtual void visitAssignment(AssignmentAst*) {}
    virtual void visitFor(ForAst*) {}
    virtual void visitWhile(WhileAst*) {}
    virtual void visitIf(IfAst*) {}
    virtual void visitTry(TryAst*) {}
    virtual void visitImport(ImportAst*) {}
    virtual void visitImportFrom(ImportAst*) {}
    virtual void visitGlobal(GlobalAst*) {}
    virtual void visitBreak(BreakAst*) {}
    virtual void visitContinue(ContinueAst*) {}

    
    // Julia-specific statement visitors
    virtual void visitModule(ModuleAst*) {}
    virtual void visitBaremodule(BaremoduleAst*) {}
    virtual void visitStruct(StructAst*) {}
    virtual void visitAbstract(AbstractAst*) {}
    virtual void visitPrimitive(PrimitiveAst*) {}
    virtual void visitMacro(MacroAst*) {}
    virtual void visitUsing(UsingAst*) {}
    virtual void visitExport(ExportAst*) {}
    virtual void visitConst(ConstAst*) {}
    virtual void visitLocal(LocalAst*) {}
    virtual void visitLet(LetAst*) {}
    virtual void visitDo(DoAst*) {}
    
    // Expression visitors - core
    virtual void visitExpression(ExpressionAst*) {}
    virtual void visitCall(CallAst*) {}
    virtual void visitAttribute(AttributeAst*) {}
    virtual void visitBinaryOperation(BinaryOperationAst*) {}
    virtual void visitUnaryOperation(UnaryOperationAst*) {}
    virtual void visitNumber(NumberAst*) {}
    virtual void visitString(StringAst*) {}
    virtual void visitList(ListAst*) {}
    virtual void visitTuple(TupleAst*) {}
    virtual void visitDict(DictAst*) {}
    virtual void visitSubscript(SubscriptAst*) {}
    virtual void visitStarred(StarredAst*) {}
    virtual void visitLambda(LambdaAst*) {}
    virtual void visitIfExpression(IfExpressionAst*) {}
    
    // Julia-specific expression visitors
    virtual void visitParameter(ParameterAst*) {}
    virtual void visitTypeAnnotation(TypeAnnotationAst*) {}
    virtual void visitCurly(CurlyAst*) {}
    virtual void visitImportPath(ImportPathAst*) {}
    virtual void visitGenerator(GeneratorAst*) {}
    virtual void visitInterpolatedString(InterpolatedStringAst*) {}
    virtual void visitMacroCall(MacroCallAst*) {}
    virtual void visitRef(RefAst*) {}
    virtual void visitKwArg(KwArgAst*) {}
    
    // Pattern visitors
    virtual void visitPattern(PatternAst*) {}
    virtual void visitMatch(MatchAst*) {}
    virtual void visitMatchCase(MatchCaseAst*) {}
    
    // Other visitors
    virtual void visitCode(CodeAst*) {}
    virtual void visitArguments(ArgumentsAst*) {}
    virtual void visitArg(ArgAst*) {}
    virtual void visitKeyword(KeywordAst*) {}
    virtual void visitAlias(AliasAst*) {}
    virtual void visitExceptionHandler(ExceptionHandlerAst*) {}
    virtual void visitComprehension(ComprehensionAst*) {}
    virtual void visitSlice(SliceAst*) {}
    virtual void visitIdentifier(IdentifierAst*) {}
};

} // namespace Julia

#endif
