// SPDX-License-Identifier: MIT
#pragma once
#include <QtWidgets>
#if defined(UKE_CONTROL_SOURCE_HEADERS)
#include <client.hpp>
#elif __has_include(<uke-control/client.hpp>)
#include <uke-control/client.hpp>
#else
#include <client.hpp>
#endif
#include <memory>
#include <optional>
namespace uq {
using namespace uke::client;
struct Fixture {
  bool enabled = false;
  QJsonObject results;
  static Fixture load(const QString &path);
  Result result(const std::string &id) const;
  int delay(const std::string &id) const;
};
QString safe(const std::string &text);
class Observation : public QWidget {
  Q_OBJECT
  Fixture fixture_;
  std::unique_ptr<Query> query_;
  QTimer poll_, simulated_;
  QElapsedTimer elapsed_;
  QString id_;
  Result result_;
  QLabel *state_, *stamp_;
  QTreeWidget *tree_;
  QPlainTextEdit *raw_, *error_;
  QPushButton *refresh_, *stop_;
  Values values_;
  unsigned displayedValues_ = 0;
  void showResult(const Result &result);
  void addValue(QTreeWidgetItem *parent, const QString &key,
                const QJsonValue &value, unsigned depth = 0);

public:
  Observation(Fixture fixture, QWidget *parent = nullptr);
  ~Observation() override;
  void start(const std::string &id, const Values &values = {});
  void cancel();
  bool running() const;
  const Result &result() const { return result_; }
};
class Window : public QMainWindow {
  Q_OBJECT
  Fixture fixture_;
  bool readOnly_;
  QListWidget *navigation_;
  QStackedWidget *stack_;
  QWidget *page_ = nullptr, *detail_ = nullptr;
  QLabel *pageTitle_, *pageSubtitle_;
  QString styleNotice_;
  int section_ = 0;
  std::string actionId_;
  std::map<std::string, QWidget *> fields_;
  QPushButton *primary_ = nullptr;
  Observation *observation_ = nullptr;
  Values values() const;
  void showSection(int index);
  void addGroup(QVBoxLayout *layout, const QString &title,
                const QString &subtitle, const std::vector<std::string> &ids);
  void inspect(const std::string &id);
  void submit();
  void review(const Plan &request);

public:
  Window(Fixture fixture, bool readOnly, QString styleNotice = {});
  ~Window() override;
  void openAction(const std::string &id);
  void back();
  void closeEvent(QCloseEvent *event) override;
};
} // namespace uq
