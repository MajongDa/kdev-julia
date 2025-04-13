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
#include <KTextEditor/View>

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

KDevelop::ContextMenuExtension LanguageSupport::contextMenuExtension(Context* context, QWidget* parent)
{
    ContextMenuExtension cm;
    EditorContext *ec = dynamic_cast<KDevelop::EditorContext *>(context);

    if (ec && ICore::self()->languageController()->languagesForUrl(ec->url()).contains(this)) {
        // It's a Julia file, let's add our context menu.
        // TODO: Add any Julia-specific context menu items here
    }
    return cm;
}

LanguageSupport::LanguageSupport(QObject* parent, const KPluginMetaData& metaData, const QVariantList& args)
    : KDevelop::IPlugin(QStringLiteral("julialanguagesupport"), parent, metaData)
    , KDevelop::ILanguageSupport()
{
    Q_UNUSED(args);

    qCDebug(KDEV_JULIA) << "Julia language support plugin loaded!";
    m_self = this;
    
    // Initialize syntax highlighting
    m_highlighting = new Highlighting(this);
    
    // Check if Julia Language Server is installed
    checkJuliaLanguageServer();
    
    // Connect to document opened signal
    QObject::connect(ICore::self()->documentController(), &IDocumentController::documentOpened,
                     this, &LanguageSupport::documentOpened);
}

void LanguageSupport::checkJuliaLanguageServer()
{
    // Create a process to check if the Julia Language Server package is installed
    QProcess process;
    process.setProgram(QStringLiteral("julia"));
    process.setArguments({
        QStringLiteral("-e"), 
        QStringLiteral("using Pkg; haskey(Pkg.installed(), \"LanguageServer\") || println(\"Not installed\")")
    });
    
    process.start();
    if (!process.waitForFinished(5000)) {
        qCWarning(KDEV_JULIA) << "Failed to check for Julia Language Server installation";
        return;
    }
    
    QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    
    if (output == QLatin1String("Not installed")) {
        qCWarning(KDEV_JULIA) << "Julia Language Server not installed. LSP features will be disabled.";
        qCWarning(KDEV_JULIA) << "To install, run: julia -e \"using Pkg; Pkg.add(\\\"LanguageServer\\\")\" in your terminal";
    } else {
        qCDebug(KDEV_JULIA) << "Julia Language Server is installed, LSP features enabled";
    }
}

void LanguageSupport::documentOpened(IDocument* doc)
{
    if (!ICore::self()->languageController()->languagesForUrl(doc->url()).contains(this)) {
        // not a Julia file
        return;
    }

    DUChainReadLocker lock;
    ReferencedTopDUContext top = DUChain::self()->chainForDocument(doc->url());
    lock.unlock();
    
    // The LSP support is provided by the LSP client plugin which will
    // recognize our Julia files based on the MIME types and file extensions
    // that we've registered in the kdevjuliasupport.json file.
    qCDebug(KDEV_JULIA) << "Julia file opened:" << doc->url().toLocalFile();
}

LanguageSupport::~LanguageSupport()
{
    // By locking the parse-mutexes, we make sure that parse jobs get a chance to finish in a good state
    // parseLock()->lockForWrite();
    // parseLock()->unlock();
    
    delete m_highlighting;
    m_highlighting = nullptr;
    
    m_self = nullptr;
}

KDevelop::ParseJob* LanguageSupport::createParseJob(const IndexedString& url)
{
    // Use LSP for parsing instead of creating a custom parse job
    // For now, return nullptr
    return nullptr;
}

QString LanguageSupport::name() const
{
    return QStringLiteral("Julia");
}

KDevelop::ICodeHighlighting* LanguageSupport::codeHighlighting() const
{
    return m_highlighting;
}

LanguageSupport* LanguageSupport::self()
{
    return m_self;
}

bool LanguageSupport::enabledForFile(const QUrl& url)
{
    // Check if the file is a Julia file (.jl extension)
    return url.toString().endsWith(QLatin1String(".jl"));
}

int LanguageSupport::configPages() const
{
    return 1; // We provide one config page
}

KDevelop::ConfigPage* LanguageSupport::configPage(int number, QWidget* parent)
{
    if (number == 0) {
        // TODO: Create and return a Julia config page
        // return new JuliaConfigPage(this, parent);
    }
    return nullptr;
}

int LanguageSupport::perProjectConfigPages() const
{
    return 1; // We provide one per-project config page
}

KDevelop::ConfigPage* LanguageSupport::perProjectConfigPage(int number, const KDevelop::ProjectConfigOptions& options, QWidget* parent)
{
    if (number == 0) {
        // TODO: Create and return a Julia project config page
        // return new Julia::ProjectConfigPage(this, options, parent);
    }
    return nullptr;
}

}

#include "julialanguagesupport.moc"

#include "moc_julialanguagesupport.cpp"

