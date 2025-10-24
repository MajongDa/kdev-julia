#ifndef JULIAEXECUTIONJOB_H
#define JULIAEXECUTIONJOB_H

#include <QProcess>
#include <outputview/outputjob.h>
#include <outputview/outputmodel.h>
#include <util/processlinemaker.h>

class KProcess;

namespace Julia {

class JuliaExecutionJob : public KDevelop::OutputJob
{
Q_OBJECT
public:
    JuliaExecutionJob(const QStringList& interpreter, const QString& scriptPath,
                      const QStringList& arguments, const QUrl& workingDirectory,
                      const QString& environmentProfileName, int graphicsMethod = -1,
                      const QString& graphicsDir = QString(), const QString& socketHost = QString(),
                      int socketPort = 8080);
    void start() override;
    bool doKill() override;
    KDevelop::OutputModel* model();
private Q_SLOTS:
    void processError(QProcess::ProcessError);
    void processFinished(int, QProcess::ExitStatus);
private:
    void appendLine(const QString &l);
    QString findStartupScript();
    KProcess* proc;
    KDevelop::ProcessLineMaker* lineMaker;
};

}

#endif // JULIAEXECUTIONJOB_H