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
    virtual void visitBlock(BlockAst*) {}
    virtual void visitFunctionDefinition(FunctionDefinitionAst*) {}
    virtual void visitReturn(ReturnAst*) {}
    virtual void visitAssignment(AssignmentAst*) {}
    virtual void visitFor(ForAst*) {}
    virtual void visitWhile(WhileAst*) {}
    virtual void visitIf(IfAst*) {}
    virtual void visitTry(TryAst*) {}
    virtual void visitImport(ImportAst*) {}
    virtual void visitSelectiveImport(ImportAst*) {}
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
    virtual void visitNumber(NumberAst*) {}
    virtual void visitString(StringAst*) {}
    virtual void visitList(ListAst*) {}
    virtual void visitTuple(TupleAst*) {}
    virtual void visitDict(DictAst*) {}
    virtual void visitStarred(StarredAst*) {}
    virtual void visitLambda(LambdaAst*) {}
    virtual void visitIfExpression(IfExpressionAst*) {}
    
    // Julia-specific expression visitors
    virtual void visitParameter(ParameterAst*) {}
    virtual void visitTypeAnnotation(TypeAnnotationAst*) {}
    virtual void visitSubtype(SubtypeAst*) {}
    virtual void visitCurly(CurlyAst*) {}
    virtual void visitImportPath(ImportPathAst*) {}
    virtual void visitGenerator(GeneratorAst*) {}
    virtual void visitInterpolatedString(InterpolatedStringAst*) {}
    virtual void visitMacroCall(MacroCallAst*) {}
    virtual void visitRef(RefAst*) {}
    virtual void visitWhere(WhereAst*) {}
    // add InAst
    virtual void visitEllipsis(EllipsisAst*) {}

    // Other visitors
    virtual void visitCode(TopLevelAst*) {}
    virtual void visitAlias(AliasAst*) {}
    virtual void visitCatch(CatchAst*) {}
    virtual void visitComprehension(ComprehensionAst*) {}
    virtual void visitSlice(SliceAst*) {}
    virtual void visitIdentifier(IdentifierAst*) {}
    virtual void visitFilter(FilterAst*) {}
    virtual void visitIn(InAst*) {}
    virtual void visitIteration(IterationAst*) {}
};

} // namespace Julia

#endif
