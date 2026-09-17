#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>

#include "launcher.h"
using namespace Qt::Literals::StringLiterals;

int main(int argc, char *argv[]) {
    qputenv("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
    QCoreApplication::setOrganizationName(QString());
    QCoreApplication::setApplicationName(QStringLiteral(APP_ID));
    QGuiApplication app(argc, argv);

    app.setDesktopFileName(QStringLiteral(APP_ID));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral(APP_ID)));

    auto &launcher = Launcher::instance();
    launcher.reloadProtonBuilds();

    QQmlApplicationEngine engine;
    engine.loadFromModule(u"io.marlonn.carafe"_s, u"Main"_s);
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
