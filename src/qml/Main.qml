import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs
import io.marlonn.carafe.backend

Kirigami.ApplicationWindow {
    id: window
    visible: true
    width: 1200
    height: 820
    title: "Carafe"
    property string viewMode: "grid"

    function fetchArtwork(gameId) {
        const apiKey = Backend.settings.steamgridApiKey;
        if (!apiKey || apiKey.trim() === "") {
            toastBar.showMessage("Set a SteamGridDB API key in Settings first.", true);
            return;
        }
        Backend.fetchGrid(gameId, apiKey);
        Backend.fetchIcon(gameId, apiKey);
        toastBar.showMessage("Fetching artwork…", false);
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
                icon.name: viewMode === "grid" ? "view-list-details" : "view-grid"
                text: viewMode === "grid" ? "List View" : "Grid View"
                onTriggered: viewMode = viewMode === "grid" ? "list" : "grid"
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
                sourceComponent: viewMode === "grid" ? gridPage : listPage
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
                onFetchArtworkRequested: gameId => window.fetchArtwork(gameId)
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
                onFetchArtworkRequested: gameId => window.fetchArtwork(gameId)
            }
        }
    }

    // ── Dialogs ───────────────────────────────────────────────────────────

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
        settings: Backend.settings
        onSettingsSaved: settings => Backend.saveSettings(settings)
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
                onTriggered: {
                    Backend.removeGame(removeGameDialog.gameId, false);
                    removeGameDialog.close();
                }
            },
            Kirigami.Action {
                text: "Remove & Delete Prefix"
                icon.name: "edit-delete"
                onTriggered: {
                    Backend.removeGame(removeGameDialog.gameId, true);
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
    }

    // ── Toast notification bar ────────────────────────────────────────────
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

        type: _isError ? Kirigami.MessageType.Error : Kirigami.MessageType.Information

        function showMessage(msg, isError) {
            text = msg;
            _isError = isError;
            visible = true;
            hideTimer.restart();
        }

        Timer {
            id: hideTimer
            interval: 4000
            onTriggered: toastBar.visible = false
        }
    }
}
