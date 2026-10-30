// SPDX-License-Identifier: MIT
// Render the actual plugin with synthetic observations; no queries or writes.
#include <KPluginFactory>
#include <KQuickConfigModule>
#include <KQuickConfigModuleLoader>
#include <KPluginMetaData>
#include <KLocalizedString>
#include <QGuiApplication>
#include <QFont>
#include <QImage>
#include <QJsonArray>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlEngine>
#include <QTimer>
#include <QStandardPaths>
#include <cstdio>
#include <memory>

int main(int argc,char** argv) {
    if(argc!=3)return 64;
    QGuiApplication application(argc,argv);
    if(qEnvironmentVariableIsSet("UKE_TEST_LOCALE_DIR"))KLocalizedString::addDomainLocaleDir("kcm_uke",qEnvironmentVariable("UKE_TEST_LOCALE_DIR"));
    if(qEnvironmentVariable("LANGUAGE")==QStringLiteral("tr"))KLocalizedString::setLanguages({QStringLiteral("tr")});
    if(qEnvironmentVariableIsSet("UKE_RENDER_LARGE")){auto font=application.font();font.setPointSize(17);application.setFont(font);}
    const auto loaded=KQuickConfigModuleLoader::loadModule(KPluginMetaData(QString::fromLocal8Bit(argv[1])));
    if(!loaded){std::fprintf(stderr,"%s\n",qPrintable(loaded.errorText));return 1;}
    std::unique_ptr<KQuickConfigModule> module(loaded.plugin);
    if(!module)return 1;
    std::fprintf(stderr,"Plugin domain: %s; translated refresh: %s; data paths: %s\n",qPrintable(module->metaData().pluginId()),qPrintable(i18nd("kcm_uke","Refresh")),qPrintable(QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).join(':')));
    auto* model=module->property("overview").value<QObject*>();if(!model)return 1;
    QJsonArray cards;
    cards.append(QJsonObject{{"component","battery"},{"availability","available"},{"data",QJsonObject{{"readings",QJsonObject{{"battery.capacity",QJsonObject{{"value",76},{"unit","percent"},{"validity","reported"}}},{"battery.current_now",QJsonObject{{"value",-850000},{"unit","uA_positive_into_battery"},{"validity","unqualified"}}}}},{"battery.cycle_count",QJsonObject{{"value",0},{"unit","cycles"},{"validity","reported"}}}}}});
    cards.append(QJsonObject{{"component","charge"},{"availability","not_qualified"},{"data",QJsonObject{{"profile_qualified",false},{"applied",false}}}});
    cards.append(QJsonObject{{"component","display"},{"availability","available"},{"data",QJsonObject{{"session",QJsonObject{{"rotation",true},{"automatic_brightness",false}}}}}});
    cards.append(QJsonObject{{"component","usb"},{"availability","stale"},{"data",QJsonObject{{"typec",QJsonArray{QJsonObject{{"name","port0"},{"data_role","host"}}}}}}});
    const QJsonObject fixture{{"schema","org.uke.control.overview.v1"},{"components",cards}};bool accepted=false;
    if(!QMetaObject::invokeMethod(model,"setFixture",Qt::DirectConnection,Q_RETURN_ARG(bool,accepted),Q_ARG(QJsonObject,fixture))||!accepted)return 1;
    auto* item=module->mainUi();
    if(!item){std::fprintf(stderr,"%s\n",qPrintable(module->errorString()));return 1;}
    QQuickWindow window;window.resize(qEnvironmentVariableIsSet("UKE_RENDER_LARGE")?480:900,1100);item->setParentItem(window.contentItem());item->setSize(QSizeF(window.size()));window.show();
    QTimer::singleShot(1500,&application,[&]{
        const auto image=window.grabWindow();const bool saved=!image.isNull()&&image.save(QString::fromLocal8Bit(argv[2]));
        application.exit(saved?0:1);
    });
    return application.exec();
}
