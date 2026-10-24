#include <QApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include <QQuickStyle>

#include "launcher.h"
using namespace Qt::Literals::StringLiterals;

int main(int argc, char *argv[]) {
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    QCoreApplication::setOrganizationName(QStringLiteral("Marlonn"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("marlonn.io"));
    QCoreApplication::setApplicationName(QStringLiteral(APP_ID));
    QApplication app(argc, argv);

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
