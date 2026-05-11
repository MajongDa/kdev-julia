#ifndef JULIA_ASTDEFAULTVISITOR_H
#define JULIA_ASTDEFAULTVISITOR_H

#include "astvisitor.h"

namespace Julia {

class JuliaAstDefaultVisitor : public AstVisitor
{
public:
    JuliaAstDefaultVisitor() = default;
    ~JuliaAstDefaultVisitor() override = default;

    // Statements - core
    void visitBlock(BlockAst * node) override;
    void visitFunctionDefinition(FunctionDefinitionAst* node) override;
    void visitAssignment(AssignmentAst* node) override;
    void visitReturn(ReturnAst* node) override;
    void visitIf(IfAst* node) override;
    void visitFor(ForAst* node) override;
    void visitWhile(WhileAst* node) override;
    void visitTry(TryAst* node) override;
    void visitImport(ImportAst* node) override;
    void visitGlobal(GlobalAst* node) override;
    void visitBreak(BreakAst* node) override;
    void visitContinue(ContinueAst* node) override;

    // Julia-specific statements
    void visitModule(ModuleAst* node) override;
    void visitBaremodule(BaremoduleAst* node) override;
    void visitStruct(StructAst* node) override;
    void visitAbstract(AbstractAst* node) override;
    void visitPrimitive(PrimitiveAst* node) override;
    void visitMacro(MacroAst* node) override;
    void visitUsing(UsingAst* node) override;
    void visitExport(ExportAst* node) override;
    void visitConst(ConstAst* node) override;
    void visitLocal(LocalAst* node) override;
    void visitLet(LetAst* node) override;
    void visitDo(DoAst* node) override;
    
    // Expressions - core
    void visitCall(CallAst* node) override;
    void visitAttribute(AttributeAst* node) override;
    void visitNumber(NumberAst* node) override;
    void visitString(StringAst* node) override;
    void visitList(ListAst* node) override;
    void visitTuple(TupleAst* node) override;
    void visitDict(DictAst* node) override;
    void visitLambda(LambdaAst* node) override;
    void visitIfExpression(IfExpressionAst* node) override;

    // Julia-specific expressions
    void visitParameter(ParameterAst* node) override;
    void visitTypeAnnotation(TypeAnnotationAst* node) override;
    void visitSubtype(SubtypeAst* node) override;
    void visitCurly(CurlyAst* node) override;
    void visitImportPath(ImportPathAst* node) override;
    void visitGenerator(GeneratorAst* node) override;
    void visitInterpolatedString(InterpolatedStringAst* node) override;
    void visitMacroCall(MacroCallAst* node) override;
    void visitRef(RefAst* node) override;

