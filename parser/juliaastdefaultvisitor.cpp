#include "juliaastdefaultvisitor.h"
#include "ast.h"
#include "juliadebug.h"

namespace Julia {

void free_ast_recursive(CodeAst* node)
{
    qCDebug(KDEV_JULIA) << "hello from CodeAst destructor";
    if (node) {
        AstFreeVisitor visitor;
        visitor.visitCode(node);
    }
}

void visitStatement(StatementAst* node)
{
    if(!node) return;
}

void JuliaAstDefaultVisitor::visitFunctionDefinition(FunctionDefinitionAst* node)
{
    if (!node) return;
    qCDebug(KDEV_JULIA) << ">>> JuliaAstDefaultVisitor::visitFunctionDefinition";
    if (node->arguments) visitNode(node->arguments);
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
    qCDebug(KDEV_JULIA) << "<<< DONE";
}

void JuliaAstDefaultVisitor::visitAssignment(AssignmentAst* node)
{
    if (!node) return;
    for (auto* target : node->targets) {
        if (target) visitNode(target);
    }
    if (node->value) visitNode(node->value);
}

void JuliaAstDefaultVisitor::visitReturn(ReturnAst* node)
{
    if (!node) return;
    if (node->value) visitNode(node->value);
}

void JuliaAstDefaultVisitor::visitIf(IfAst* node)
{
    if (!node) return;
    if (node->condition) visitNode(node->condition);
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
    for (auto* stmt : node->orelse) {
        if (stmt) visitNode(stmt);
    }
}

void JuliaAstDefaultVisitor::visitFor(ForAst* node)
{
    if (!node) return;
    if (node->target) visitNode(node->target);
    if (node->iterator) visitNode(node->iterator);
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
}

void JuliaAstDefaultVisitor::visitWhile(WhileAst* node)
{
    if (!node) return;
    if (node->condition) visitNode(node->condition);
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
}

void JuliaAstDefaultVisitor::visitTry(TryAst* node)
{
    if (!node) return;
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
}

void JuliaAstDefaultVisitor::visitImport(ImportAst* node)
{
    // TODO: Implement
}

void JuliaAstDefaultVisitor::visitGlobal(GlobalAst* node)
{
    for (auto* ident : node->names) {
        if (ident) visitNode(ident);
    }
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
    if (node->function) visitNode(node->function);
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
    // for (auto* key : node->keys) {
    //     if (key) visitNode(key);
    // }
    // for (auto* val : node->values) {
    //     if (val) visitNode(val);
    // }
}

void JuliaAstDefaultVisitor::visitSubscript(SubscriptAst* node)
{
    if (!node) return;
    if (node->value) visitNode(node->value);
    if (node->slice) visitNode(node->slice);
}

void JuliaAstDefaultVisitor::visitBinaryOperation(BinaryOperationAst* node)
{
    if (!node) return;
    if (node->left) visitNode(node->left);
    if (node->right) visitNode(node->right);
}

void JuliaAstDefaultVisitor::visitUnaryOperation(UnaryOperationAst* node)
{
    if (!node) return;
    if (node->operand) visitNode(node->operand);
}

void JuliaAstDefaultVisitor::visitLambda(LambdaAst* node)
{
    if (!node) return;
    if (node->arguments) visitNode(node->arguments);
    if (node->body) visitNode(node->body);
}

void JuliaAstDefaultVisitor::visitIfExpression(IfExpressionAst* node)
{
    if (!node) return;
    if (node->condition) visitNode(node->condition);
    if (node->body) visitNode(node->body);
    if (node->orelse) visitNode(node->orelse);
}

void JuliaAstDefaultVisitor::visitCode(CodeAst* node)
{
    if (!node) return;
    visitNodeList(node->body);
    // for (auto* stmt : node->body) {
    //     if (stmt) visitNode(stmt);
    // }
}

void JuliaAstDefaultVisitor::visitArguments(ArgumentsAst* node)
{
    if (!node) return;
    for (auto* arg : node->arguments) {
        if (arg) visitNode(arg);
    }
}

void JuliaAstDefaultVisitor::visitKeyword(KeywordAst* node)
{
    if (!node) return;
    if (node->value) visitNode(node->value);
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
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
}

void JuliaAstDefaultVisitor::visitBaremodule(BaremoduleAst* node)
{
    if (!node) return;
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
}

void JuliaAstDefaultVisitor::visitStruct(StructAst* node)
{
    if (!node) return;
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
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
    if (node->parameters) visitNode(node->parameters);
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
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
        if (binding) visitNode(binding);
    }
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
}

void JuliaAstDefaultVisitor::visitDo(DoAst* node)
{
    if (!node) return;
    if (node->call) visitNode(node->call);
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
}

// Julia-specific expressions

void JuliaAstDefaultVisitor::visitParameter(ParameterAst* node)
{
    if (!node) return;
    if (node->name) visitNode(node->name);
    if (node->defaultValue) visitNode(node->defaultValue);
    if (node->annotation) visitNode(node->annotation);
}

void JuliaAstDefaultVisitor::visitTypeAnnotation(TypeAnnotationAst* node)
{
    if (!node) return;
    if (node->value) visitNode(node->value);
    if (node->type) visitNode(node->type);
}

void JuliaAstDefaultVisitor::visitCurly(CurlyAst* node)
{
    if (!node) return;
    if (node->name) visitNode(node->name);
    for (auto* param : node->parameters) {
        if (param) visitNode(param);
    }
}

void JuliaAstDefaultVisitor::visitImportPath(ImportPathAst* node)
{
    if (!node) return;
    for (auto* name : node->names) {
        if (name) visitNode(name);
    }
    if (node->asName) visitNode(node->asName);
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
        if (part) visitNode(part);
    }
}

void JuliaAstDefaultVisitor::visitMacroCall(MacroCallAst* node)
{
    if (!node) return;
    if (node->name) visitNode(node->name);
    for (auto* arg : node->arguments) {
        if (arg) visitNode(arg);
    }
}

void JuliaAstDefaultVisitor::visitRef(RefAst* node)
{
    if (!node) return;
    if (node->value) visitNode(node->value);
    for (auto* idx : node->indices) {
        if (idx) visitNode(idx);
    }
}

void JuliaAstDefaultVisitor::visitKwArg(KwArgAst* node)
{
    if (!node) return;
    if (node->key) visitNode(node->key);
    if (node->value) visitNode(node->value);
}

void JuliaAstDefaultVisitor::visitArg(ArgAst* node)
{
    if (!node) return;
    if (node->argumentName) visitNode(node->argumentName);
    if (node->annotation) visitNode(node->annotation);
}

void JuliaAstDefaultVisitor::visitAlias(AliasAst* node)
{
    if (!node) return;
    if (node->name) visitNode(node->name);
    if (node->asName) visitNode(node->asName);
}

void JuliaAstDefaultVisitor::visitExceptionHandler(ExceptionHandlerAst* node)
{
    if (!node) return;
    if (node->type) visitNode(node->type);
    if (node->name) visitNode(node->name);
    for (auto* stmt : node->body) {
        if (stmt) visitNode(stmt);
    }
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

}
