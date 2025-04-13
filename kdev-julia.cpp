#include "kdev-julia.h"

#include <debug.h>

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(kdev_juliaFactory, "kdev-julia.json", registerPlugin<kdev_julia>(); )

kdev_julia::kdev_julia(QObject *parent, const QVariantList& args)
    : KDevelop::IPlugin(QStringLiteral("kdev-julia"), parent)
{
    Q_UNUSED(args);

    qCDebug(PLUGIN_KDEV_JULIA) << "Hello world, my plugin is loaded!";
}

// needed for QObject class created from K_PLUGIN_FACTORY_WITH_JSON
#include "kdev-julia.moc"
#include "moc_kdev-julia.cpp"
