cmake_minimum_required(VERSION 4.2)

project(carafe VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_MIN_VERSION "6.6.0")
set(KF_MIN_VERSION "6.0.0")

set(APP_ID          "io.marlonn.carafe")
set(APP_DESCRIPTION "A game launcher for Linux using Proton to run Windows games.")
set(APP_LICENSE     "MIT")
set(APP_HOMEPAGE    "https://github.com/marlonn/carafe")
set(APP_BUG_ADDRESS "https://github.com/marlonn/carafe/issues")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMFindQmlModule)
include(ECMQmlModule)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    Concurrent
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Kirigami
    CoreAddons
)

find_package(KF6 ${KF_MIN_VERSION} QUIET COMPONENTS Wallet)

ecm_find_qmlmodule(org.kde.kirigami REQUIRED)

add_subdirectory(src)

install(FILES res/io.marlonn.carafe.desktop      DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES res/io.marlonn.carafe.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES res/io.marlonn.carafe.svg
    DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
