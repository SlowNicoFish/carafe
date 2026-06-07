qt_add_executable(carafe
    main.cpp
    game.h
    game.cpp
    gamemodel.h
    gamemodel.cpp
    proton.h
    proton.cpp
    settings.h
    settings.cpp
    storage.h
    storage.cpp
    launcher.h
    launcher.cpp
    steamgrid.h
    steamgrid.cpp
    icoextract.h
    icoextract.cpp
)

target_compile_definitions(carafe PRIVATE
    APP_ID="${APP_ID}"
    APP_VERSION="${PROJECT_VERSION}"
)

target_link_libraries(carafe PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Concurrent
    KF6::CoreAddons
    KF6::Kirigami
)

if(KF6::Wallet)
  target_link_libraries(carafe PRIVATE KF6::Wallet)
  target_compile_definitions(carafe PRIVATE HAVE_KWALLET)
endif()

qt_policy(SET QTP0004 NEW)

qt_add_qml_module(carafe
    NO_IMPORT_SCAN
    URI io.marlonn.carafe
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/Main.qml
        qml/GameGrid.qml
        qml/GameList.qml
        qml/GameCard.qml
        qml/GameListItem.qml
        qml/AddGameDialog.qml
        qml/EditGameDialog.qml
        qml/SettingsDialog.qml
        qml/ContextMenu.qml
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS carafe ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
