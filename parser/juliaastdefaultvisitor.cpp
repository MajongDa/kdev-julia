#include "ast.h"
#include "juliaastdefaultvisitor.h"

#include "juliadebug.h"

namespace Julia {

void free_ast_recursive(TopLevelAst* node)
{
    qCDebug(KDEV_JULIA) << "hello from CodeAst destructor";
    if (node) {
        AstFreeVisitor visitor;
        visitor.visitCode(node);
    }
}

void JuliaAstDefaultVisitor::visitBlock(BlockAst* node)
{
    if (!node) return;
    visitNodeList(node->block);
}

void JuliaAstDefaultVisitor::visitFunctionDefinition(FunctionDefinitionAst* node)
{
    if (!node) return;
    qCDebug(KDEV_JULIA) << ">>> JuliaAstDefaultVisitor::visitFunctionDefinition";
    visitNode(node->signature);
    visitNode(node->block);
    qCDebug(KDEV_JULIA) << "<<< DONE";
}

void JuliaAstDefaultVisitor::visitAssignment(AssignmentAst* node)
{
    if (!node) return;
    visitNode(node->target);
    visitNode(node->value);
}

void JuliaAstDefaultVisitor::visitReturn(ReturnAst* node)
{
    if (!node) return;
    if (node->value) visitNode(node->value);
}

void JuliaAstDefaultVisitor::visitIf(IfAst* node)
{
    if (!node) return;
    visitNode(node->condition);
    visitNode(node->block);
    if (node->orelse) visitNode(node->orelse);
}

void JuliaAstDefaultVisitor::visitFor(ForAst* node)
{
    if (!node) return;
    visitNode(node->iterator);
    if (node->block) visitNode(node->block);
}

void JuliaAstDefaultVisitor::visitWhile(WhileAst* node)
{
    if (!node) return;
    visitNode(node->condition);
    visitBlock(node->block);
}

void JuliaAstDefaultVisitor::visitTry(TryAst* node)
{
    if (!node) return;
    if (node->block) visitNode(node->block);
}

void JuliaAstDefaultVisitor::visitImport(ImportAst* node)
{
    visitNode(node->module);
    visitNodeList(node->names);
}

void JuliaAstDefaultVisitor::visitGlobal(GlobalAst* node)
{
    visitNodeList(node->names);
}

void JuliaAstDefaultVisitor::visitBreak(BreakAst* node)
{
}

void JuliaAstDefaultVisitor::visitContinue(ContinueAst* node)
{
}

void JuliaAstDefaultVisitor::visitCall(CallAst* node)
{
    if (!node) return;
    visitNode(node->name);
    for (auto* arg : node->arguments) {
        if (arg) visitNode(arg);
    }
}

void JuliaAstDefaultVisitor::visitAttribute(AttributeAst* node)
{
    if (!node) return;
    if (node->value) visitNode(node->value);
}

void JuliaAstDefaultVisitor::visitNumber(NumberAst* node)
{
}

void JuliaAstDefaultVisitor::visitString(StringAst* node)
{
}

void JuliaAstDefaultVisitor::visitList(ListAst* node)
{
    if (!node) return;
    for (auto* elem : node->elements) {
        if (elem) visitNode(elem);
    }
}

void JuliaAstDefaultVisitor::visitTuple(TupleAst* node)
{
    if (!node) return;
    for (auto* elem : node->elements) {
        if (elem) visitNode(elem);
    }
}

void JuliaAstDefaultVisitor::visitDict(DictAst* node)
{
    if (!node) return;
    visitNodeList(node->keys);
    visitNodeList(node->values);
}

void JuliaAstDefaultVisitor::visitLambda(LambdaAst* node)
{
    if (!node) return;
    if (node->arguments) visitNode(node->arguments);
    if (node->block) visitNode(node->block);
}

void JuliaAstDefaultVisitor::visitIfExpression(IfExpressionAst* node)
{
    if (!node) return;
    if (node->condition) visitNode(node->condition);
    if (node->block) visitNode(node->block);
    if (node->orelse) visitNode(node->orelse);
}

void JuliaAstDefaultVisitor::visitCode(TopLevelAst* node)
{
    if (!node) return;
    visitNodeList(node->children);
}

void JuliaAstDefaultVisitor::visitIdentifier(IdentifierAst* node)
{
    //TODO Implement correct node traversal for identifiers
    //make it QT_Unused
}

// Julia-specific statements

void JuliaAstDefaultVisitor::visitModule(ModuleAst* node)
{
    if (!node) return;
    for (auto* stmt : node->block->block) {
        if (stmt) visitNode(stmt);
    }
}

void JuliaAstDefaultVisitor::visitBaremodule(BaremoduleAst* node)
{
    if (!node) return;
    for (auto* stmt : node->block->block) {
        if (stmt) visitNode(stmt);
    }
}

void JuliaAstDefaultVisitor::visitStruct(StructAst* node)
{
    if (!node) return;
    visitNode(node->signature);
    visitNode(node->block);
}

void JuliaAstDefaultVisitor::visitAbstract(AbstractAst* node)
{
}

void JuliaAstDefaultVisitor::visitPrimitive(PrimitiveAst* node)
{
}

void JuliaAstDefaultVisitor::visitMacro(MacroAst* node)
{
    if (!node) return;
    visitNode(node->signature);
    visitNode(node->block);
}

void JuliaAstDefaultVisitor::visitUsing(UsingAst* node)
{
    if (!node) return;
    for (auto* name : node->names) {
        if (name) visitNode(name);
    }
}

void JuliaAstDefaultVisitor::visitExport(ExportAst* node)
{
    if (!node) return;
    for (auto* name : node->names) {
        if (name) visitNode(name);
    }
}

void JuliaAstDefaultVisitor::visitConst(ConstAst* node)
{
    if (!node) return;
    if (node->target) visitNode(node->target);
    if (node->value) visitNode(node->value);
}

void JuliaAstDefaultVisitor::visitLocal(LocalAst* node)
{
    if (!node) return;
    for (auto* name : node->names) {
        if (name) visitNode(name);
    }
}

void JuliaAstDefaultVisitor::visitLet(LetAst* node)
{
    if (!node) return;
    for (auto* binding : node->bindings) {
        visitNode(binding);
    }
    for (auto* stmt : node->block) {
        visitNode(stmt);
    }
}

void JuliaAstDefaultVisitor::visitDo(DoAst* node)
{
    if (!node) return;
    visitNode(node->call);
    for (auto* stmt : node->block) {
        visitNode(stmt);
    }
}

// Julia-specific expressions
void JuliaAstDefaultVisitor::visitParameter(ParameterAst* node)
{
    if (!node) return;
    for (auto* kw : node->kwargs) {
        if (kw) visitNode(kw);
    }
}

void JuliaAstDefaultVisitor::visitTypeAnnotation(TypeAnnotationAst* node)
{
    if (!node) return;
    visitNode(node->value);
    visitNode(node->type);
}

void JuliaAstDefaultVisitor::visitSubtype(SubtypeAst* node)
{
    if (!node) return;
    visitNode(node->left);
    visitNode(node->right);
}

void JuliaAstDefaultVisitor::visitCurly(CurlyAst* node)
{
    if (!node) return;
    if (node->name) visitNode(node->name);
    for (auto* param : node->parameters) {
        visitNode(param);
    }
}

void JuliaAstDefaultVisitor::visitImportPath(ImportPathAst* node)
{
    if (!node) return;
    for (auto* name : node->names) {
        visitNode(name);
    }
}

void JuliaAstDefaultVisitor::visitGenerator(GeneratorAst* node)
{
    if (!node) return;
    if (node->expression) visitNode(node->expression);
    if (node->iterator) visitNode(node->iterator);
    for (auto* filter : node->filters) {
        if (filter) visitNode(filter);
    }
}

void JuliaAstDefaultVisitor::visitInterpolatedString(InterpolatedStringAst* node)
{
    if (!node) return;
    for (auto* part : node->parts) {
        visitNode(part);
    }
}

void JuliaAstDefaultVisitor::visitMacroCall(MacroCallAst* node)
{
    if (!node) return;
    visitNode(node->name);
    for (auto* arg : node->arguments) {
        visitNode(arg);
    }
}

void JuliaAstDefaultVisitor::visitRef(RefAst* node)
{
    if (!node) return;
    visitNode(node->value);
    for (auto* idx : node->indices) {
        visitNode(idx);
    }
}

void JuliaAstDefaultVisitor::visitWhere(WhereAst* node)
{
    if (!node) return;
    if (node->signature) visitNode(node->signature);
    for (auto* c : node->constraints) {
        if (c) visitNode(c);
    }
}

void JuliaAstDefaultVisitor::visitStarred(StarredAst* node)
{
    if (!node) return;
    if (node->value) visitNode(node->value);
}

void JuliaAstDefaultVisitor::visitEllipsis(EllipsisAst* node)
{
    if (!node) return;
    if (node->name) visitNode(node->name);
}

void JuliaAstDefaultVisitor::visitExpression(ExpressionAst* node)
{
    // ExpressionAst has no children
}

void JuliaAstDefaultVisitor::visitAlias(AliasAst* node)
{
    if (!node) return;
    if (node->name) visitNode(node->name);
    if (node->asName) visitNode(node->asName);
}

void JuliaAstDefaultVisitor::visitCatch(CatchAst* node)
{
    if (!node) return;
    visitNode(node->type);
    visitNode(node->name);
    visitNode(node->block);
}

void JuliaAstDefaultVisitor::visitComprehension(ComprehensionAst* node)
{
    if (!node) return;
    if (node->target) visitNode(node->target);
    if (node->iterator) visitNode(node->iterator);
    for (auto* cond : node->conditions) {
        if (cond) visitNode(cond);
    }
}

void JuliaAstDefaultVisitor::visitSlice(SliceAst* node)
{
    if (!node) return;
    if (node->lower) visitNode(node->lower);
    if (node->upper) visitNode(node->upper);
    if (node->step) visitNode(node->step);
}

void JuliaAstDefaultVisitor::visitFilter(FilterAst* node)
{
    if (!node) return;
    if (node->iterator) visitNode(node->iterator);
    if (node->condition) visitNode(node->condition);
}

void JuliaAstDefaultVisitor::visitIn(InAst* node)
{
    if (!node) return;
    if (node->target) visitNode(node->target);
    if (node->iter) visitNode(node->iter);
}

void JuliaAstDefaultVisitor::visitIteration(IterationAst* node)
{
    if (!node) return;
    visitNodeList(node->iterators);
}

}
