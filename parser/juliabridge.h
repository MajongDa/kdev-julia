#ifndef JULIA_BRIDGE_H
#define JULIA_BRIDGE_H

#include <QObject>
#include <QProcess>
#include <QByteArray>
#include <QString>
#include <QStringList>

namespace Julia {

/**
 * Bridge class for communicating with Julia parser
 * Manages the Julia process and handles JSON serialization/deserialization
 */
class JuliaBridge : public QObject
{
    Q_OBJECT

public:
    explicit JuliaBridge(QObject* parent = nullptr);
    ~JuliaBridge() override;
    
    /**
     * Check if Julia is available on the system
     * @return true if Julia can be executed
     */
    bool isJuliaAvailable() const;
    
    /**
     * Parse Julia source code and return JSON AST
     * @param juliaCode The Julia source code to parse
     * @return JSON representation of the AST, or empty QByteArray on error
     */
    QByteArray parseToJson(const QString& juliaCode);
    
    /**
     * Parse Julia file and return JSON AST
     * @param filepath Path to the Julia file to parse
     * @return JSON representation of the AST, or empty QByteArray on error
     */
    QByteArray parseFileToJson(const QString& filepath);
    
    /**
     * Get the last error message if parsing failed
     * @return Error message, or empty string if no error
     */
    QString lastError() const;
    
    /**
     * Set the path to the Julia parser bridge script
     * @param bridgeScriptPath Absolute path to parser_bridge.jl
     */
    void setBridgeScriptPath(const QString& bridgeScriptPath);
    
    /**
     * Set the Julia project directory
     * @param projectDir Absolute path to Julia project directory
     */
    void setJuliaProjectDir(const QString& projectDir);

private Q_SLOTS:
    void onJuliaFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onJuliaErrorOccurred(QProcess::ProcessError error);

private:
    void initializeProcess();
    QByteArray runJuliaProcess(const QStringList& args, const QByteArray& stdinData = QByteArray());
    
    QString m_bridgeScriptPath;
    QString m_juliaProjectDir;
    QString m_lastError;
    bool m_processAvailable;
};

} // namespace Julia

#endif // JULIA_BRIDGE_H