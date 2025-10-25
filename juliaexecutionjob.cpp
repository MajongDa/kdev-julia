/*
    SPDX-FileCopyrightText: 2025 Your Name <your.email@example.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "juliaexecutionjob.h"

#include <QFileInfo>
#include <QStandardPaths>

#include <KProcess>
#include <KSharedConfig>
#include <KLocalizedString>

#include <interfaces/iruntimecontroller.h>
#include <interfaces/iruntime.h>
#include <outputview/outputmodel.h>
#include <outputview/outputdelegate.h>
#include <outputview/ioutputview.h>
#include <util/processlinemaker.h>
#include <util/environmentprofilelist.h>

#include <interfaces/icore.h>
#include <project/projectmodel.h>
#include <util/path.h>

#include "juliadebug.h"

using namespace KDevelop;

namespace Julia {

JuliaExecutionJob::JuliaExecutionJob(const QStringList& interpreter, const QString& scriptPath,
                                     const QStringList& arguments, const QUrl& workingDirectory,
                                     const QString& environmentProfileName, int graphicsMethod,
                                     const QString& graphicsDir, const QString& socketHost,
                                     int socketPort)
    : OutputJob(nullptr)
    , proc(nullptr)
    , lineMaker(nullptr)
{
    qCDebug(KDEV_JULIA) << "creating Julia execution job";
    setCapabilities(Killable);

    proc = new KProcess(this);
    lineMaker = new ProcessLineMaker(proc, this);

    setStandardToolView(IOutputView::RunView);
    setBehaviours(IOutputView::AllowUserClose | IOutputView::AutoScroll);
    auto* m = new OutputModel;
    m->setFilteringStrategy(OutputModel::NoFilter);
    setModel(m);
    setDelegate(new OutputDelegate);

    connect(lineMaker, &ProcessLineMaker::receivedStdoutLines, m, &OutputModel::appendLines);
    connect(proc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished), this, &JuliaExecutionJob::processFinished);
    connect(proc, &QProcess::errorOccurred, this, &JuliaExecutionJob::processError);

    // Set up environment
    const EnvironmentProfileList environmentProfiles(KSharedConfig::openConfig());
    QString envProfileName = environmentProfileName;
    if (envProfileName.isEmpty()) {
        envProfileName = environmentProfiles.defaultProfileName();
    }
    auto environment = environmentProfiles.createEnvironment(envProfileName, proc->systemEnvironment());

    // Add graphics redirection environment variables
    if (graphicsMethod == 0) { // File-based
        environment << QStringLiteral("JULIA_GRAPHICS_METHOD=file");
        environment << QStringLiteral("JULIA_GRAPHICS_DIR=") + graphicsDir;
        qCDebug(KDEV_JULIA) << "Set file-based graphics redirection: dir =" << graphicsDir;
    } else if (graphicsMethod == 1) { // TCP socket
        environment << QStringLiteral("JULIA_GRAPHICS_METHOD=socket");
        environment << QStringLiteral("JULIA_SOCKET_HOST=") + socketHost;
        environment << QStringLiteral("JULIA_SOCKET_PORT=") + QString::number(socketPort);
        qCDebug(KDEV_JULIA) << "Set socket-based graphics redirection: host =" << socketHost << "port =" << socketPort;
    }

    proc->setEnvironment(environment);

    // Set working directory
    QUrl wc = workingDirectory;
    if (!wc.isValid() || wc.isEmpty()) {
        wc = QUrl::fromLocalFile(QFileInfo{scriptPath}.absolutePath());
    }
    proc->setWorkingDirectory(ICore::self()->runtimeController()->currentRuntime()->pathInRuntime(Path(wc)).toLocalFile());

    // Set program and arguments
    QStringList program = interpreter;

    // Add startup script for graphics redirection if configured
    if (graphicsMethod >= 0) {
        QString startupScript = findStartupScript();
        if (!startupScript.isEmpty()) {
            program << QStringLiteral("--startup-file=no") <<QStringLiteral("-L")<< startupScript;
        }
    }

    program << scriptPath;
    program << arguments;

    qCDebug(KDEV_JULIA) << "setting Julia app:" << program;

    proc->setProgram(program);

    const auto scriptFileName = scriptPath.sliced(scriptPath.lastIndexOf(QLatin1Char{'/'}) + 1);
    setObjectName(scriptFileName);
}

void JuliaExecutionJob::start()
{
    qCDebug(KDEV_JULIA) << "launching Julia execution";
    if (proc) {
        Q_ASSERT(error() == NoError);
        startOutput();
        appendLine(i18n("Starting: %1", proc->program().join(QLatin1Char(' '))));
        ICore::self()->runtimeController()->currentRuntime()->startProcess(proc);
    } else {
        // No process means we've returned early on from the constructor, some bad error happened
        Q_ASSERT(error() != NoError);
        emitResult();
    }
}

bool JuliaExecutionJob::doKill()
{
    if (proc) {
        proc->kill();
        appendLine(i18n("*** Killed Application ***"));
    }
    return true;
}

void JuliaExecutionJob::processFinished(int exitCode, QProcess::ExitStatus status)
{
    lineMaker->flushBuffers();

    if (exitCode == 0 && status == QProcess::NormalExit) {
        appendLine(i18n("*** Exited normally ***"));
    } else if (status == QProcess::NormalExit) {
        appendLine(i18n("*** Exited with return code: %1 ***", QString::number(exitCode)));
        setError(OutputJob::FailedShownError);
    } else if (error() == KJob::KilledJobError) {
        appendLine(i18n("*** Process aborted ***"));
        setError(KJob::KilledJobError);
    } else {
        appendLine(i18n("*** Crashed with return code: %1 ***", QString::number(exitCode)));
        setError(OutputJob::FailedShownError);
    }
    qCDebug(KDEV_JULIA) << "Process done";
    emitResult();
}

void JuliaExecutionJob::processError(QProcess::ProcessError error)
{
    qCDebug(KDEV_JULIA) << proc->readAllStandardError();
    qCDebug(KDEV_JULIA) << proc->readAllStandardOutput();
    qCDebug(KDEV_JULIA) << proc->errorString();
    if (error == QProcess::FailedToStart) {
        setError(FailedShownError);
        QString errmsg = i18n("*** Could not start program '%1'. Make sure that the "
                              "path is specified correctly ***", proc->program().join(QLatin1Char(' ')));
        appendLine(errmsg);
        setErrorText(errmsg);
        emitResult();
    }
    qCDebug(KDEV_JULIA) << "Process error";
}

void JuliaExecutionJob::appendLine(const QString& l)
{
    if (OutputModel* m = model()) {
        m->appendLine(l);
    }
}

OutputModel* JuliaExecutionJob::model()
{
    return qobject_cast<OutputModel*>(OutputJob::model());
}

QString JuliaExecutionJob::findStartupScript()
{
    // Find the startup script in the installed data directory
    QString scriptPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                               QStringLiteral("kdevjuliasupport/juliastartupscript.jl"));
    if (!scriptPath.isEmpty()) {
        qCDebug(KDEV_JULIA) << "Found startup script:" << scriptPath;
        return scriptPath;
    }

    // For development builds, try relative to the plugin
    // QString devPath = QStringLiteral("../share/kdevjuliasupport/juliastartupscript.jl");
    // if (QFile::exists(devPath)) {
    //     qCDebug(KDEV_JULIA) << "Found startup script (dev):" << devPath;
    //     return devPath;
    // }

    qCWarning(KDEV_JULIA) << "Could not find Julia startup script";
    return QString();
}

}
