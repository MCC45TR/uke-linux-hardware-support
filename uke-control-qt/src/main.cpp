// SPDX-License-Identifier: MIT
#include "ui.hpp"
#include <unistd.h>
int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QCoreApplication::setApplicationName("uke-control-qt");
  QCoreApplication::setApplicationVersion("1.0.0");
  QCoreApplication::setOrganizationName("Uke");
  QCommandLineParser parser;
  parser.setApplicationDescription(
      "Native Qt Widgets interface to the Uke Control client. Run as a normal "
      "session user.");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addOption(
      {"fixture",
       "Run without any commands, using a bounded simulation fixture.",
       "file"});
  parser.addOption({"read-only", "Disable mutations and local file creation."});
  parser.addOption({"start", "Open a known action guide without executing it.",
                    "action-id"});
  parser.process(app);
  QString notice;
  const auto styles = QStyleFactory::keys();
  bool breeze = false;
  for (const auto &style : styles)
    if (style.compare("Breeze", Qt::CaseInsensitive) == 0) {
      app.setStyle(QStyleFactory::create(style));
      breeze = true;
      break;
    }
  if (!breeze)
    notice = "Breeze Qt style is unavailable; using the installed native " +
             app.style()->objectName() +
             " style. Install plasma-breeze-qt6 for Breeze.";
  try {
    uq::Fixture fixture;
    if (parser.isSet("fixture"))
      fixture = uq::Fixture::load(parser.value("fixture"));
    if (geteuid() == 0 && !fixture.enabled) {
      qCritical("Run Uke Control as a normal graphical session user; root GUI "
                "is refused.");
      return 77;
    }
    uq::Window window(fixture, parser.isSet("read-only"), notice);
    if (parser.isSet("start"))
      window.openAction(parser.value("start").toStdString());
    window.show();
    return app.exec();
  } catch (const std::exception &e) {
    qCritical("%s", e.what());
    return 64;
  }
}
