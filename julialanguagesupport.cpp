#include <QMutexLocker>
#include <QReadWriteLock>

#include <KPluginFactory>

#include <interfaces/icore.h>
#include <interfaces/idocument.h>
#include <interfaces/ilanguagecontroller.h>
#include <interfaces/iplugincontroller.h>
#include <interfaces/isourceformatter.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/context.h>
#include <interfaces/contextmenuextension.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>
#include <interfaces/isession.h>
#include <language/assistant/renameassistant.h>
#include <language/assistant/staticassistantsmanager.h>
#include <language/interfaces/editorcontext.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/codecompletion/codecompletion.h>
#include <language/codecompletion/codecompletionmodel.h>
#include <KTextEditor/Document>

#include <julialanguagesupport.h>
#include <juliahighlighting.hpp>

#include <QDebug>
#include <QProcess>
#include "juliadebug.h"

using namespace KDevelop;

K_PLUGIN_FACTORY_WITH_JSON( KDevJulaiSupportFactory, "kdevjuliasupport.json", registerPlugin<Julia::LanguageSupport>(); )

namespace Julia
{
LanguageSupport* LanguageSupport::m_self = nullptr;

// ContextMenuExtension LanguageSupport::contextMenuExtension(Context* context, QWidget* parent)
// {
//     ContextMenuExtension cm;
//     //EditorContext *ec = dynamic_cast<KDevelop::EditorContext *>(context);
//
//     // if (ec && ICore::self()->languageController()->languagesForUrl(ec->url()).contains(this)) {
//     //     // It's a Julia file, let's add our context menu.
//     //     //TODO add TypeCorrection::self().doContextMenu
//     // }
//     return cm;
// }

LanguageSupport::LanguageSupport(QObject* parent, const KPluginMetaData& metaData, const QVariantList& args)
    : KDevelop::IPlugin(QStringLiteral("julialanguagesupport"), parent, metaData )
//    , KDevelop::ILanguageSupport()
{
    Q_UNUSED(args);

    qCDebug(KDEV_JULIA) << "Hello world, my plugin is loaded!";
    // m_self = this;
    // // TODO add lsp-based code completition model
    // // JuliaCodeCompletionModel* codeCompletion = new JuliaCodeCompletionModel(this);
    // // new KDevelop::CodeCompletion(this, codeCompletion, "Julia");
    //
    // auto assistantsManager = core()->languageController()->staticAssistantsManager();
    // // assistantsManager->registerAssistant(StaticAssistant::Ptr(new RenameAssistant(this)));
    //
    // QObject::connect(ICore::self()->documentController(), &IDocumentController::documentOpened,
    //                  this, &LanguageSupport::documentOpened);
}

// void LanguageSupport::documentOpened(IDocument* doc)
// {
//     if ( ! ICore::self()->languageController()->languagesForUrl(doc->url()).contains(this) ) {
//         // not a julia file
//         return;
//     }
//
//     DUChainReadLocker lock;
//     ReferencedTopDUContext top = DUChain::self()->chainForDocument(doc->url());
//     lock.unlock();
// }

LanguageSupport::~LanguageSupport()
{
    // parseLock()->lockForWrite();
    // By locking the parse-mutexes, we make sure that parse jobs get a chance to finish in a good state
    // parseLock()->unlock();
    // TODO add lsp-based codeHighlighting
    // delete m_highlighting;
    // m_highlighting = nullptr;
}

//FIXME no parsejob here, only LSP instance invoke
// KDevelop::ParseJob *LanguageSupport::createParseJob( const IndexedString& url )
// {
//     return new ParseJob(url, this);
// }

QString LanguageSupport::name() const
{
    return QStringLiteral( "Julia" );
}

LanguageSupport* LanguageSupport::self()
{
    return m_self;
}
//
// int LanguageSupport::configPages() const
// {
//     return 2;
// }
//
// KDevelop::ConfigPage* LanguageSupport::configPage(int number, QWidget* parent)
// {
//     return nullptr;
// }
//
// int Julia::LanguageSupport::perProjectConfigPages() const
// {
//     return 1;
// }
//
// KDevelop::ConfigPage* LanguageSupport::perProjectConfigPage(int number, const KDevelop::ProjectConfigOptions& options, QWidget* parent)
// {
//     if ( number == 0 ) {
//         //TODO add julia configpage.h
//         // return new Julia::ProjectConfigPage(this, options, parent);
//     }
//     return nullptr;
// }

}

#include "julialanguagesupport.moc"

#include "moc_julialanguagesupport.cpp"

