// SPDX-License-Identifier: MIT
#include "ui.hpp"
#if defined(UKE_CONTROL_SOURCE_HEADERS)
#include <frontend.hpp>
#elif __has_include(<uke-control/frontend.hpp>)
#include <uke-control/frontend.hpp>
#else
#include <frontend.hpp>
#endif
#include <algorithm>
#include <climits>
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
namespace uq {
static QString q(const std::string &s) {
  return QString::fromUtf8(s.data(), int(s.size()));
}
QString safe(const std::string &text) {
  QString out;
  for (const QChar ch : q(text)) {
    const auto u = ch.unicode();
    if ((u < 32 && u != '\n' && u != '\t') || (u >= 0x7f && u <= 0x9f) ||
        (u >= 0x202a && u <= 0x202e) || (u >= 0x2066 && u <= 0x2069))
      out += QString("\\u%1").arg(unsigned(u), 4, 16, QChar('0'));
    else
      out += ch;
  }
  return out;
}
Fixture Fixture::load(const QString &path) {
  const auto bytes = QFile::encodeName(path);
  const int fd =
      ::open(bytes.constData(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
  if (fd < 0)
    throw std::runtime_error(
        "Cannot open fixture as a non-symlink regular file");
  struct stat st{};
  if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 2 ||
      st.st_size > 1024 * 1024) {
    ::close(fd);
    throw std::runtime_error("Fixture must be a regular file of at most 1 MiB");
  }
  QByteArray data;
  char buffer[4096];
  ssize_t count;
  while ((count = ::read(fd, buffer, sizeof(buffer))) > 0) {
    data.append(buffer, int(count));
    if (data.size() > 1024 * 1024)
      break;
  }
  ::close(fd);
  if (count < 0 || data.size() > 1024 * 1024)
    throw std::runtime_error("Cannot read bounded fixture");
  QJsonParseError error;
  auto doc = QJsonDocument::fromJson(data, &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject() ||
      doc.object().value("schema").toString() !=
          "org.uke.control.tui-fixture.v1" ||
      !doc.object().value("results").isObject())
    throw std::runtime_error("Invalid org.uke.control.tui-fixture.v1 fixture");
  const auto root = doc.object();
  for (auto it = root.begin(); it != root.end(); ++it)
    if (it.key() != "schema" && it.key() != "results")
      throw std::runtime_error("Unknown fixture field");
  Fixture fixture;
  fixture.enabled = true;
  fixture.results = doc.object().value("results").toObject();
  for (auto it = fixture.results.begin(); it != fixture.results.end(); ++it) {
    action(it.key().toStdString());
    if (!it.value().isObject())
      throw std::runtime_error("Fixture result must be an object");
    const auto entry = it.value().toObject();
    for (auto field = entry.begin(); field != entry.end(); ++field)
      if (field.key() != "data" && field.key() != "stdout" &&
          field.key() != "stderr" && field.key() != "status" &&
          field.key() != "delay_ms")
        throw std::runtime_error("Unknown fixture result field");
    if (entry.contains("data") && entry.contains("stdout"))
      throw std::runtime_error("Fixture data/stdout are mutually exclusive");
    for (const auto &key : {"stdout", "stderr"})
      if (entry.contains(key) && !entry.value(key).isString())
        throw std::runtime_error("Fixture stdout/stderr must be strings");
    for (const auto &key : {"delay_ms", "status"})
      if (entry.contains(key) &&
          (!entry.value(key).isDouble() ||
           entry.value(key).toDouble() != entry.value(key).toInt(-1)))
        throw std::runtime_error(
            "Fixture delay/status must be bounded integers");
    if (entry.value("delay_ms").toInt() < 0 ||
        entry.value("delay_ms").toInt() > 3000 ||
        entry.value("status").toInt() < 0 ||
        entry.value("status").toInt() > 255)
      throw std::runtime_error("Fixture delay/status outside allowed range");
  }
  return fixture;
}
Result Fixture::result(const std::string &id) const {
  Result r;
  r.action_id = id;
  if (!results.contains(q(id))) {
    r.exit_status = 69;
    r.error =
        "Unavailable: no simulated result for this action. No command was run.";
    return r;
  }
  const auto entry = results.value(q(id)).toObject();
  r.exit_status = entry.value("status").toInt();
  if (entry.contains("stdout"))
    r.output = entry.value("stdout").toString().toStdString();
  else if (entry.contains("data")) {
    const auto data = entry.value("data");
    if (data.isObject())
      r.output = QJsonDocument(data.toObject()).toJson().toStdString();
    else if (data.isArray())
      r.output = QJsonDocument(data.toArray()).toJson().toStdString();
    else {
      QJsonArray a;
      a.append(data);
      auto s = QJsonDocument(a).toJson(QJsonDocument::Compact);
      r.output = s.mid(1, s.size() - 2).toStdString();
    }
  }
  r.error = entry.value("stderr").toString().toStdString();
  return r;
}
int Fixture::delay(const std::string &id) const {
  return results.value(q(id)).toObject().value("delay_ms").toInt();
}
static QLabel *label(const QString &text, QWidget *parent = nullptr) {
  auto *l = new QLabel(text, parent);
  l->setTextFormat(Qt::PlainText);
  l->setWordWrap(true);
  l->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return l;
}
static QLabel *heading(const QString &text, int size) {
  auto *l = label(text);
  auto font = l->font();
  font.setPointSize(size);
  font.setBold(true);
  l->setFont(font);
  return l;
}
static QPushButton *button(const QString &text, const QString &icon = {}) {
  auto *b = new QPushButton(QIcon::fromTheme(icon), text);
  b->setMinimumHeight(40);
  return b;
}
static QString friendlyTitle(const Action &a) {
  static const std::map<std::string, QString> names = {
      {"status", "Device status"},
      {"overview.battery", "Battery overview"},
      {"overview.workload", "System activity"},
      {"hardware.status", "Hardware information"},
      {"overview.desktop", "Desktop information"},
      {"overview.storage", "Storage overview"},
      {"battery.status", "Battery readings"},
      {"battery.health", "Battery health"},
      {"charge.mode", "Charging preference"},
      {"charge.limit", "Charge limit"},
      {"charge.status", "Charging status"},
      {"charge.admission", "Charging availability"},
      {"charge.temporary-preview", "Temporary full-charge preview"},
      {"desktop.plasma", "Rotation & brightness"},
      {"desktop.rotation", "Automatic rotation"},
      {"desktop.brightness", "Automatic brightness"},
      {"flashlight.state", "Flashlight"},
      {"flashlight.level", "Flashlight brightness"},
      {"led.color", "Notification light color"},
      {"led.auto", "Automatic notification light"},
      {"led.status", "Notification light"},
      {"display.bits", "Display connection"},
      {"display.profile", "Display connection preference"},
      {"usb.status", "USB connections"},
      {"usb.role", "USB data role"},
      {"usb.power", "USB accessory power"},
      {"ethernet.status", "Ethernet connection"},
      {"ethernet.profile", "Ethernet preference"},
      {"microphone.select", "Microphone preference"},
      {"microphone.status", "Microphone"},
      {"channel.set", "Software channel"},
      {"channel.status", "Channel & pending updates"},
      {"update.apply", "Install updates"},
      {"channel.preview", "Preview software channel"},
      {"update.preview", "Check for updates"},
      {"sensors.once", "Read sensors once"},
      {"sensors.watch", "Watch sensors briefly"}};
  const auto found = names.find(a.id);
  return found == names.end() ? q(a.title.en) : found->second;
}
static QString summary(const Action &a) {
  if (a.effect == Effect::maintenance)
    return q(a.description.en);
  if (a.effect == Effect::publish)
    return "This publishes a generated summary externally. Review the summary "
           "and the exact destination before continuing.";
  if (a.id == "charge.mode")
    return "Choose how charging should be limited. Review changes before "
           "applying; availability is checked before a request is accepted.";
  if (a.id == "charge.limit")
    return "Choose a requested charge limit. Check charging availability "
           "before applying.";
  if (a.id == "charge.admission")
    return "See whether charging controls are available and what is blocking "
           "them.";
  if (a.id.starts_with("charge.") && a.effect == Effect::read)
    return "Read charging information. Unavailable or unknown values stay "
           "visible.";
  if (a.id == "flashlight.level")
    return "Choose a brightness request. The light has two lit steps: 1–99% "
           "maps to 50%, and 100% maps to full brightness.";
  if (a.id == "flashlight.state")
    return "Turn the flashlight on or off. It has no automatic timeout and "
           "turns off before sleep.";
  if (a.id == "usb.role")
    return "Choose the USB data role. Changing it can disconnect accessories "
           "or a remote session; inspect the connection first.";
  if (a.id == "ethernet.profile")
    return "Choose a supported link preference. Applying changes can interrupt "
           "your network connection.";
  if (a.id == "display.profile" || a.id == "display.lanes" ||
      a.id == "display.compression-set")
    return "Review the current display connection first. Changing this "
           "preference can disconnect a display or accessory.";
  if (a.effect == Effect::packages)
    return "Review the signed software plan before continuing. Inspect "
           "unfinished work before retrying, and wait for updates to finish "
           "before rebooting.";
  if (a.id.starts_with("test."))
    return "Keep the device attended during this diagnostic. Review Details "
           "before continuing.";
  if (a.id.starts_with("desktop.") && a.effect != Effect::read)
    return "Choose a preference for your current desktop session. Changes use "
           "the existing desktop controls.";
  if (a.effect == Effect::local_file)
    return "Review the requested local file changes. Existing backend checks "
           "and private-file protections still apply.";
  if (a.effect == Effect::read)
    return "Read the current information. Missing data will be shown as "
           "unavailable or unknown.";
  return "Choose your preference, inspect current state, and review the "
         "request before applying changes.";
}
static QWidget *details(const Action &a, const Plan *request = nullptr) {
  auto *widget = new QWidget;
  auto *layout = new QVBoxLayout(widget);
  layout->setContentsMargins(0, 0, 0, 0);
  auto *toggle = new QToolButton;
  toggle->setText("Details");
  toggle->setObjectName("detailsToggle");
  toggle->setCheckable(true);
  toggle->setArrowType(Qt::RightArrow);
  toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  toggle->setMinimumHeight(36);
  layout->addWidget(toggle, 0, Qt::AlignLeft);
  auto *content = new QWidget;
  auto *body = new QVBoxLayout(content);
  body->setContentsMargins(12, 4, 0, 8);
  body->addWidget(label(q(a.description.en)));
  body->addWidget(label("Action: " + q(a.id) +
                        "\nEffect: " + q(effect_name(a.effect)) +
                        "\nAccess: " + q(access_name(a.access))));
  for (const auto &p : a.parameters) {
    body->addWidget(label(q(p.title.en) + ": " + q(p.description.en)));
    for (const auto &c : p.choices)
      body->addWidget(label(q(c.title.en) + " — " + q(c.description.en)));
  }
  if (request) {
    auto *exact = new QPlainTextEdit(safe(display_command(request->argv)));
    exact->setReadOnly(true);
    exact->setObjectName("validatedCommand");
    exact->setMaximumHeight(100);
    body->addWidget(exact);
  }
  content->hide();
  layout->addWidget(content);
  QObject::connect(
      toggle, &QToolButton::toggled, content, [toggle, content](bool expanded) {
        content->setVisible(expanded);
        toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
      });
  return widget;
}
Observation::Observation(Fixture fixture, QWidget *parent)
    : QWidget(parent), fixture_(std::move(fixture)) {
  setObjectName("observation");
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  auto *line = new QHBoxLayout;
  state_ = label("Not read");
  state_->setObjectName("observationState");
  line->addWidget(state_, 1);
  refresh_ = button("Refresh read", "view-refresh");
  refresh_->setObjectName("refreshRead");
  refresh_->setEnabled(false);
  stop_ = button("Stop", "process-stop");
  stop_->setObjectName("stopRead");
  stop_->setEnabled(false);
  line->addWidget(refresh_);
  line->addWidget(stop_);
  layout->addLayout(line);
  stamp_ = label("No observation loaded.");
  layout->addWidget(stamp_);
  auto *tabs = new QTabWidget;
  tree_ = new QTreeWidget;
  tree_->setObjectName("observationTree");
  tree_->setHeaderLabels({"Observation", "Value / availability"});
  tree_->setAlternatingRowColors(true);
  tree_->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  tree_->header()->setStretchLastSection(true);
  tree_->setMinimumHeight(230);
  raw_ = new QPlainTextEdit;
  raw_->setObjectName("rawOutput");
  raw_->setReadOnly(true);
  error_ = new QPlainTextEdit;
  error_->setObjectName("errorOutput");
  error_->setReadOnly(true);
  tabs->addTab(tree_, "Values");
  tabs->addTab(raw_, "Raw output");
  tabs->addTab(error_, "Errors");
  layout->addWidget(tabs, 1);
  connect(refresh_, &QPushButton::clicked, this,
          [this] { start(id_.toStdString(), values_); });
  connect(stop_, &QPushButton::clicked, this, &Observation::cancel);
  connect(&poll_, &QTimer::timeout, this, [this] {
    if (!query_)
      return;
    query_->poll();
    showResult(query_->result());
    if (!query_->result().running) {
      poll_.stop();
      query_.reset();
    }
  });
  simulated_.setSingleShot(true);
  connect(&simulated_, &QTimer::timeout, this, [this] {
    auto r = fixture_.result(id_.toStdString());
    r.elapsed_ms = elapsed_.elapsed();
    showResult(r);
  });
}
Observation::~Observation() { cancel(); }
bool Observation::running() const { return result_.running; }
void Observation::start(const std::string &id, const Values &values) {
  cancel();
  id_ = q(id);
  values_ = values;
  tree_->clear();
  raw_->clear();
  error_->clear();
  elapsed_.start();
  try {
    const auto request = plan(id, values);
    if (request.effect != Effect::read ||
        request.presentation != Presentation::capture)
      throw std::runtime_error(
          "This observation requires a reviewed foreground terminal");
    Result r;
    r.action_id = id;
    r.running = true;
    showResult(r);
    if (fixture_.enabled)
      simulated_.start(fixture_.delay(id));
    else {
      query_ = std::make_unique<Query>(id, values);
      poll_.start(40);
    }
  } catch (const std::exception &e) {
    Result r;
    r.action_id = id;
    r.error = e.what();
    r.exit_status = 69;
    showResult(r);
  }
}
void Observation::cancel() {
  simulated_.stop();
  poll_.stop();
  if (query_) {
    query_->cancel();
    query_.reset();
  }
  if (result_.running) {
    auto r = result_;
    r.running = false;
    r.cancelled = true;
    r.elapsed_ms = elapsed_.isValid() ? elapsed_.elapsed() : 0;
    showResult(r);
  }
}
void Observation::addValue(QTreeWidgetItem *parent, const QString &key,
                           const QJsonValue &value, unsigned depth) {
  if (displayedValues_++ >= 2000)
    return;
  auto *item =
      parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(tree_);
  item->setText(0, key);
  if (depth >= 16 && (value.isObject() || value.isArray())) {
    item->setText(1, "Nested data; inspect Raw output");
    return;
  }
  if (value.isObject()) {
    const auto object = value.toObject();
    item->setText(1, object.isEmpty()
                         ? "Empty object"
                         : QString::number(object.size()) + " fields");
    for (auto it = object.begin();
         it != object.end() && displayedValues_ < 2000; ++it)
      addValue(item, safe(it.key().toStdString()), it.value(), depth + 1);
  } else if (value.isArray()) {
    const auto a = value.toArray();
    item->setText(1, QString::number(a.size()) + " entries");
    for (int i = 0; i < a.size() && displayedValues_ < 2000; ++i)
      addValue(item, QString::number(i), a.at(i), depth + 1);
  } else if (value.isNull() || value.isUndefined())
    item->setText(1, "Unknown (not reported)");
  else if (value.isBool())
    item->setText(1, value.toBool() ? "true" : "false");
  else if (value.isDouble())
    item->setText(1, QString::number(value.toDouble(), 'g', 16));
  else
    item->setText(1, safe(value.toString().toStdString()));
}
void Observation::showResult(const Result &r) {
  result_ = r;
  stop_->setEnabled(r.running);
  refresh_->setEnabled(!r.running && !id_.isEmpty());
  QString status =
      r.running     ? "Reading…"
      : r.cancelled ? "Cancelled · output may be partial"
      : r.timed_out ? "Timed out · output may be partial"
      : r.exit_status == 0
          ? "Read complete"
          : QString("Unavailable / failed · exit %1").arg(r.exit_status);
  if (r.truncated)
    status += " · output truncated";
  if (fixture_.enabled)
    status.prepend("SIMULATION · ");
  state_->setText(status);
  stamp_->setText(
      r.running ? "Read in progress. Stop or leave this view to cancel."
                : QString("Last read: %1")
                      .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
  raw_->setPlainText(safe(r.output));
  error_->setPlainText(r.error.empty() ? "No stderr reported." : safe(r.error));
  tree_->clear();
  displayedValues_ = 0;
  QJsonParseError err;
  const auto doc =
      QJsonDocument::fromJson(QByteArray::fromStdString(r.output), &err);
  if (err.error == QJsonParseError::NoError) {
    if (doc.isObject()) {
      auto obj = doc.object();
      if (!r.running && !r.cancelled && r.exit_status == 0 &&
          obj.contains("available")) {
        if (obj.value("available").isBool() && !obj.value("available").toBool())
          state_->setText(status + " · provider unavailable");
        else if (obj.value("available").isNull())
          state_->setText(status + " · availability unknown");
      }
      for (auto it = obj.begin(); it != obj.end(); ++it)
        addValue(nullptr, safe(it.key().toStdString()), it.value());
    } else
      addValue(nullptr, "Result", doc.array());
    tree_->expandToDepth(1);
  } else if (!r.output.empty()) {
    auto *i = new QTreeWidgetItem(tree_, {"Text result", safe(r.output)});
    i->setToolTip(1, safe(r.output));
  } else if (!r.running)
    new QTreeWidgetItem(tree_,
                        {"Availability", r.cancelled ? "Cancelled"
                                         : r.exit_status == 0
                                             ? "No output reported"
                                             : "Unavailable; inspect Errors"});
  if (displayedValues_ >= 2000)
    new QTreeWidgetItem(tree_, {"Display limit",
                                "First 2,000 values shown; inspect Raw output "
                                "for the full bounded result"});
}
struct Section {
  const char *title;
  const char *icon;
  const char *description;
};
static constexpr Section sections[] = {
    {"Status", "dialog-information",
     "See device information and current availability."},
    {"Power", "battery", "Battery, charging preferences, and wake settings."},
    {"Display & lights", "video-display",
     "Screen preferences, flashlight, and notification light."},
    {"Connections", "network-wired", "USB, Ethernet, and NFC connections."},
    {"Audio & sensors", "audio-card",
     "Audio, microphone, camera, and sensor information."},
    {"Updates", "system-software-update",
     "Review updates and choose your software channel."},
    {"Advanced", "preferences-system",
     "Additional diagnostics, recovery, and expert tools."}};
Window::Window(Fixture fixture, bool readOnly, QString styleNotice)
    : fixture_(std::move(fixture)), readOnly_(readOnly),
      styleNotice_(std::move(styleNotice)) {
  setWindowTitle(fixture_.enabled ? "Uke Control · SIMULATION" : "Uke Control");
  resize(1120, 800);
  setMinimumSize(780, 560);
  setWindowIcon(QIcon::fromTheme("org.uke.ControlQt",
                                 QIcon::fromTheme("preferences-system")));
  auto *central = new QWidget;
  auto *outer = new QVBoxLayout(central);
  outer->setContentsMargins(16, 16, 16, 12);
  outer->setSpacing(12);
  auto *top = new QHBoxLayout;
  auto *name = heading("Uke Control", 20);
  top->addWidget(name);
  top->addStretch();
  if (fixture_.enabled) {
    auto *banner = heading("SIMULATION — no commands or writes", 11);
    banner->setObjectName("simulationIndicator");
    top->addWidget(banner);
  } else
    top->addWidget(label(readOnly_ ? "Read-only session" : "Device settings"));
  outer->addLayout(top);
  auto *body = new QHBoxLayout;
  body->setSpacing(20);
  navigation_ = new QListWidget;
  navigation_->setObjectName("navigation");
  navigation_->setMinimumWidth(188);
  navigation_->setMaximumWidth(240);
  navigation_->setSpacing(4);
  navigation_->setIconSize(QSize(24, 24));
  for (const auto &s : sections) {
    auto *i =
        new QListWidgetItem(QIcon::fromTheme(s.icon), s.title, navigation_);
    i->setSizeHint(QSize(180, 48));
  }
  body->addWidget(navigation_);
  auto *right = new QVBoxLayout;
  pageTitle_ = heading("Status", 24);
  right->addWidget(pageTitle_);
  pageSubtitle_ = label("");
  right->addWidget(pageSubtitle_);
  stack_ = new QStackedWidget;
  right->addWidget(stack_, 1);
  body->addLayout(right, 1);
  outer->addLayout(body, 1);
  auto *foot =
      label(styleNotice_.isEmpty() ? "Changes require review before applying."
                                   : styleNotice_);
  foot->setObjectName("styleNotice");
  outer->addWidget(foot);
  setCentralWidget(central);
  connect(navigation_, &QListWidget::currentRowChanged, this,
          &Window::showSection);
  navigation_->setCurrentRow(0);
  auto *backAction = new QAction(this);
  backAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Left));
  addAction(backAction);
  connect(backAction, &QAction::triggered, this, &Window::back);
  auto *quit = new QAction(this);
  quit->setShortcut(QKeySequence::Quit);
  addAction(quit);
  connect(quit, &QAction::triggered, this, &Window::close);
}
Window::~Window() {
  if (observation_)
    observation_->cancel();
}
void Window::closeEvent(QCloseEvent *event) {
  if (observation_)
    observation_->cancel();
  QMainWindow::closeEvent(event);
}
void Window::back() { showSection(section_); }
void Window::addGroup(QVBoxLayout *layout, const QString &title,
                      const QString &subtitle,
                      const std::vector<std::string> &ids) {
  auto *box = new QGroupBox(title);
  auto f = box->font();
  f.setBold(true);
  box->setFont(f);
  auto *l = new QVBoxLayout(box);
  l->setContentsMargins(16, 18, 16, 14);
  l->setSpacing(8);
  l->addWidget(label(subtitle));
  for (const auto &id : ids) {
    const auto &a = action(id);
    auto *row = new QHBoxLayout;
    auto *titleLabel = label(friendlyTitle(a));
    auto normal = titleLabel->font();
    normal.setBold(false);
    titleLabel->setFont(normal);
    row->addWidget(titleLabel, 1);
    auto *b =
        button(a.effect == Effect::read ? "Inspect" : "Configure",
               a.effect == Effect::read ? "document-preview" : "configure");
    b->setObjectName(q(id));
    b->setMinimumWidth(116);
    row->addWidget(b);
    connect(b, &QPushButton::clicked, this, [this, id] { openAction(id); });
    l->addLayout(row);
  }
  layout->addWidget(box);
}
void Window::showSection(int index) {
  if (index < 0 || index > 6)
    return;
  section_ = index;
  observation_ = nullptr;
  fields_.clear();
  if (detail_) {
    stack_->removeWidget(detail_);
    delete detail_;
    detail_ = nullptr;
  }
  if (page_) {
    stack_->removeWidget(page_);
    delete page_;
  }
  pageTitle_->setText(sections[index].title);
  pageSubtitle_->setText(sections[index].description);
  auto *scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  page_ = scroll;
  auto *content = new QWidget;
  auto *l = new QVBoxLayout(content);
  l->setContentsMargins(0, 4, 8, 12);
  l->setSpacing(14);
  if (index == 0) {
    addGroup(l, "Device at a glance",
             "Choose the information you want to read.",
             {"status", "overview.battery", "overview.workload"});
    addGroup(l, "System information", "Hardware, desktop, and storage details.",
             {"hardware.status", "overview.desktop", "overview.storage"});
  }
  if (index == 1) {
    addGroup(l, "Battery & charging",
             "Measured values and saved requests are shown separately.",
             {"battery.status", "battery.health", "charge.status",
              "charge.admission"});
    addGroup(l, "Charging requests",
             "Check availability and preview your preference before applying.",
             {"charge.mode", "charge.limit", "charge.temporary-preview"});
    addGroup(l, "Sleep & protection",
             "Read modes and permissions without starting a sleep trial.",
             {"overview.sleep", "charge.protection-status", "charge.settings"});
  }
  if (index == 2) {
    addGroup(l, "Desktop",
             "Screen preferences for your current desktop session.",
             {"desktop.plasma", "desktop.rotation", "desktop.brightness"});
    addGroup(l, "Lights", "Flashlight and notification light preferences.",
             {"flashlight.status", "flashlight.state", "flashlight.level",
              "led.status", "led.color", "led.auto"});
    addGroup(l, "Display link",
             "Connected display information and preferences.",
             {"display.bits", "display.profile", "display.compression",
              "display.vrr-configured"});
  }
  if (index == 3) {
    addGroup(
        l, "USB & accessories",
        "Role changes can interrupt connected accessories or a remote session.",
        {"usb.status", "usb.role", "usb.power"});
    addGroup(l, "Network & NFC",
             "Read connection information and available preferences.",
             {"ethernet.status", "ethernet.profile", "overview.nfc"});
  }
  if (index == 4) {
    addGroup(l, "Audio & microphone",
             "Audio information and recording preferences.",
             {"overview.audio", "microphone.status", "microphone.select",
              "overview.camera"});
    addGroup(l, "Sensors",
             "Read sensor information once or for a short interval.",
             {"sensors.list", "sensors.once", "sensors.watch"});
  }
  if (index == 5) {
    addGroup(l, "Channel & update review",
             "Check available updates before making changes.",
             {"channel.status", "channel.preview", "update.preview"});
    addGroup(l, "Apply reviewed work",
             "Inspect unfinished updates before continuing. Wait for "
             "completion before rebooting.",
             {"channel.set", "channel.sync-preview", "channel.sync",
              "update.apply"});
  }
  if (index == 6) {
    auto *search = new QLineEdit;
    search->setObjectName("advancedSearch");
    search->setPlaceholderText(
        "Search all actions, descriptions or identifiers…");
    search->setMinimumHeight(40);
    l->addWidget(search);
    auto *list = new QListWidget;
    list->setObjectName("advancedActions");
    list->setMinimumHeight(420);
    for (const auto &a : uke::client::actions()) {
      auto *item = new QListWidgetItem(q(a.title.en) + "\n" + q(a.id) + " · " +
                                           q(effect_name(a.effect)),
                                       list);
      item->setData(Qt::UserRole, q(a.id));
      item->setData(Qt::UserRole + 1,
                    q(a.title.en + " " + a.description.en + " " + a.id));
      item->setSizeHint(QSize(0, 60));
    }
    l->addWidget(list);
    auto *open = button("Open selected action", "go-next");
    l->addWidget(open);
    connect(search, &QLineEdit::textChanged, this, [list](const QString &text) {
      for (int i = 0; i < list->count(); ++i) {
        auto *item = list->item(i);
        item->setHidden(!item->data(Qt::UserRole + 1)
                             .toString()
                             .contains(text, Qt::CaseInsensitive));
      }
    });
    connect(open, &QPushButton::clicked, this, [this, list] {
      if (auto *item = list->currentItem())
        openAction(item->data(Qt::UserRole).toString().toStdString());
    });
    connect(list, &QListWidget::itemActivated, this,
            [this](QListWidgetItem *item) {
              openAction(item->data(Qt::UserRole).toString().toStdString());
            });
  }
  l->addStretch();
  scroll->setWidget(content);
  stack_->addWidget(page_);
  stack_->setCurrentWidget(page_);
}
Values Window::values() const {
  Values result;
  for (const auto &[id, w] : fields_) {
    if (auto *combo = qobject_cast<QComboBox *>(w))
      result[id] = combo->currentData().toString().toStdString();
    else if (auto *spin = qobject_cast<QSpinBox *>(w))
      result[id] = std::to_string(spin->value());
    else if (auto *edit = qobject_cast<QLineEdit *>(w))
      result[id] = edit->text().toStdString();
  }
  return result;
}
void Window::openAction(const std::string &id) {
  const auto &a = action(id);
  actionId_ = id;
  fields_.clear();
  observation_ = nullptr;
  if (detail_) {
    stack_->removeWidget(detail_);
    delete detail_;
  }
  pageTitle_->setText(friendlyTitle(a));
  pageSubtitle_->setText(summary(a));
  auto *scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  detail_ = scroll;
  auto *content = new QWidget;
  auto *layout = new QVBoxLayout(content);
  layout->setContentsMargins(0, 4, 8, 12);
  layout->setSpacing(14);
  auto *backButton =
      button("Back to " + QString(sections[section_].title), "go-previous");
  backButton->setObjectName("backButton");
  backButton->setMaximumWidth(260);
  layout->addWidget(backButton);
  connect(backButton, &QPushButton::clicked, this, &Window::back);
  if (!a.parameters.empty()) {
    auto *group =
        new QGroupBox(a.effect == Effect::read ? "Options" : "Your preference");
    auto *form = new QFormLayout(group);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setSpacing(12);
    form->addRow(label("These are draft choices. Nothing changes until you "
                       "review and continue."));
    for (const auto &p : a.parameters) {
      QWidget *field = nullptr;
      if (p.kind == Kind::choice) {
        auto *combo = new QComboBox;
        for (const auto &c : p.choices) {
          combo->addItem(q(c.title.en), q(c.value));
          combo->setItemData(combo->count() - 1, q(c.description.en),
                             Qt::ToolTipRole);
        }
        combo->setCurrentIndex(std::max(0, combo->findData(q(p.initial))));
        field = combo;
      } else if (p.kind == Kind::integer && p.maximum <= INT_MAX) {
        auto *spin = new QSpinBox;
        spin->setRange(int(p.minimum), int(p.maximum));
        spin->setValue(q(p.initial).toInt());
        spin->setSuffix(p.id == "percent"   ? " %"
                        : p.id == "hz"      ? " Hz"
                        : p.id == "minutes" ? " min"
                                            : "");
        field = spin;
      } else {
        auto *edit = new QLineEdit(q(p.initial));
        edit->setMaxLength(4096);
        field = edit;
      }
      field->setMinimumHeight(38);
      field->setObjectName("parameter_" + q(p.id));
      field->setAccessibleName(q(p.title.en));
      field->setToolTip(q(p.description.en));
      fields_[p.id] = field;
      auto *fieldBox = new QWidget;
      fieldBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
      auto *fl = new QVBoxLayout(fieldBox);
      fl->setContentsMargins(0, 0, 0, 0);
      fl->addWidget(field);
      if (p.kind == Kind::integer)
        fl->addWidget(label(
            QString("Allowed range: %1–%2").arg(p.minimum).arg(p.maximum)));
      else if (p.kind == Kind::color)
        fl->addWidget(label("Enter #RRGGBB or choose a color."));
      else if (p.kind != Kind::choice)
        fl->addWidget(label(q(p.description.en)));
      if (auto *combo = qobject_cast<QComboBox *>(field);
          combo && p.id == "mode" && a.category == "charge") {
        auto choiceHelp = [combo] {
          const auto value = combo->currentData().toString();
          if (value == "firmware")
            return QString("Leave charging under firmware management.");
          if (value == "charge80")
            return QString(
                "Request a charge cap of 80%, without forced discharge.");
          if (value == "hold70-80")
            return QString("Request charging within a daily 70–80% band.");
          if (value == "hold80-90")
            return QString("Request charging within an 80–90% band.");
          if (value == "slow")
            return QString(
                "Use the charging provider's supported slow-charge limit.");
          return QString(
              "Use the charging provider's reviewed automatic limits.");
        };
        auto *explanation = label(choiceHelp());
        explanation->setObjectName("choiceExplanation_" + q(p.id));
        fl->addWidget(explanation);
        connect(
            combo, &QComboBox::currentIndexChanged, explanation,
            [choiceHelp, explanation] { explanation->setText(choiceHelp()); });
      }
      if (p.kind == Kind::color) {
        auto *edit = qobject_cast<QLineEdit *>(field);
        auto *choose = button("Choose color…", "color-management");
        fl->addWidget(choose);
        connect(choose, &QPushButton::clicked, this, [this, edit] {
          const auto color = QColorDialog::getColor(QColor(edit->text()), this,
                                                    "Requested LED color");
          if (color.isValid())
            edit->setText(color.name(QColor::HexRgb));
        });
      }
      form->addRow(q(p.title.en), fieldBox);
    }
    layout->addWidget(group);
  }
  auto *related = new QHBoxLayout;
  if (!a.inspect.empty()) {
    auto *b = button("Inspect current state", "document-preview");
    b->setObjectName("inspectCurrent");
    related->addWidget(b);
    connect(b, &QPushButton::clicked, this,
            [this, id = a.inspect] { inspect(id); });
  }
  if (!a.preview.empty()) {
    auto *b = button("Preview requested values", "document-preview");
    b->setObjectName("previewRequest");
    related->addWidget(b);
    connect(b, &QPushButton::clicked, this,
            [this, id = a.preview] { inspect(id); });
  }
  related->addStretch();
  layout->addLayout(related);
  layout->addWidget(details(a));
  primary_ =
      button(a.effect == Effect::read ? (a.presentation == Presentation::capture
                                             ? "Read observation"
                                             : "Review terminal observation")
                                      : "Review request",
             a.effect == Effect::read ? "view-refresh" : "dialog-ok-apply");
  primary_->setObjectName("primaryAction");
  primary_->setMinimumWidth(220);
  primary_->setMaximumWidth(340);
  layout->addWidget(primary_);
  connect(primary_, &QPushButton::clicked, this, &Window::submit);
  if (a.effect == Effect::read && a.presentation == Presentation::capture) {
    observation_ = new Observation(fixture_);
    layout->addWidget(observation_, 1);
  } else {
    layout->addWidget(label(
        fixture_.enabled
            ? "SIMULATION: every command, terminal launch and write is blocked."
        : readOnly_ && a.effect != Effect::read
            ? "Read-only session: applying changes is disabled."
            : "Review the request before continuing in a terminal."));
    layout->addStretch();
  }
  scroll->setWidget(content);
  stack_->addWidget(detail_);
  stack_->setCurrentWidget(detail_);
}
void Window::inspect(const std::string &id) {
  try {
    auto v = defaults(action(id));
    const auto draft = values();
    for (auto &[key, value] : v)
      if (draft.contains(key))
        value = draft.at(key);
    const auto request = plan(id, v);
    if (request.presentation == Presentation::terminal) {
      review(request);
      return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle(friendlyTitle(action(id)));
    dialog.resize(820, 580);
    auto *l = new QVBoxLayout(&dialog);
    l->addWidget(label(summary(action(id))));
    auto *observation = new Observation(fixture_);
    l->addWidget(observation, 1);
    l->addWidget(details(action(id)));
    auto *close = new QDialogButtonBox(QDialogButtonBox::Close);
    l->addWidget(close);
    connect(close, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    observation->start(id, v);
    dialog.exec();
    observation->cancel();
  } catch (const std::exception &e) {
    QMessageBox::warning(this, "Cannot inspect", safe(e.what()));
  }
}
void Window::submit() {
  try {
    const auto request = plan(actionId_, values());
    if (request.effect == Effect::read &&
        request.presentation == Presentation::capture) {
      observation_->start(actionId_, request.values);
      return;
    }
    review(request);
  } catch (const std::exception &e) {
    QMessageBox::warning(this, "Check requested values", safe(e.what()));
  }
}
void Window::review(const Plan &request) {
  QDialog dialog(this);
  dialog.setObjectName("requestReview");
  const auto &a = action(request.action_id);
  dialog.setWindowTitle("Review · " + friendlyTitle(a));
  dialog.resize(720, 570);
  auto *outer = new QVBoxLayout(&dialog);
  auto *scroll = new QScrollArea;
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  auto *content = new QWidget;
  auto *l = new QVBoxLayout(content);
  scroll->setWidget(content);
  outer->addWidget(scroll, 1);
  l->setSpacing(14);
  l->addWidget(heading("Review before continuing", 18));
  l->addWidget(heading(friendlyTitle(a), 14));
  l->addWidget(label(summary(a)));
  for (const auto &p : a.parameters) {
    QString value = safe(request.values.at(p.id));
    for (const auto &c : p.choices)
      if (c.value == request.values.at(p.id))
        value = q(c.title.en);
    l->addWidget(label(q(p.title.en) + ": " + value));
  }
  if (request.access == Access::root)
    l->addWidget(label(
        "Administrator authentication may be requested in the terminal."));
  if (request.effect == Effect::maintenance)
    l->addWidget(
        label("Maintenance can modify storage or prepare reboot. Preserve "
              "backups and the documented external recovery route. Existing "
              "plan-bound backend approvals remain required."));
  if (request.effect == Effect::publish)
    l->addWidget(
        label("This request publishes externally. Review the generated summary "
              "and the exact repository before continuing."));
  l->addWidget(details(a, &request));
  QString token = q(uke::frontend::required_confirmation(request));
  QLineEdit *confirmation = nullptr;
  if (!token.isEmpty()) {
    l->addWidget(label("Type " + token + " to confirm this reviewed request:"));
    confirmation = new QLineEdit;
    confirmation->setObjectName("confirmationToken");
    confirmation->setMinimumHeight(38);
    l->addWidget(confirmation);
  }
  auto *consent = new QCheckBox(
      request.effect == Effect::read
          ? "I reviewed this foreground observation and its access "
            "requirements."
          : "I reviewed the request, its effects and the current state.");
  consent->setObjectName("reviewConsent");
  l->addWidget(consent);
  const bool blocked =
      fixture_.enabled || (readOnly_ && request.effect != Effect::read);
  if (blocked)
    l->addWidget(label(fixture_.enabled
                           ? "SIMULATION: terminal launch and every real "
                             "command/write are blocked."
                           : "Read-only session: this change is blocked."));
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
  auto *run =
      buttons->addButton("Continue in terminal", QDialogButtonBox::AcceptRole);
  run->setObjectName("launchTerminal");
  run->setMinimumHeight(40);
  run->setEnabled(false);
  l->addStretch();
  outer->addWidget(buttons);
  auto update = [=] {
    run->setEnabled(!blocked && consent->isChecked() &&
                    (!confirmation || confirmation->text() == token));
  };
  connect(consent, &QCheckBox::toggled, &dialog, update);
  if (confirmation)
    connect(confirmation, &QLineEdit::textChanged, &dialog, update);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  connect(run, &QPushButton::clicked, &dialog, &QDialog::accept);
  if (dialog.exec() != QDialog::Accepted || blocked)
    return;
  // The shared terminal adapter revalidates the ID and values. No shell command
  // is executed by this GUI.
  const auto launched =
      uke::frontend::launch_terminal(request.action_id, request.values, true);
  if (!launched.launched)
    QMessageBox::warning(this, "Cannot open terminal", safe(launched.error));
  else
    QMessageBox::information(
        this, "Continue in terminal",
        launched.error.empty()
            ? "Continue in the terminal. Check the final result there, then "
              "refresh current state here."
            : safe(launched.error));
}
} // namespace uq
