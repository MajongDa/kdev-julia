#ifndef JULIAEXECUTIONLAUNCHER_H
#define JULIAEXECUTIONLAUNCHER_H

#include <interfaces/ilauncher.h>

namespace Julia {

class JuliaExecutionLauncher : public KDevelop::ILauncher
{
public:
    JuliaExecutionLauncher();
    QList< KDevelop::LaunchConfigurationPageFactory* > configPages() const override;
    QString description() const override;
    QString id() override;
    QString name() const override;
    KJob* start(const QString& launchMode, KDevelop::ILaunchConfiguration* cfg) override;
    QStringList supportedModes() const override;
};

}

#endif // JULIAEXECUTIONLAUNCHER_H