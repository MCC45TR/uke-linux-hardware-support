// SPDX-License-Identifier: MIT
#include <QCoreApplication>
#include <KPluginFactory>
#include <KPluginMetaData>
#include <systemstats/SensorPlugin.h>
#include <systemstats/SensorContainer.h>
#include <cassert>
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);assert(argc==2);
    KPluginMetaData metadata(QString::fromLocal8Bit(argv[1]));assert(metadata.isValid());
    auto loaded=KPluginFactory::instantiatePlugin<KSysGuard::SensorPlugin>(metadata);
    assert(loaded.plugin&&loaded.plugin->providerName()==QStringLiteral("uke-adreno"));
    auto containers=loaded.plugin->containers();
    assert(containers.size()==1&&containers[0]->id()==QStringLiteral("adreno"));
    loaded.plugin->update();app.processEvents();delete loaded.plugin;
}
