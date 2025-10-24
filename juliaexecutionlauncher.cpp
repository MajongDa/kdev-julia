/*
    SPDX-FileCopyrightText: 2025 Your Name <your.email@example.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "juliaexecutionlauncher.h"
#include <interfaces/idocumentcontroller.h>
#include <executescript/iexecutescriptplugin.h>
#include <interfaces/launchconfigurationpage.h>
#include <interfaces/ilaunchconfiguration.h>
#include <interfaces/iplugincontroller.h>
#include <interfaces/icore.h>
#include <interfaces/iuicontroller.h>

#include <KLocalizedString>
#include <KMessageBox>
#include <KParts/MainWindow>
#include <KConfigGroup>
#include <QFileInfo>
#include <QStandardPaths>
#include <QProcess>

#include <QDebug>
#include "juliadebug.h"
#include <util/environmentprofilelist.h>

#include "juliaexecutionjob.h"
#include <interfaces/iruntimecontroller.h>
#include <interfaces/iruntime.h>
#include <project/projectmodel.h>
#include <util/path.h>

namespace Julia {

JuliaExecutionLauncher::JuliaExecutionLauncher()
{

}

QList< KDevelop::LaunchConfigurationPageFactory* > JuliaExecutionLauncher::configPages() const
{
    return QList<KDevelop::LaunchConfigurationPageFactory*>();
}

QString JuliaExecutionLauncher::description() const
{
    return i18n("A plugin to execute Julia applications.");
}

QString JuliaExecutionLauncher::id()
{
    return QStringLiteral("juliaexecutionlauncher");
}

QString JuliaExecutionLauncher::name() const
{
    return QStringLiteral("Julia Execution");
}

KJob* JuliaExecutionLauncher::start(const QString& launchMode, KDevelop::ILaunchConfiguration* cfg)
{
    qCDebug(KDEV_JULIA) << "start of Julia execution requested";
    if ( launchMode == QStringLiteral("execute") ) {
        IExecuteScriptPlugin* iface = KDevelop::ICore::self()->pluginController()
                                      ->pluginForExtension(QStringLiteral("org.kdevelop.IExecuteScriptPlugin"))->extension<IExecuteScriptPlugin>();
        Q_ASSERT(iface);

        QString err;
        QStringList interpreter = iface->interpreter(cfg, err);
        if (interpreter.isEmpty()) {
            KMessageBox::error(KDevelop::ICore::self()->uiController()->activeMainWindow(),
                               i18n("No Julia interpreter configured. Please configure it in the project settings."),
                               i18n("Missing interpreter"));
            return nullptr;
        }

        // Check if interpreter is Julia
        if (!interpreter.first().contains(QLatin1String("julia"))) {
            KMessageBox::error(KDevelop::ICore::self()->uiController()->activeMainWindow(),
                               i18n("The configured interpreter is not Julia. Please configure a Julia interpreter."),
                               i18n("Wrong interpreter"));
            return nullptr;
        }

        QUrl scriptUrl;
        if ( iface->runCurrentFile(cfg) ) {
            auto document = KDevelop::ICore::self()->documentController()->activeDocument();
            if ( ! document ) {
                qCDebug(KDEV_JULIA) << "no current document";
                return nullptr;
            }
            scriptUrl = document->url();
        }
        else {
            scriptUrl = iface->script(cfg, err);
            if (scriptUrl.isEmpty()) {
                return nullptr;
            }
        }
        QString scriptPath = scriptUrl.toLocalFile();

        QStringList arguments = iface->arguments(cfg, err);
        if (!err.isEmpty()) {
            KMessageBox::error(KDevelop::ICore::self()->uiController()->activeMainWindow(),
                               i18n("Error parsing arguments: %1", err),
                               i18n("Argument error"));
            return nullptr;
        }

        // Get graphics redirection settings from config
        KConfigGroup grp = cfg->config();
        int graphicsMethod = grp.readEntry("graphicsMethod", 0);
        QString graphicsDir = grp.readEntry("graphicsFileDir", QStringLiteral("/tmp/graphics"));
        QString socketHost = grp.readEntry("socketHost", QStringLiteral("localhost"));
        int socketPort = grp.readEntry("socketPort", 8080);

        // Create the execution job with graphics redirection support
        auto* job = new JuliaExecutionJob(interpreter, scriptPath, arguments, iface->workingDirectory(cfg),
                                        iface->environmentProfileName(cfg), graphicsMethod, graphicsDir,
                                        socketHost, socketPort);


        return job;
    }
    qCDebug(KDEV_JULIA) << "unknown launch mode";
    return nullptr;
}

QStringList JuliaExecutionLauncher::supportedModes() const
{
    return QStringList() << QStringLiteral("execute");
}

}