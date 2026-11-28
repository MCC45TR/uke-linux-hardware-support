// SPDX-License-Identifier: MIT
#include "settings.hpp"
#include <KLocalizedString>
#include <algorithm>
#include <memory>

namespace uq {
QVariantMap Settings::updateChannel() const {
  const auto status=data_.value("channel.status");
  const auto selected=status.value("selected").toString();
  const QStringList channels{"stable","test","unstable"};
  const auto report=status.value("repositories");
  const bool known=status.value("schema").toString()=="org.uke.control.channel.v1" &&
      report.isArray() && !errors_.contains("channel.status");
  const auto repositories=report.toArray();
  const auto enabled=std::count_if(repositories.cbegin(),repositories.cend(),[](const auto &repo) {
    return repo.toObject().value("enabled").toBool();
  });
  const auto active=known && enabled>0 && channels.contains(selected) ? selected : QString();
  return {{"active",active},{"known",known},
          {"multiple",known && enabled>1 && active.isEmpty()},{"ready",!active.isEmpty()},
          {"kernel",safe(status.value("running_kernel").toString().left(120).toStdString())}};
}

void Settings::runPackageOperation(const QString &operation,const QString &channel) {
  if (fixture_.enabled || restricted_ || packageProcess_ || !topic_ || topic_->id!="updates" || busy())
    return;
  const bool preview=operation=="check";
  const bool switchChannel=operation=="channel";
  const auto selection=updateChannel();
  if (switchChannel) {
    const auto choices=actionChoices("channel.set","channel");
    const auto valid=std::any_of(choices.cbegin(),choices.cend(),[&](const auto &choice){
      return choice.toMap().value("value").toString()==channel;
    });
    if (!valid || (selection.value("ready").toBool() && selection.value("active").toString()==channel)) return;
  } else if ((!preview && operation!="update") || !channel.isEmpty()) {
    return;
  } else if (!selection.value("ready").toBool()) {
    return;
  }
  auto *process=new QProcess(this);
  packageProcess_=process;
  packageOperation_={{"running",true},{"message",preview?i18n("Checking for updates…"):
      switchChannel?i18n("Changing the update channel…"):i18n("Installing system updates…")}};
  auto buffer=std::make_shared<QByteArray>();
  const auto drain=[process,buffer] {
    buffer->append(process->readAllStandardOutput());
    if (buffer->size()>2*1024*1024) buffer->remove(0,buffer->size()-2*1024*1024);
  };
  connect(process,&QProcess::readyReadStandardOutput,this,drain);
  process->setStandardErrorFile(QProcess::nullDevice());
  const auto finish=[this,process,buffer,drain,preview,switchChannel](bool ok) {
    if (packageProcess_!=process) return;
    drain();
    packageProcess_=nullptr;
    process->deleteLater();
    auto report=QJsonDocument::fromJson(*buffer).object();
    const bool confirmed=ok && report.value("schema").toString()=="org.uke.control.packages.v1" &&
        (preview?report.value("preview_only").toBool():report.value("operation_completed").toBool());
    QVariantList packages;
    if (confirmed && preview) {
      const auto items=report.value("packages").toArray();
      for (const auto &entry:items) {
        const auto item=entry.toObject();
        const auto action=item.value("action").toString();
        if (action=="Replaced" || action=="Remove" || action=="Removed") continue;
        if (packages.size()>=60) break;
        packages<<safe(item.value("package").toString().left(240).toStdString());
      }
    }
    auto detail=report.value("error").toObject().value("message").toString().trimmed().left(750);
    if (!confirmed && detail.isEmpty()) detail=i18n("Administrator authorization was cancelled or the update service could not start.");
    if (detail.contains("thirparty-repos-orcaslicer") && detail.contains("obsoletes"))
      detail=i18n("The new repository package conflicts with an older OrcaSlicer repository package. The system update is paused.");
    const auto message=confirmed ? (preview ? (packages.isEmpty()?i18n("The system is up to date."):i18n("Updates are available.")) :
        switchChannel?i18n("Update channel changed."):i18n("System update complete.")) :
        switchChannel?i18n("Could not change the update channel."):i18n("The update task could not be completed.");
    packageOperation_={{"running",false},{"error",!confirmed},{"message",message},
                       {"packages",packages},{"detail",confirmed?QString():safe(detail.toStdString())}};
    if (confirmed && !preview && topic_ && topic_->id=="updates") refresh();
    else emit changed();
  };
  connect(process,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,
          [finish](int code,QProcess::ExitStatus status){finish(status==QProcess::NormalExit && code==0);});
  connect(process,&QProcess::errorOccurred,this,[finish](QProcess::ProcessError error){if(error==QProcess::FailedToStart)finish(false);});
  QStringList args{"--disable-internal-agent","/usr/libexec/uke-control-qt/package-task",operation};
  if (switchChannel) args<<channel;
  process->start("/usr/bin/pkexec",args);
  emit changed();
}
}
