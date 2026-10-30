// SPDX-License-Identifier: MIT
#include "overview-model.hpp"
#include <KPluginFactory>
#include <KQuickConfigModule>

class UkeModule:public KQuickConfigModule {
    Q_OBJECT
    Q_PROPERTY(QObject* overview READ overview CONSTANT)
    OverviewModel model_;
public:
    UkeModule(QObject* parent,const KPluginMetaData& metadata):KQuickConfigModule(parent,metadata),model_(this){setButtons(NoAdditionalButton);}
    QObject* overview(){return &model_;}
};
K_PLUGIN_CLASS_WITH_JSON(UkeModule,"../kcm_uke.json")
#include "kcm.moc"
