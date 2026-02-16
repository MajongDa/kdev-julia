#include "juliahighlighting.hpp"

#include <language/duchain/duchain.h>
#include <language/duchain/declaration.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/use.h>
#include <language/duchain/types/structuretype.h>
#include <language/duchain/types/functiontype.h>
#include <language/duchain/types/integraltype.h>
#include <language/duchain/types/arraytype.h>

#include "juliadebug.h"

using namespace KDevelop;

namespace Julia {

CodeHighlightingInstance::CodeHighlightingInstance(const Highlighting* highlighting)
    : KDevelop::CodeHighlightingInstance(highlighting)
    , checked_blocks(false)
    , has_blocks(false)
{
}

void CodeHighlightingInstance::checkHasBlocks(TopDUContext* top) const
{
    if (checked_blocks) {
        return;
    }
    checked_blocks = true;
    
    if (!top) {
        has_blocks = false;
        return;
    }
    
    has_blocks = !top->childContexts().isEmpty();
}

bool CodeHighlightingInstance::useRainbowColor(Declaration* dec) const
{
    if (!dec || !dec->context()) {
        return KDevelop::CodeHighlightingInstance::useRainbowColor(dec);
    }
    
    DUContext* ctx = dec->context();
    
    if (ctx->type() == DUContext::Function) {
        return true;
    }
    
    if (ctx->type() == DUContext::Other && ctx->owner()) {
        return true;
    }
    
    checkHasBlocks(dec->topContext());
    if (!has_blocks && !dec->internalContext() && ctx == dec->topContext()) {
        return true;
    }
    
    return KDevelop::CodeHighlightingInstance::useRainbowColor(dec);
}

CodeHighlightingType CodeHighlightingInstance::typeForDeclaration(Declaration* dec, DUContext* context) const
{
    if (!dec) {
        return CodeHighlightingType::Error;
    }
    
    if (dec->kind() == Declaration::Type) {
        if (dec->type<FunctionType>()) {
            return CodeHighlightingType::Function;
        }
        if (dec->type<StructureType>()) {
            return CodeHighlightingType::Class;
        }
    }
    
    if (dec->kind() == Declaration::Instance) {
        if (dec->type<ArrayType>()) {
            return CodeHighlightingType::LocalVariable;
        }
        
        IntegralType::Ptr intType = dec->type<IntegralType>();
        if (intType) {
            switch (intType->dataType()) {
                case IntegralType::TypeInt:
                case IntegralType::TypeLong:
                    return CodeHighlightingType::LocalVariable;
                case IntegralType::TypeDouble:
                case IntegralType::TypeFloat:
                    return CodeHighlightingType::LocalVariable;
                default:
                    break;
            }
        }
    }
    
    return KDevelop::CodeHighlightingInstance::typeForDeclaration(dec, context);
}

void CodeHighlightingInstance::highlightDeclaration(Declaration* decl, const QColor& color)
{
    KDevelop::CodeHighlightingInstance::highlightDeclaration(decl, color);
}

void CodeHighlightingInstance::highlightUse(DUContext* context, int index, const QColor& color)
{
    KDevelop::CodeHighlightingInstance::highlightUse(context, index, color);
}

Highlighting::Highlighting(QObject* parent)
    : CodeHighlighting(parent)
{
}

CodeHighlightingInstance* Highlighting::createInstance() const
{
    return new CodeHighlightingInstance(this);
}

void Highlighting::highlightDUChain(ReferencedTopDUContext context)
{
    qCDebug(KDEV_JULIA) << "Highlighting::highlightDUChain called for context:" << context.data();
    CodeHighlighting::highlightDUChain(context);
}

}
