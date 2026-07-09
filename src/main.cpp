#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>

#include "launcher.h"

int main(int argc, char *argv[]) {
  qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
  QGuiApplication app(argc, argv);

  app.setDesktopFileName(QStringLiteral(APP_ID));
  app.setWindowIcon(QIcon::fromTheme(QStringLiteral(APP_ID)));

  Launcher launcher;
  qmlRegisterSingletonInstance<Launcher>("io.marlonn.carafe.backend", 1, 0,
                                         "Backend", &launcher);

  QQmlApplicationEngine engine;
  engine.loadFromModule(QStringLiteral("io.marlonn.carafe"),
                        QStringLiteral("Main"));
  if (engine.rootObjects().isEmpty())
    return -1;

  launcher.loadLibrary();
  launcher.reloadProtonBuilds();

  return app.exec();
}