    // Other
    void visitCode(TopLevelAst* node) override;
    void visitWhere(WhereAst* node) override;
    void visitStarred(StarredAst* node) override;
    void visitEllipsis(EllipsisAst* node) override;
    void visitExpression(ExpressionAst* node) override;
    void visitAlias(AliasAst* node) override;
    void visitCatch(CatchAst* node) override;
    void visitComprehension(ComprehensionAst* node) override;
    void visitSlice(SliceAst* node) override;
    void visitIdentifier(IdentifierAst* node) override;
    void visitFilter(FilterAst* node) override;
    void visitIn(InAst* node) override;
    void visitIteration(IterationAst* node) override;
};

class AstFreeVisitor : public JuliaAstDefaultVisitor
{
public:
    // Statements - core
    void visitFunctionDefinition(FunctionDefinitionAst* node) override {
        // Free raw signature tree first (all AST nodes that signature fields point into)
        if (node->raw) {
            visitNode(node->raw);
            node->raw = nullptr;
        }
        // Free the FunctionSignatureAst wrapper (NOT its children, owned by raw)
        if (node->signature) {
            delete node->signature;
            node->signature = nullptr;
        }
        // Free block
        if (node->block) {
            visitNode(node->block);
            node->block = nullptr;
        }
        delete node;
    }
    void visitBlock(BlockAst* node) override {
        JuliaAstDefaultVisitor::visitBlock(node);
        delete node;
    }
    void visitAssignment(AssignmentAst* node) override {
        JuliaAstDefaultVisitor::visitAssignment(node);
        delete node;
    }
    /* void visitCompoundAssignment(CompoundAssignmentAst * node) override;
     * currently dispatches through visitAssignment. Memory leak?
     */
    void visitReturn(ReturnAst* node) override {
        JuliaAstDefaultVisitor::visitReturn(node);
        delete node;
    }
    void visitIf(IfAst* node) override {
        JuliaAstDefaultVisitor::visitIf(node);
        delete node;
    }
    void visitFor(ForAst* node) override {
        JuliaAstDefaultVisitor::visitFor(node);
        delete node;
    }
    void visitWhile(WhileAst* node) override {
        JuliaAstDefaultVisitor::visitWhile(node);
        delete node;
    }
    void visitTry(TryAst* node) override {
        JuliaAstDefaultVisitor::visitTry(node);
        delete node;
    }
    void visitImport(ImportAst* node) override {
        JuliaAstDefaultVisitor::visitImport(node);
        delete node;
    }
    void visitGlobal(GlobalAst* node) override {
        JuliaAstDefaultVisitor::visitGlobal(node);
        delete node;
    }
    void visitBreak(BreakAst* node) override {
        JuliaAstDefaultVisitor::visitBreak(node);
        delete node;
    }
    void visitContinue(ContinueAst* node) override {
        JuliaAstDefaultVisitor::visitContinue(node);
        delete node;
    }
    // Julia-specific statements
    void visitModule(ModuleAst* node) override {
        JuliaAstDefaultVisitor::visitModule(node);
        delete node;
    }
    void visitBaremodule(BaremoduleAst* node) override {
        JuliaAstDefaultVisitor::visitBaremodule(node);
        delete node;
    }
    void visitStruct(StructAst* node) override {
        JuliaAstDefaultVisitor::visitStruct(node);
        delete node;
    }
    void visitAbstract(AbstractAst* node) override {
        JuliaAstDefaultVisitor::visitAbstract(node);
        delete node;
    }
    void visitPrimitive(PrimitiveAst* node) override {
        JuliaAstDefaultVisitor::visitPrimitive(node);
        delete node;
    }
    void visitMacro(MacroAst* node) override {
        JuliaAstDefaultVisitor::visitMacro(node);
        delete node;
    }
    void visitUsing(UsingAst* node) override {
        JuliaAstDefaultVisitor::visitUsing(node);
        delete node;
    }
    void visitExport(ExportAst* node) override {
        JuliaAstDefaultVisitor::visitExport(node);
        delete node;
    }
    void visitConst(ConstAst* node) override {
        JuliaAstDefaultVisitor::visitConst(node);
        delete node;
    }
    void visitLocal(LocalAst* node) override {
        JuliaAstDefaultVisitor::visitLocal(node);
        delete node;
    }
    void visitLet(LetAst* node) override {
        JuliaAstDefaultVisitor::visitLet(node);
        delete node;
    }
    void visitDo(DoAst* node) override {
        JuliaAstDefaultVisitor::visitDo(node);
        delete node;
    }
    // Expressions - core
    void visitCall(CallAst* node) override {
        JuliaAstDefaultVisitor::visitCall(node);
        delete node;
    }
    void visitAttribute(AttributeAst* node) override {
        JuliaAstDefaultVisitor::visitAttribute(node);
        delete node;
    }
    void visitNumber(NumberAst* node) override {
        JuliaAstDefaultVisitor::visitNumber(node);
        delete node;
    }
    void visitString(StringAst* node) override {
        JuliaAstDefaultVisitor::visitString(node);
        delete node;
    }
    void visitList(ListAst* node) override {
        JuliaAstDefaultVisitor::visitList(node);
        delete node;
    }
    void visitTuple(TupleAst* node) override {
        JuliaAstDefaultVisitor::visitTuple(node);
        delete node;
    }
    void visitDict(DictAst* node) override {
        JuliaAstDefaultVisitor::visitDict(node);
        delete node;
    }
    void visitLambda(LambdaAst* node) override {
        JuliaAstDefaultVisitor::visitLambda(node);
        delete node;
    }
    void visitIfExpression(IfExpressionAst* node) override {
        JuliaAstDefaultVisitor::visitIfExpression(node);
        delete node;
    }
    void visitExpression(ExpressionAst* node) override {
        JuliaAstDefaultVisitor::visitExpression(node);
        delete node;
    }
    void visitStarred(StarredAst* node) override {
        JuliaAstDefaultVisitor::visitStarred(node);
        delete node;
    }
    void visitEllipsis(EllipsisAst* node) override {
        JuliaAstDefaultVisitor::visitEllipsis(node);
        delete node;
    }
    // Julia-specific expressions
    void visitParameter(ParameterAst* node) override {
        JuliaAstDefaultVisitor::visitParameter(node);
        delete node;
    }
    void visitTypeAnnotation(TypeAnnotationAst* node) override {
        JuliaAstDefaultVisitor::visitTypeAnnotation(node);
        delete node;
    }
    void visitSubtype(SubtypeAst* node) override {
        JuliaAstDefaultVisitor::visitSubtype(node);
        delete node;
    }
    void visitCurly(CurlyAst* node) override {
        JuliaAstDefaultVisitor::visitCurly(node);
        delete node;
    }
    void visitImportPath(ImportPathAst* node) override {
        JuliaAstDefaultVisitor::visitImportPath(node);
        delete node;
    }
    void visitGenerator(GeneratorAst* node) override {
        JuliaAstDefaultVisitor::visitGenerator(node);
        delete node;
    }
    void visitInterpolatedString(InterpolatedStringAst* node) override {
        JuliaAstDefaultVisitor::visitInterpolatedString(node);
        delete node;
    }
    void visitMacroCall(MacroCallAst* node) override {
        JuliaAstDefaultVisitor::visitMacroCall(node);
        delete node;
    }
    void visitRef(RefAst* node) override {
        JuliaAstDefaultVisitor::visitRef(node);
        delete node;
    }
    void visitWhere(WhereAst* node) override {
        JuliaAstDefaultVisitor::visitWhere(node);
        delete node;
    }
    // Other
    // The CodeAst should not free itself, as this is supposed to be called from ~CodeAst.
    void visitCode(TopLevelAst* node) override {
        JuliaAstDefaultVisitor::visitCode(node);
    }
    void visitAlias(AliasAst* node) override {
        JuliaAstDefaultVisitor::visitAlias(node);
        delete node;
    }
    void visitCatch(CatchAst* node) override {
        JuliaAstDefaultVisitor::visitCatch(node);
        delete node;
    }
    void visitComprehension(ComprehensionAst* node) override {
        JuliaAstDefaultVisitor::visitComprehension(node);
        delete node;
    }
    void visitSlice(SliceAst* node) override {
        JuliaAstDefaultVisitor::visitSlice(node);
        delete node;
    }
    void visitIdentifier(IdentifierAst* node) override {
        JuliaAstDefaultVisitor::visitIdentifier(node);
        delete node;
    }
    void visitFilter(FilterAst* node) override {
        JuliaAstDefaultVisitor::visitFilter(node);
        delete node;
    }
    void visitIn(InAst* node) override {
        JuliaAstDefaultVisitor::visitIn(node);
        delete node;
    }
    void visitIteration(IterationAst* node) override {
        JuliaAstDefaultVisitor::visitIteration(node);
        delete node;
    }
};

void free_ast_recursive(TopLevelAst* node);
}

#endif
