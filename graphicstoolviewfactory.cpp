#include "graphicstoolviewfactory.h"
#include "graphicsoutputwidget.h"

#include <interfaces/icore.h>
#include <interfaces/iproject.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/idocument.h>
#include <interfaces/idocumentcontroller.h>

#include <KConfigGroup>

#include "juliadebug.h"

namespace Julia {

GraphicsToolViewFactory::GraphicsToolViewFactory()
{
    loadProjectConfiguration();
}

GraphicsToolViewFactory::~GraphicsToolViewFactory()
{
}

QWidget* GraphicsToolViewFactory::create(QWidget* parent)
{
    GraphicsWidget* view = new GraphicsWidget(parent,
                                                  m_graphicsMethod,
                                                  m_graphicsDir,
                                                  m_socketHost,
                                                  m_socketPort
                                                  );

    // Configure the view with current settings
    // equal to default value
    // TODO make proper default configured file for orphans or socket saved pics
    view->setGraphicsDirectory(m_graphicsDir);

    // Configure socket settings if needed
    if (m_graphicsMethod == 1) { // socket-based
        view->startSocketServer(m_socketPort);
    }

    return view;
}

Qt::DockWidgetArea GraphicsToolViewFactory::defaultPosition() const
{
    return Qt::RightDockWidgetArea;
}

QString GraphicsToolViewFactory::id() const
{
    return QStringLiteral("org.kdevelop.GraphicsOutputView");
}


void GraphicsToolViewFactory::loadProjectConfiguration()
{
    auto document = KDevelop::ICore::self()->documentController()->activeDocument();
    if (!document) {
        qCWarning(KDEV_JULIA) << "No active document to execute";
        return;
    }
    // Get project configuration
    int     m_graphicsMethod = 0; // file-based by default
    QString m_graphicsDir = QStringLiteral("/tmp/graphics");
    QString m_socketHost = QStringLiteral("localhost");
    int     m_socketPort = 8080;

    if (auto project = KDevelop::ICore::self()->projectController()->findProjectForUrl(document->url())) {
        KConfigGroup config = project->projectConfiguration()->group(QStringLiteral("Juliasupport"));
        m_graphicsMethod = config.readEntry("graphicsMethod", 0);
        m_graphicsDir = config.readEntry("graphicsFileDir", QStringLiteral("/tmp/graphics"));
        m_socketPort = config.readEntry("socketPort", 8080);
        m_socketHost = config.readEntry("socketHost", QStringLiteral("localhost"));

        qCDebug(KDEV_JULIA) << "Loaded graphics configuration from project:"
                        << "method=" << m_graphicsMethod
                        << "dir=" << m_graphicsDir
                        << "host=" << m_socketHost
                        << "port=" << m_socketPort;
    }
}

void GraphicsToolViewFactory::setExecutionContext(int graphicsMethod, QString graphicsDir, QString host, int port)
{
    m_graphicsMethod = graphicsMethod;
    m_graphicsDir = graphicsDir;
    m_socketHost = host;
    m_socketPort = port;

    qCDebug(KDEV_JULIA) << "Updated graphics configuration from execution context" << ":"
    << "method=" << m_graphicsMethod
    << "dir=" << m_graphicsDir
    << "host=" << m_socketHost
    << "port=" << m_socketPort;
}

} // namespace Julia
