#include "juliacompletion.h"

#include <QDebug>

#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/codecompletion/codecompletionitem.h>
#include <language/codecompletion/codecompletioncontext.h>
#include <language/codecompletion/normaldeclarationcompletionitem.h>
#include <KTextEditor/Document>

namespace Julia {

JuliaCompletionWorker::JuliaCompletionWorker(KDevelop::CodeCompletionModel* model)
    : KDevelop::CodeCompletionWorker(model)
{
}

JuliaCompletionWorker::~JuliaCompletionWorker() = default;

void JuliaCompletionWorker::computeCompletions(const KDevelop::DUContextPointer& context,
                                             const KTextEditor::Cursor& position,
                                             const QString& followingText,
                                             const KTextEditor::Range& contextRange,
                                             const QString& contextText)
{
    Q_UNUSED(position);
    Q_UNUSED(contextRange);
    Q_UNUSED(contextText);

    addKeywordCompletions(followingText);
    addDeclarationCompletions(context, followingText);
}

void JuliaCompletionWorker::addKeywordCompletions(const QString& followingText)
{
    static const QStringList keywords = {
        QStringLiteral("function"), QStringLiteral("end"),
        QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("elseif"),
        QStringLiteral("for"), QStringLiteral("while"), QStringLiteral("do"),
        QStringLiteral("return"), QStringLiteral("break"), QStringLiteral("continue"),
        QStringLiteral("struct"), QStringLiteral("mutable"), QStringLiteral("abstract"),
        QStringLiteral("primitive"), QStringLiteral("type"),
        QStringLiteral("module"), QStringLiteral("baremodule"),
        QStringLiteral("using"), QStringLiteral("import"), QStringLiteral("export"),
        QStringLiteral("let"), QStringLiteral("local"), QStringLiteral("global"),
        QStringLiteral("const"),
        QStringLiteral("try"), QStringLiteral("catch"), QStringLiteral("finally"),
        QStringLiteral("throw"), QStringLiteral("begin"),
        QStringLiteral("quote"),
        QStringLiteral("macro"),
        QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("nothing"),
        QStringLiteral("in"), QStringLiteral("isa"),
        QStringLiteral("new"), QStringLiteral("super"),
    };

    QList<QExplicitlySharedDataPointer<KDevelop::CompletionTreeElement>> items;

    for (const QString& keyword : keywords) {
        if (keyword.startsWith(followingText)) {
            auto* item = new KDevelop::NormalDeclarationCompletionItem(
                KDevelop::DeclarationPointer(),
                QExplicitlySharedDataPointer<KDevelop::CodeCompletionContext>(),
                0
            );
            items << QExplicitlySharedDataPointer<KDevelop::CompletionTreeElement>(item);
        }
    }

    if (!items.isEmpty()) {
        foundDeclarations(items, QExplicitlySharedDataPointer<KDevelop::CodeCompletionContext>());
    }
}

void JuliaCompletionWorker::addDeclarationCompletions(const KDevelop::DUContextPointer& ctx,
                                                      const QString& followingText)
{
    if (!ctx) {
        return;
    }

    KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());

    QList<QExplicitlySharedDataPointer<KDevelop::CompletionTreeElement>> items;

    KDevelop::DUContext* context = ctx.data();
    if (!context) {
        return;
    }

    for (KDevelop::Declaration* decl : context->localDeclarations()) {
        if (decl && decl->identifier().toString().startsWith(followingText)) {
            auto* item = new KDevelop::NormalDeclarationCompletionItem(
                KDevelop::DeclarationPointer(decl),
                QExplicitlySharedDataPointer<KDevelop::CodeCompletionContext>(),
                0
            );
            items << QExplicitlySharedDataPointer<KDevelop::CompletionTreeElement>(item);
        }
    }

    for (KDevelop::DUContext* child : context->childContexts()) {
        if (child) {
            for (KDevelop::Declaration* decl : child->localDeclarations()) {
                if (decl && decl->identifier().toString().startsWith(followingText)) {
                    auto* item = new KDevelop::NormalDeclarationCompletionItem(
                        KDevelop::DeclarationPointer(decl),
                        QExplicitlySharedDataPointer<KDevelop::CodeCompletionContext>(),
                        0
                    );
                    items << QExplicitlySharedDataPointer<KDevelop::CompletionTreeElement>(item);
                }
            }
        }
    }

    lock.unlock();

    if (!items.isEmpty()) {
        foundDeclarations(items, QExplicitlySharedDataPointer<KDevelop::CodeCompletionContext>());
    }
}

}
