import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: "Edit Game"
    padding: Kirigami.Units.largeSpacing

    property string gameId: ""
    property var protonBuilds: []
    property bool _fetchingArtwork: false
    property string _pendingFetchTitle: ""
    property string _gridPath: ""
    property string _iconPath: ""

    function openGame(id) {
        Backend.reloadProtonBuilds();
        gameId = id;
        const game = Backend.gameById(id);
        if (!game || Object.keys(game).length === 0)
            return;
        titleField.text = game.title ?? "";
        exeField.text = game.exePath ?? "";
        prefixField.text = game.prefixPath ?? "";
        launchArgsField.text = game.launchArgs ?? "";
        wrapperField.text = game.wrapperCommand ?? "";
        umuField.text = game.umuId ?? "";
        _gridPath = game.gridPath ?? "";
        _iconPath = game.steamgridIconPath ?? "";
        _fetchingArtwork = false;
        const idx = protonCombo.find(game.protonVersion ?? "");
        protonCombo.currentIndex = idx;
        dialog.open();
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            id: validationMessage
            Layout.fillWidth: true
            type: Kirigami.MessageType.Warning
            text: "Game name and executable path are required."
            visible: false
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            QQC2.TextField {
                id: titleField
                Kirigami.FormData.label: "Game name:"
                onTextChanged: validationMessage.visible = false
            }

            RowLayout {
                Kirigami.FormData.label: "Executable:"
                spacing: Kirigami.Units.smallSpacing

                QQC2.TextField {
                    id: exeField
                    Layout.fillWidth: true
                    onTextChanged: validationMessage.visible = false
                }
                QQC2.Button {
                    text: "Browse…"
                    onClicked: exeDialog.open()
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Wine prefix:"
                spacing: Kirigami.Units.smallSpacing

                QQC2.TextField {
                    id: prefixField
                    Layout.fillWidth: true
                }
                QQC2.Button {
                    text: "Browse…"
                    onClicked: prefixDialog.open()
                }
            }

            QQC2.ComboBox {
                id: protonCombo
                Kirigami.FormData.label: "Proton version:"
                model: dialog.protonBuilds
                displayText: currentIndex < 0 ? "Choose Proton version" : currentText
            }

            QQC2.TextField {
                id: launchArgsField
                Kirigami.FormData.label: "Launch arguments:"
                placeholderText: "Optional"
            }

            QQC2.TextField {
                id: wrapperField
                Kirigami.FormData.label: "Launch wrapper:"
                placeholderText: "Optional, e.g. game-performance"
            }

            QQC2.TextField {
                id: umuField
                Kirigami.FormData.label: "UMU game ID:"
                placeholderText: "Optional"
            }

            RowLayout {
                Kirigami.FormData.label: "Grid image:"
                spacing: Kirigami.Units.smallSpacing

                Image {
                    source: _gridPath.length > 0 ? Backend.localFileToUrl(_gridPath) : ""
                    visible: _gridPath.length > 0
                    fillMode: Image.PreserveAspectFit
                    Layout.preferredHeight: 100
                    Layout.preferredWidth: 160
                }

                QQC2.Button {
                    text: "Browse…"
                    icon.name: "image-x-generic"
                    onClicked: gridImageDialog.open()
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Icon:"
                spacing: Kirigami.Units.smallSpacing

                Image {
                    source: _iconPath.length > 0 ? Backend.localFileToUrl(_iconPath) : ""
                    visible: _iconPath.length > 0
                    fillMode: Image.PreserveAspectFit
                    Layout.preferredHeight: 100
                    Layout.preferredWidth: 100
                }

                QQC2.Button {
                    text: "Browse…"
                    icon.name: "image-x-generic"
                    onClicked: iconImageDialog.open()
                }
            }

            QQC2.Button {
                Kirigami.FormData.label: "Artwork:"
                text: _fetchingArtwork ? "Fetching…" : "Fetch from SteamGridDB"
                icon.name: _fetchingArtwork ? "view-refresh" : "download"
                enabled: titleField.text.trim().length > 0 && !_fetchingArtwork
                onClicked: {
                    const title = titleField.text.trim();
                    dialog._pendingFetchTitle = title;
                    _fetchingArtwork = true;
                    Backend.fetchGridArtwork(title);
                    Backend.fetchIconArtwork(title);
                }
            }
        }
    }

    customFooterActions: [
        Kirigami.Action {
            text: "Save"
            icon.name: "document-save"
            onTriggered: {
                if (titleField.text.trim() === "" || exeField.text.trim() === "") {
                    validationMessage.visible = true;
                    return;
                }
                if (Backend.updateGame(dialog.gameId, {
                    title: titleField.text.trim(),
                    exePath: exeField.text.trim(),
                    launchArgs: launchArgsField.text,
                    wrapperCommand: wrapperField.text,
                    prefixPath: prefixField.text.trim(),
                    protonVersion: protonCombo.currentIndex >= 0 ? protonCombo.currentText : "",
                    umuId: umuField.text.trim(),
                    gridPath: dialog._gridPath,
                    steamgridIconPath: dialog._iconPath
                }))
                    dialog.close();
            }
        },
        Kirigami.Action {
            text: "Cancel"
            icon.name: "dialog-cancel"
            onTriggered: dialog.close()
        }
    ]

    function urlToPath(url) {
        return Backend.urlToLocalFile(url);
    }

    FileDialog {
        id: exeDialog
        title: "Select game executable"
        nameFilters: ["Executables (*.exe)", "All Files (*)"]
        onAccepted: exeField.text = dialog.urlToPath(selectedFile)
    }

    FolderDialog {
        id: prefixDialog
        title: "Select Wine prefix directory"
        onAccepted: prefixField.text = dialog.urlToPath(selectedFolder)
    }

    FileDialog {
        id: gridImageDialog
        title: "Select grid image"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp *.gif)", "All Files (*)"]
        onAccepted: {
            const path = dialog.urlToPath(selectedFile);
            const imported = Backend.importImage(path, dialog.gameId, "grid");
            if (imported.length > 0)
                dialog._gridPath = imported;
        }
    }

    FileDialog {
        id: iconImageDialog
        title: "Select icon image"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp *.gif)", "All Files (*)"]
        onAccepted: {
            const path = dialog.urlToPath(selectedFile);
            const imported = Backend.importImage(path, dialog.gameId, "icon");
            if (imported.length > 0)
                dialog._iconPath = imported;
        }
    }

    Connections {
        target: Backend
        function onGridPreviewReady(gameName, path) {
            dialog._fetchingArtwork = false;
            if (gameName === dialog._pendingFetchTitle)
                dialog._gridPath = path;
        }
        function onIconPreviewReady(gameName, path) {
            dialog._fetchingArtwork = false;
            if (gameName === dialog._pendingFetchTitle)
                dialog._iconPath = path;
        }
        function onGridPreviewFailed(gameName, error) {
            dialog._fetchingArtwork = false;
            if (gameName === dialog._pendingFetchTitle) {
                validationMessage.text = error;
                validationMessage.visible = true;
            }
        }
        function onIconPreviewFailed(gameName, error) {
            dialog._fetchingArtwork = false;
            if (gameName === dialog._pendingFetchTitle) {
                validationMessage.text = error;
                validationMessage.visible = true;
            }
        }
    }
}
