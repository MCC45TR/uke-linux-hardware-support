// SPDX-License-Identifier: MIT
#include "settings.hpp"

#include <KLocalizedString>
#include <QDateTime>
#include <QElapsedTimer>
#include <QHash>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QtConcurrentRun>
#include <algorithm>
#include <array>

namespace uq {
namespace {
constexpr int maxRows = 1000;
constexpr qsizetype maxOutput = 12 * 1024 * 1024;
const QStringList journalOptions{"--quiet", "--no-pager", "--output=json"};

struct JournalResult {
  QByteArray output;
  bool complete = false;
};

JournalResult readJournal(const QStringList &arguments, QElapsedTimer &deadline) {
  QProcess process;
  auto environment = QProcessEnvironment::systemEnvironment();
  environment.insert("LC_ALL", "C");
  process.setProcessEnvironment(environment);
  process.setStandardErrorFile(QProcess::nullDevice());
  process.start("/usr/bin/journalctl", journalOptions + arguments, QIODevice::ReadOnly);

  JournalResult result;
  while (process.state() != QProcess::NotRunning) {
    const auto remaining = 20000 - deadline.elapsed();
    if (remaining <= 0 || result.output.size() > maxOutput) {
      process.kill();
      process.waitForFinished(1000);
      return result;
    }
    process.waitForReadyRead(qMin<qint64>(remaining, 250));
    result.output += process.readAllStandardOutput();
  }
  result.output += process.readAllStandardOutput();
  result.complete = process.error() != QProcess::FailedToStart &&
      process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0 &&
      result.output.size() <= maxOutput;
  return result;
}

QString textField(const QJsonObject &entry, const QString &field) {
  const auto value = entry.value(field);
  if (value.isString()) return value.toString();
  // JSON encodes a journal field containing non-UTF-8 as a byte array.
  if (!value.isArray()) return {};
  const auto values = value.toArray();
  if (values.isEmpty() || !values.first().isDouble() || values.size() > 4096) return {};
  QByteArray bytes;
  for (const auto &byte : values) bytes.append(static_cast<char>(byte.toInt()));
  auto text = QString::fromUtf8(bytes);
  text.replace(QChar::Null, QChar::ReplacementCharacter);
  return text;
}

QVariantMap scanJournal(const QVariantList &previous,int olderBoot) {
  QElapsedTimer deadline;
  deadline.start();
  QVariantList groups=previous;
  if (olderBoot<0) {
    const auto bootList = readJournal({"--list-boots", "--lines=3"}, deadline);
    const auto boots = QJsonDocument::fromJson(bootList.output).array();
    if (!bootList.complete || boots.isEmpty()) {
      return {{"message", i18n("No system journal was readable. Check this account’s journal access.")}};
    }
    groups.clear();
    for (qsizetype i=boots.size();i>0;--i) {
      const auto boot=boots.at(i-1).toObject();
      const auto index=boot.value("index").toInt();
      const auto start=QDateTime::fromMSecsSinceEpoch(boot.value("first_entry").toInteger()/1000);
      groups<<QVariantMap{{"id",boot.value("boot_id").toString()},
          {"title",index==0 ? i18n("Current boot") : i18n("Boot %1",index)},
          {"date",start.toString("yyyy-MM-dd HH:mm")}};
    }
  }

  // PRIORITY identifies native journal warnings. MESSAGE also catches service
  // stderr logged at info priority and kernel errors embedded in diagnostics.
  const QString errors = R"((?:\b(?:error|warn(?:ing)?|failed|failure|fatal|oops|panic|assertion|timeout|segfault|deadlock|denied|traceback)\b|)"
                         R"(timed out|device or resource busy|operation not permitted|)"
                         R"(\b(?:errno|status|rc|result|ret(?:urn(?:ed)?)?)\s*(?:[=:]|was)?\s*-[0-9]{1,4}\b|)"
                         R"(\bE(?:2BIG|ACCES|ADDRINUSE|ADDRNOTAVAIL|AGAIN|ALREADY|BADF|BADMSG|BUSY|CANCELED|CHILD|COMM|CONNABORTED|CONNREFUSED|CONNRESET|DEADLK|DESTADDRREQ|DOM|EXIST|FAULT|FBIG|HOSTDOWN|HOSTUNREACH|IDRM|ILSEQ|INPROGRESS|INTR|INVAL|IO|ISCONN|ISDIR|LOOP|MEDIUMTYPE|MFILE|MLINK|MSGSIZE|MULTIHOP|NAMETOOLONG|NETDOWN|NETRESET|NETUNREACH|NFILE|NOBUFS|NODATA|NODEV|NOENT|NOEXEC|NOKEY|NOLCK|NOLINK|NOMEDIUM|NOMEM|NOMSG|NOPROTOOPT|NOSPC|NOSR|NOSTR|NOSYS|NOTBLK|NOTCONN|NOTDIR|NOTEMPTY|NOTRECOVERABLE|NOTSOCK|NOTSUP|NOTTY|NXIO|OPNOTSUPP|OVERFLOW|OWNERDEAD|PERM|PIPE|PROTO|PROTONOSUPPORT|PROTOTYPE|RANGE|REMOTE|REMOTEIO|RESTARTSYS|ROFS|SHUTDOWN|SPIPE|SRCH|STALE|TIME|TIMEDOUT|TXTBSY|UCLEAN|UNATCH|USERS|WOULDBLOCK|XDEV)\b))";
  static const QRegularExpression bootId("^[0-9a-f]{32}$");
  bool incomplete = false;
  for (int bootIndex=0;bootIndex<groups.size();++bootIndex) {
    if (olderBoot>=0 && bootIndex!=olderBoot) continue;
    auto boot=groups[bootIndex].toMap();
    const auto id = boot.value("id").toString();
    if (!bootId.match(id).hasMatch()) continue;
    const QStringList options{"--boot=" + id, "--reverse", "--lines=" + QString::number(maxRows + 1),
        "--output-fields=MESSAGE,PRIORITY,_SYSTEMD_UNIT,_SYSTEMD_USER_UNIT,SYSLOG_IDENTIFIER,_COMM"};
    QHash<QString, QVariantMap> entries;
    for (const auto &entry : boot.value("rows").toList()) {
      const auto row=entry.toMap();
      entries.insert(row.value("cursor").toString(),row);
    }
    auto cursors=boot.value("cursors").toStringList();
    if (cursors.isEmpty()) cursors=QStringList{QString(),QString()};
    const std::array filters{QStringList{"--priority=warning"},QStringList{"--case-sensitive=no","--grep="+errors}};
    for (int filter=0;filter<int(filters.size());++filter) {
      auto arguments=options+filters[filter];
      if (olderBoot>=0) {
        if (cursors[filter].isEmpty()) continue;
        arguments<<"--after-cursor="+cursors[filter];
      }
      const auto journal = readJournal(arguments, deadline);
      incomplete |= !journal.complete;
      int found = 0;
      QString lastCursor;
      bool limited=false;
      for (const auto &line : journal.output.split('\n')) {
        if (line.isEmpty()) continue;
        if (++found > maxRows) {
          limited = true;
          break;
        }
        const auto entry = QJsonDocument::fromJson(line).object();
        const auto cursor = entry.value("__CURSOR").toString();
        if (cursor.isEmpty()) continue;
        lastCursor=cursor;
        if (entries.contains(cursor)) continue;
        const auto timestamp = entry.value("__REALTIME_TIMESTAMP").toString().toLongLong();
        const auto monotonic = entry.value("__MONOTONIC_TIMESTAMP").toString().toLongLong();
        bool hasPriority = false;
        const auto priority = entry.value("PRIORITY").toString().toInt(&hasPriority);
        QString source;
        for (const auto field : {"_SYSTEMD_USER_UNIT", "_SYSTEMD_UNIT", "SYSLOG_IDENTIFIER", "_COMM"}) {
          source = textField(entry, QString::fromLatin1(field));
          if (!source.isEmpty()) break;
        }
        const auto message = textField(entry, "MESSAGE");
        entries.insert(cursor, {{"cursor",cursor},{"message", message.isEmpty() ? i18n("Message unavailable in the journal") : message.left(4096)},
            {"source", source.left(160)},
            {"time", QDateTime::fromMSecsSinceEpoch(timestamp / 1000).toString("yyyy-MM-dd HH:mm:ss")},
            {"monotonic", monotonic},
            {"level", hasPriority && priority < 4 ? "error" : hasPriority && priority == 4 ? "warning" : "message"}});
      }
      if (journal.complete || !lastCursor.isEmpty())
        cursors[filter]=limited || !journal.complete ? lastCursor : QString();
    }
    auto sorted = entries.values();
    std::stable_sort(sorted.begin(), sorted.end(), [](const QVariantMap &a, const QVariantMap &b) {
      return a.value("monotonic").toLongLong() > b.value("monotonic").toLongLong();
    });
    QVariantList rows;
    for (const auto &row : sorted) rows << row;
    boot["rows"]=rows;
    boot["cursors"]=cursors;
    boot["limited"]=std::any_of(cursors.cbegin(),cursors.cend(),[](const QString &cursor){return !cursor.isEmpty();});
    groups[bootIndex]=boot;
  }
  return {{"boots", groups}, {"message", incomplete ? i18n("The scan was incomplete. Some journal entries could not be read.") : QString()}};
}
}

void Settings::scanSystemErrors(int olderBoot) {
  if (!topic_ || topic_->id != "system-errors" || systemErrorsScan_.isRunning()) return;
  const auto previous=systemErrors_.value("boots").toList();
  if (olderBoot < -1 || olderBoot>=previous.size()) return;
  if (fixture_.enabled) {
    const QVariantList rows{QVariantMap{{"message", "Atomic commit returned EBUSY"},
        {"source", "kernel"}, {"time", "2026-11-28 09:00:01"}, {"level", "message"}}};
    systemErrors_ = {{"boots", QVariantList{QVariantMap{{"title", i18n("Current boot")}, {"date", "2026-11-28 09:00"},
        {"rows", rows}, {"limited", false}}}}, {"message", QString()}};
    emit systemErrorsChanged();
    return;
  }
  connect(&systemErrorsScan_, &QFutureWatcher<QVariantMap>::finished, this, [this] {
    systemErrors_ = systemErrorsScan_.result();
    emit systemErrorsChanged();
  }, Qt::SingleShotConnection);
  systemErrors_["busy"] = true;
  systemErrorsScan_.setFuture(QtConcurrent::run(scanJournal,previous,olderBoot));
  emit systemErrorsChanged();
}
} // namespace uq
