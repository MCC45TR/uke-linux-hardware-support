// SPDX-License-Identifier: MIT
#include "settings.hpp"
#include <KLocalizedString>
#include <QDate>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
#include <memory>

namespace uq {
namespace {
constexpr qsizetype maxChangelogBytes = 128 * 1024;

QVariantMap readChangelog(const QByteArray &bytes) {
  const auto report=QJsonDocument::fromJson(bytes).object();
  if (report.value("schema").toString()!="org.uke.control.changelog.v1") return {};
  static const QRegularExpression transition(R"(^[0-9]+\.[0-9]+\.[0-9]+ → [0-9]+\.[0-9]+\.[0-9]+$)");
  const auto label=[](const QJsonValue &value,qsizetype limit) {
    return safe(value.toString().simplified().left(limit).toStdString());
  };
  const auto cardsFor=[&](const QString &section) {
  const bool kernel=section=="kernel_releases";
  QVariantList cards;
  QSet<QString> versions;
  for (const auto &entry:report.value(section).toArray()) {
    if (cards.size()>=24) break;
    const auto record=entry.toObject();
    const auto version=label(record.value("version"),64);
    const auto date=QDate::fromString(record.value("date").toString(),Qt::ISODate);
    const auto summary=label(record.value("summary"),120);
    if (version.isEmpty() || !date.isValid() || summary.isEmpty()) continue;
    if (kernel && (!transition.match(version).hasMatch() || versions.contains(version))) continue;
    QVariantList rows;
    for (const auto &change:record.value("changes").toArray()) {
      if (rows.size()>=8) break;
      const auto description=label(change,240);
      if (!description.isEmpty())
        rows<<QVariantMap{{"title",description},{"value",QString()}};
    }
    if (rows.isEmpty()) continue;
    versions.insert(version);
    cards<<QVariantMap{{"key",version+'-'+date.toString(Qt::ISODate)},{"title",summary},
        {"message",i18n("%1 · %2",version,QLocale().toString(date,QLocale::ShortFormat))},
        {"rows",rows},{"previewCount",3}};
  }
  return cards;
  };
  return {{"entries",cardsFor("entries")},{"kernel_releases",cardsFor("kernel_releases")}};
}
}

void Settings::loadChangelog() {
  if (changelogReply_) return;
  if (changelog_.value("entries").toList().isEmpty()) {
    QFile shipped(":/uke-control-qt/changelog/index.json");
    if (shipped.open(QIODevice::ReadOnly)) {
      const auto local=readChangelog(shipped.read(maxChangelogBytes));
      for (auto it=local.cbegin();it!=local.cend();++it) changelog_.insert(it.key(),it.value());
    }
  }
  if (fixture_.enabled || changelogOnline_) { emit changelogChanged(); return; }
  if (!changelogNetwork_) changelogNetwork_=new QNetworkAccessManager(this);
  QNetworkRequest request(QUrl("https://raw.githubusercontent.com/MCC45TR/uke-linux-hardware-support/control-changelog/uke-control-qt/changelog/index.json"));
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::SameOriginRedirectPolicy);
  request.setTransferTimeout(7500);
  auto *reply=changelogNetwork_->get(request);
  reply->setReadBufferSize(maxChangelogBytes+1);
  changelogReply_=reply;
  auto bytes=std::make_shared<QByteArray>();
  const auto collect=[reply,bytes] {
    bytes->append(reply->read(maxChangelogBytes + 1 - bytes->size()));
    if (bytes->size()>maxChangelogBytes) reply->abort();
  };
  connect(reply,&QNetworkReply::readyRead,this,collect);
  connect(reply,&QNetworkReply::finished,this,[this,reply,bytes,collect] {
    changelogReply_=nullptr;
    if (reply->error()==QNetworkReply::NoError) {
      collect();
      const auto remote=bytes->size()<=maxChangelogBytes ? readChangelog(*bytes) : QVariantMap{};
      if (!remote.value("entries").toList().isEmpty()) {
        changelog_["entries"]=remote.value("entries");
        if (!remote.value("kernel_releases").toList().isEmpty())
          changelog_["kernel_releases"]=remote.value("kernel_releases");
        changelogOnline_=true;
      }
    }
    changelog_["message"]=changelogOnline_ ? QString() : i18n("Showing the change log included with Uke Control. GitHub could not be read.");
    reply->deleteLater();
    emit changelogChanged();
  });
  emit changelogChanged();
}
} // namespace uq
