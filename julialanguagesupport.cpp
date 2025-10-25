#include <QMutexLocker>
#include <QReadWriteLock>

#include <KPluginFactory>
#include <KLocalizedString>

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
#include <interfaces/launchconfigurationtype.h>
#include <interfaces/icore.h>
#include <interfaces/iplugincontroller.h>
#include <interfaces/iruncontroller.h>
#include <interfaces/iuicontroller.h>

#include <executescript/iexecutescriptplugin.h>

#include <language/assistant/renameassistant.h>
#include <language/interfaces/editorcontext.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/codecompletion/codecompletion.h>
#include <language/codecompletion/codecompletionmodel.h>

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <QDebug>
#include <QProcess>
#include <QAction>
#include <QtAssert>

#include "julialanguagesupport.h"
#include "juliahighlighting.hpp"
#include "juliaexecutionlauncher.h"
#include "juliaexecutionjob.h"
#include "kdevjuliaversion.h"
#include "projectconfig/projectconfigpage.h"
#include "graphicstoolviewfactory.h"

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
        QAction* executeAction = new QAction(QIcon::fromTheme(QStringLiteral("system-run")), i18n("Execute Julia Script"), parent);
        connect(executeAction, &QAction::triggered, this, &LanguageSupport::executeCurrentJuliaFile);
        cm.addAction(ContextMenuExtension::BuildGroup, executeAction);
    }
    return cm;
}

LanguageSupport::LanguageSupport(QObject* parent, const KPluginMetaData& metaData, const QVariantList& args)
    : KDevelop::IPlugin(QStringLiteral("julialanguagesupport"), parent, metaData)
    , KDevelop::ILanguageSupport()
{
    Q_UNUSED(args);

    qDebug(KDEV_JULIA) << "Julia language support plugin loaded!";
    m_self = this;

    // Initialize syntax highlighting
    //m_highlighting = new Highlighting(this);

    // Check if Julia Language Server is installed
    checkJuliaLanguageServer();

    // Register the Julia execution launcher
    IExecuteScriptPlugin* iface = KDevelop::ICore::self()->pluginController()
                                  ->pluginForExtension(QStringLiteral("org.kdevelop.IExecuteScriptPlugin"))->extension<IExecuteScriptPlugin>();
    Q_ASSERT(iface);
    KDevelop::LaunchConfigurationType* type = core()->runController()
                                                  ->launchConfigurationTypeForId(iface->scriptAppConfigTypeId());
    Q_ASSERT(type);
    type->addLauncher(new JuliaExecutionLauncher());
    qCDebug(KDEV_JULIA) << "Julia execution launcher registered";

 
    core()->uiController()->addToolView(QStringLiteral("Graphics Output"), new GraphicsToolViewFactory);

    // Connect to document opened signal
    QObject::connect(ICore::self()->documentController(),
    &IDocumentController::documentOpened, this,
    &LanguageSupport::documentOpened);
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

    // The LSP support is provided by the LSP client plugin which will
    // recognize our Julia files based on the MIME types and file extensions
    // that we've registered in the kdevjuliasupport.json file.
    qCDebug(KDEV_JULIA) << "Julia file opened:" << doc->url().toLocalFile();

    // DUChainReadLocker lock;
    // ReferencedTopDUContext top = DUChain::self()->chainForDocument(doc->url());
    // lock.unlock();

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
//
QString LanguageSupport::name() const
{
    return QStringLiteral("Julia");
}
//
// KDevelop::ICodeHighlighting* LanguageSupport::codeHighlighting() const
// {
//     return m_highlighting;
// }
//
LanguageSupport* LanguageSupport::self()
{
    return m_self;
}
//
bool LanguageSupport::enabledForFile(const QUrl& url)
{
    // Check if the file is a Julia file (.jl extension)
    return url.toString().endsWith(QLatin1String(".jl"));
}

// int LanguageSupport::configPages() const
// {
//     return 0; // We provide one config page
// }
//
// KDevelop::ConfigPage* LanguageSupport::configPage(int number, QWidget* parent)
// {
//     if (number == 0) {
//         // TODO: Create and return a Julia config page
//         // return new JuliaConfigPage(this, parent);
//     }
//     return nullptr;
// }

int LanguageSupport::perProjectConfigPages() const
{
    return 1; // We provide one per-project config page
}

KDevelop::ConfigPage* LanguageSupport::perProjectConfigPage(int number, const KDevelop::ProjectConfigOptions& options, QWidget* parent)
{
    if (number == 0) {
        return new Julia::ProjectConfigPage(this, options, parent);
    }
    return nullptr;
}

void LanguageSupport::executeCurrentJuliaFile()
{
    auto document = KDevelop::ICore::self()->documentController()->activeDocument();
    if (!document) {
        qCWarning(KDEV_JULIA) << "No active document to execute";
        return;
    }

    // Get project configuration
    QString interpreter = QStringLiteral(JULIA_EXECUTABLE); // default
    int graphicsMethod = 0; // file-based by default
    QString graphicsDir = QStringLiteral("/tmp/graphics");
    QString socketHost = QStringLiteral("127.0.0.1");
    int socketPort = 8080;

    if (auto project = KDevelop::ICore::self()->projectController()->findProjectForUrl(document->url())) {
        KConfigGroup config = project->projectConfiguration()->group(QStringLiteral("Juliasupport"));
        interpreter = config.readEntry("interpreter", QStringLiteral(JULIA_EXECUTABLE));
        graphicsMethod = config.readEntry("graphicsMethod", 0);
        graphicsDir = config.readEntry("graphicsFileDir", QStringLiteral("/tmp/graphics"));
        socketHost = config.readEntry("socketHost", QStringLiteral("127.0.0.1"));
        socketPort = config.readEntry("socketPort", 8080);
    }
    // process orphan files with default values anyway
    core()->uiController()->removeToolView(m_view);
    m_view = nullptr;
    m_view = new GraphicsToolViewFactory();
    m_view->setExecutionContext(graphicsMethod, graphicsDir, socketHost,socketPort);
    core()->uiController()->addToolView(QStringLiteral("GraphicsOutput"),m_view);


    // Create execution job with graphics redirection
    QStringList interpreterList{interpreter};
    QString scriptPath{document->url().toLocalFile()};
    QStringList arguments; // empty for now
    QUrl workingDirectory = QUrl::fromLocalFile(QFileInfo(scriptPath).absolutePath());
    QString environmentProfileName; // empty for default

    auto* job = new JuliaExecutionJob(interpreterList, scriptPath,
        arguments, workingDirectory, environmentProfileName, graphicsMethod, graphicsDir, socketHost,
        socketPort);

    // Start the job
    KDevelop::ICore::self()->runController()->registerJob(job);
}

}

#include "julialanguagesupport.moc"

#include "moc_julialanguagesupport.cpp"

