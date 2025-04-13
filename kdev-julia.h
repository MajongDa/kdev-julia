#ifndef KDEV_JULIA_H
#define KDEV_JULIA_H

#include <interfaces/iplugin.h>

class kdev_julia : public KDevelop::IPlugin
{
    Q_OBJECT

public:
    // KPluginFactory-based plugin wants constructor with this signature
    kdev_julia(QObject* parent, const QVariantList& args);
};

#endif // KDEV_JULIA_H
