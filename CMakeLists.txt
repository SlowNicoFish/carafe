cmake_minimum_required(VERSION 4.2)

project(carafe VERSION 0.1.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

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
include(ECMDeprecationSettings)
include(KDEGitCommitHooks)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    Concurrent
)

qt_standard_project_setup(REQUIRES ${QT_MIN_VERSION})
qt_policy(SET QTP0004 NEW)

# Forbid APIs deprecated in the minimum supported versions, so building
# against newer Qt/KF releases stays warning-free.
ecm_set_disabled_deprecation_versions(QT ${QT_MIN_VERSION} KF ${KF_MIN_VERSION})

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

include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.hpp *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
