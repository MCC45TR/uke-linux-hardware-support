// SPDX-License-Identifier: MIT
#pragma once
#include "model-data.hpp"
#include "settings.hpp"
#include <optional>
namespace uq {
class Bridge : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool simulation READ simulation CONSTANT)
  Q_PROPERTY(bool readOnly READ readOnly CONSTANT)
  Q_PROPERTY(QObject *settings READ settings CONSTANT)
  Q_PROPERTY(QVariantList areas READ areas CONSTANT)
  Q_PROPERTY(int section READ section NOTIFY sectionChanged)
  Q_PROPERTY(QVariantList groups READ groups NOTIFY sectionChanged)
  Q_PROPERTY(QVariantMap guide READ guide NOTIFY guideChanged)
  Q_PROPERTY(QVariantMap draftValues READ draftValues NOTIFY draftChanged)
  Q_PROPERTY(QVariantMap observation READ observation NOTIFY observationChanged)
  Q_PROPERTY(QVariantMap review READ review NOTIFY reviewChanged)
  Q_PROPERTY(QString message READ message NOTIFY messageChanged)
  Q_PROPERTY(bool messageError READ messageError NOTIFY messageChanged)
  Fixture fixture_;
  Settings settings_;
  bool readOnly_;
  int section_ = 0;
  QVariantMap guide_, observation_, review_;
  QString message_;
  bool messageError_ = false;
  Values draft_, readValues_;
  std::string readId_;
  QString readOrigin_;
  bool observationStale_ = false;
  std::optional<Plan> reviewed_;
  std::unique_ptr<Query> query_;
  QTimer poll_, simulated_;
  QElapsedTimer elapsed_;
  Result result_;
  void showResult(const Result &result);
  void startRead(const Plan &request, const QString &origin = "read");
  void prepare(const Plan &request);
  void fail(const std::exception &error);

public:
  explicit Bridge(Fixture fixture, bool readOnly, QObject *parent = nullptr);
  ~Bridge() override;
  bool simulation() const { return fixture_.enabled; }
  QObject *settings() { return &settings_; }
  bool readOnly() const { return readOnly_; }
  int section() const { return section_; }
  QVariantList areas() const;
  QVariantList groups() const;
  QVariantMap guide() const { return guide_; }
  QVariantMap draftValues() const { QVariantMap result;for(const auto &[k,v]:draft_)result[QString::fromStdString(k)]=QString::fromStdString(v);return result; }
  QVariantMap observation() const { return observation_; }
  QVariantMap review() const { return review_; }
  QString message() const { return message_; }
  bool messageError() const { return messageError_; }
  Q_INVOKABLE QVariantList search(const QString &text) const;
  Q_INVOKABLE QVariantList catalog(const QString &text, int area = -1,
                                   const QString &kind = "all") const;
  Q_INVOKABLE int areaForAction(const QString &id) const;
  Q_INVOKABLE bool openAction(const QString &id);
  Q_INVOKABLE void selectArea(int index);
  Q_INVOKABLE void leaveGuide();
  Q_INVOKABLE void setDraft(const QString &key, const QString &value);
  Q_INVOKABLE void resetDraft();
  Q_INVOKABLE void submit();
  Q_INVOKABLE void readRelated(const QString &relation);
  Q_INVOKABLE void refresh();
  Q_INVOKABLE void cancelRead();
  Q_INVOKABLE void cancelReview();
  Q_INVOKABLE bool confirmReview(bool consent, const QString &token);
  Q_INVOKABLE void clearMessage();
signals:
  void sectionChanged();
  void guideChanged();
  void draftChanged();
  void observationChanged();
  void reviewChanged();
  void messageChanged();
};
} // namespace uq
