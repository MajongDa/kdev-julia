#include "juliabridge.h"

#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

#include "juliadebug.h"

namespace Julia {

JuliaBridge::JuliaBridge(QObject* parent)
    : QObject(parent)
    , m_processAvailable(false)
{
    m_bridgeScriptPath = QStringLiteral("/home/majong/projects/kdev-julia/julia/parser_bridge.jl");
    m_juliaProjectDir = QStringLiteral("/home/majong/projects/kdev-julia/julia");
    
    initializeProcess();
}

JuliaBridge::~JuliaBridge()
{
}

void JuliaBridge::initializeProcess()
{
    QProcess testProcess;
    testProcess.setProgram(QStringLiteral("julia"));
    testProcess.setArguments({QStringLiteral("--version")});
    testProcess.start();
    
    if (testProcess.waitForFinished(5000)) {
        m_processAvailable = (testProcess.exitCode() == 0);
        if (m_processAvailable) {
            qCDebug(KDEV_JULIA) << "Julia is available";
        } else {
            qCWarning(KDEV_JULIA) << "Julia process failed with exit code:" << testProcess.exitCode();
        }
    } else {
        qCWarning(KDEV_JULIA) << "Julia process timed out";
        m_processAvailable = false;
    }
}

bool JuliaBridge::isJuliaAvailable() const
{
    return m_processAvailable;
}

QByteArray JuliaBridge::parseToJson(const QString& juliaCode)
{
    if (!m_processAvailable) {
        m_lastError = QStringLiteral("Julia is not available");
        return QByteArray();
    }
    
    QStringList args;
    args << QStringLiteral("--project=") + m_juliaProjectDir
         << m_bridgeScriptPath
         << QStringLiteral("--stdin") 
         << QStringLiteral("code");
    
    QByteArray result = runJuliaProcess(args, juliaCode.toUtf8());
    
    // Check if result is valid JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(result, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = QStringLiteral("Failed to parse JSON output: ") + parseError.errorString();
        qCDebug(KDEV_JULIA) << "JSON parse error:" << parseError.errorString();
        qCDebug(KDEV_JULIA) << "Raw output:" << result;
        return QByteArray();
    }
    
    // Check for errors in the JSON
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        if (obj.contains(QStringLiteral("error")) && obj[QStringLiteral("error")].toBool()) {
            m_lastError = obj[QStringLiteral("message")].toString();
            qCDebug(KDEV_JULIA) << "Julia parsing error:" << m_lastError;
            return QByteArray();
        }
    }
    
    return result;
}

QByteArray JuliaBridge::parseFileToJson(const QString& filepath)
{
    if (!m_processAvailable) {
        m_lastError = QStringLiteral("Julia is not available");
        return QByteArray();
    }
    
    // Read the file content
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_lastError = QStringLiteral("Failed to open file: ") + filepath;
        return QByteArray();
    }
    
    QByteArray content = file.readAll();
    file.close();
    
    // For file parsing, we can use the file directly
    QStringList args;
    args << QStringLiteral("--project") << m_juliaProjectDir
         << m_bridgeScriptPath
         << filepath;
    
    QByteArray result = runJuliaProcess(args, QByteArray());
    
    // Check if result is valid JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(result, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        m_lastError = QStringLiteral("Failed to parse JSON output: ") + parseError.errorString();
        return QByteArray();
    }
    
    // Check for errors in the JSON
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        if (obj.contains(QStringLiteral("error")) && obj[QStringLiteral("error")].toBool()) {
            m_lastError = obj[QStringLiteral("message")].toString();
            return QByteArray();
        }
    }
    
    return result;
}

QByteArray JuliaBridge::runJuliaProcess(const QStringList& args, const QByteArray& stdinData)
{
    QProcess process;
    
    process.setProgram(QStringLiteral("julia"));
    process.setArguments(args);
    
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    process.setProcessEnvironment(env);
    
    process.start();
    
    if (!process.waitForStarted(5000)) {
        m_lastError = QStringLiteral("Failed to start Julia process");
        return QByteArray();
    }
    
    if (!stdinData.isEmpty()) {
        process.write(stdinData);
        process.closeWriteChannel();
    }
    
    if (!process.waitForFinished(30000)) {
        process.terminate();
        process.waitForFinished(1000);
        m_lastError = QStringLiteral("Julia process timed out");
        return QByteArray();
    }
    
    if (process.exitCode() != 0) {
        QByteArray stderrOutput = process.readAllStandardError();
        m_lastError = QStringLiteral("Julia process failed with exit code %1: %2")
            .arg(process.exitCode())
            .arg(QString::fromUtf8(stderrOutput));
        return QByteArray();
    }
    
    return process.readAllStandardOutput();
}

QString JuliaBridge::lastError() const
{
    return m_lastError;
}

void JuliaBridge::setBridgeScriptPath(const QString& bridgeScriptPath)
{
    m_bridgeScriptPath = bridgeScriptPath;
}

void JuliaBridge::setJuliaProjectDir(const QString& projectDir)
{
    m_juliaProjectDir = projectDir;
}

void JuliaBridge::onJuliaFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    qCDebug(KDEV_JULIA) << "Julia process finished";
}

void JuliaBridge::onJuliaErrorOccurred(QProcess::ProcessError error)
{
    qCWarning(KDEV_JULIA) << "Julia process error:" << error;
    m_lastError = QStringLiteral("Julia process error: %1").arg(error);
}

} // namespace Julia