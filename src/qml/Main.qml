import QtQuick
import QtCore
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs

Kirigami.ApplicationWindow {
    id: window
    visible: true
    width: 1200
    height: 820
    minimumWidth: 640
    minimumHeight: 480
    title: "Carafe"

    Component.onCompleted: Backend.loadLibrary()

    Settings {
        id: uiSettings
        category: "ui"
        property string viewMode: "grid"
    }

    property string pendingArtworkGameId: ""

    function fetchArtwork(gameId) {
        pendingArtworkGameId = gameId;
        steamGridKeyDialog.openForRequest();
    }

    pageStack.initialPage: Kirigami.Page {
        title: "Carafe"
        padding: Kirigami.Units.largeSpacing

        actions: [
            Kirigami.Action {
                icon.name: "list-add"
                text: "Add Game"
                onTriggered: addGameDialog.open()
            },
            Kirigami.Action {
                icon.name: uiSettings.viewMode === "grid" ? "view-list-details" : "view-grid"
                text: uiSettings.viewMode === "grid" ? "List View" : "Grid View"
                onTriggered: uiSettings.viewMode = uiSettings.viewMode === "grid" ? "list" : "grid"
            },
            Kirigami.Action {
                icon.name: "settings-configure"
                text: "Settings"
                onTriggered: settingsDialog.open()
            }
        ]

        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.largeSpacing

            Loader {
                Layout.fillWidth: true
                Layout.fillHeight: true
                sourceComponent: uiSettings.viewMode === "grid" ? gridPage : listPage
            }
        }

        Component {
            id: gridPage
            GameGrid {
                onEditRequested: gameId => editGameDialog.openGame(gameId)
                onDeleteRequested: gameId => {
                    removeGameDialog.gameId = gameId;
                    removeGameDialog.open();
                }
                onLaunchRequested: gameId => Backend.launchGame(gameId)
                onStopRequested: gameId => Backend.stopGame(gameId)
                onFetchArtworkRequested: gameId => window.fetchArtwork(gameId)
                onRunExeInPrefixRequested: gameId => {
                    const info = Backend.gameById(gameId);
                    const initialDir = info && info.prefixPath ? info.prefixPath : "";
                    const path = Backend.chooseFile("Select executable to run in prefix", ["Executables (*.exe *.msi)", "All Files (*)"], initialDir);
                    if (path.length > 0)
                        Backend.runExeInPrefix(gameId, path);
                }
            }
        }

        Component {
            id: listPage
            GameList {
                onEditRequested: gameId => editGameDialog.openGame(gameId)
                onDeleteRequested: gameId => {
                    removeGameDialog.gameId = gameId;
                    removeGameDialog.open();
                }
                onLaunchRequested: gameId => Backend.launchGame(gameId)
                onStopRequested: gameId => Backend.stopGame(gameId)
                onFetchArtworkRequested: gameId => window.fetchArtwork(gameId)
                onRunExeInPrefixRequested: gameId => {
                    const info = Backend.gameById(gameId);
                    const initialDir = info && info.prefixPath ? info.prefixPath : "";
                    const path = Backend.chooseFile("Select executable to run in prefix", ["Executables (*.exe *.msi)", "All Files (*)"], initialDir);
                    if (path.length > 0)
                        Backend.runExeInPrefix(gameId, path);
                }
            }
        }
    }

    AddGameDialog {
        id: addGameDialog
        protonBuilds: Backend.protonBuilds
    }

    EditGameDialog {
        id: editGameDialog
        protonBuilds: Backend.protonBuilds
    }

    SettingsDialog {
        id: settingsDialog
    }

    SteamGridKeyDialog {
        id: steamGridKeyDialog
        onKeyAccepted: apiKey => {
            Backend.fetchGameArtwork(window.pendingArtworkGameId, apiKey);
            toastBar.showMessage("Fetching artwork…", false);
        }
    }

    Kirigami.Dialog {
        id: removeGameDialog
        title: "Remove Game?"
        padding: Kirigami.Units.largeSpacing

        property string gameId: ""

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                text: {
                    const info = Backend.gameById(removeGameDialog.gameId);
                    return info && info.title ? `Remove "${info.title}" from the library?` : "";
                }
                wrapMode: Text.WordWrap
            }
            QQC2.Label {
                text: {
                    const info = Backend.gameById(removeGameDialog.gameId);
                    return info && info.prefixPath ? "Also delete the Wine prefix folder:\n" + info.prefixPath : "";
                }
                wrapMode: Text.WordWrap
                color: Kirigami.Theme.disabledTextColor
            }
        }

        customFooterActions: [
            Kirigami.Action {
                text: "Cancel"
                icon.name: "dialog-cancel"
                onTriggered: removeGameDialog.close()
            },
            Kirigami.Action {
                text: "Remove Only"
                icon.name: "list-remove"
                onTriggered: {
                    if (Backend.removeGame(removeGameDialog.gameId, false))
                        removeGameDialog.close();
                }
            },
            Kirigami.Action {
                text: "Remove & Delete Prefix"
                icon.name: "edit-delete"
                onTriggered: {
                    if (Backend.removeGame(removeGameDialog.gameId, true))
                        removeGameDialog.close();
                }
            }
        ]
    }

    KirigamiDialogs.PromptDialog {
        id: launchErrorDialog
        title: "Launch Failed"
        subtitle: ""
        standardButtons: Kirigami.Dialog.Ok
    }

    Connections {
        target: Backend
        function onGameLaunchFailed(gameId, reason) {
            launchErrorDialog.subtitle = reason;
            launchErrorDialog.open();
        }
        function onToastMessage(message) {
            toastBar.showMessage(message, false);
        }
        function onRunExeInPrefixFinished(success, message) {
            toastBar.showMessage(message, !success);
        }
    }

    Kirigami.InlineMessage {
        id: toastBar
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            bottomMargin: Kirigami.Units.largeSpacing * 2
        }
        width: parent.width - Kirigami.Units.largeSpacing * 4
        z: 999
        showCloseButton: true
        visible: false

        property bool _isError: false
        property var _queue: []
        property bool _active: false

        type: _isError ? Kirigami.MessageType.Error : Kirigami.MessageType.Information

        function showMessage(msg, isError) {
            _queue.push({
                msg: msg,
                isError: isError
            });
            if (!_active)
                _showNext();
        }

        function _showNext() {
            if (_queue.length === 0) {
                _active = false;
                return;
            }
            _active = true;
            var item = _queue.shift();
            text = item.msg;
            _isError = item.isError;
            visible = true;
            hideTimer.interval = _isError ? 8000 : 4000;
            hideTimer.restart();
        }

        Timer {
            id: hideTimer
            interval: 4000
            onTriggered: {
                toastBar.visible = false;
                toastBar._showNext();
            }
        }
    }
}
