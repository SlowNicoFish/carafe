import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: "Add Game"
    padding: Kirigami.Units.largeSpacing
    closePolicy: QQC2.Popup.CloseOnEscape
    width: Kirigami.Units.gridUnit * 35

    property var protonBuilds: []
    property bool prefixManuallyEdited: false
    property bool _fetchingArtwork: false
    property string _pendingFetchTitle: ""
    property bool _gridArtworkDone: false
    property bool _iconArtworkDone: false
    property string _previewGridPath: ""
    property string _previewIconPath: ""

    // Reset fields when the dialog opens
    // Except for the Proton version, which is set to the default if available
    onOpened: {
        Backend.reloadProtonBuilds();
        titleField.text = "";
        exeField.text = "";
        prefixField.text = "";
        prefixManuallyEdited = false;
        _fetchingArtwork = false;
        _pendingFetchTitle = "";
        _previewGridPath = "";
        _previewIconPath = "";
        const defaultProton = Backend.defaultProton ?? "";
        protonCombo.currentIndex = defaultProton !== "" ? protonCombo.find(defaultProton) : -1;
        installStatus.text = "";
        wrapperField.text = "";
        titleField.forceActiveFocus();
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

            RowLayout {
                Kirigami.FormData.label: "Game name:"
                spacing: Kirigami.Units.smallSpacing

                QQC2.TextField {
                    id: titleField
                    Layout.fillWidth: true
                    placeholderText: "My Game"
                    onTextChanged: {
                        validationMessage.visible = false;
                        if (!dialog.prefixManuallyEdited) {
                            prefixField.text = text.trim().length > 0 ? Backend.suggestPrefix(text.trim()) : "";
                        }
                    }
                }

                QQC2.Button {
                    id: fetchBtn
                    text: _fetchingArtwork ? "Fetching…" : "Fetch Artwork"
                    icon.name: _previewGridPath.length > 0 ? "view-refresh" : "image-x-generic"
                    enabled: titleField.text.trim().length > 0 && !_fetchingArtwork
                    onClicked: {
                        const title = titleField.text.trim();
                        dialog._pendingFetchTitle = title;
                        steamGridKeyDialog.openForRequest();
                    }
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Grid image:"
                spacing: Kirigami.Units.smallSpacing

                Image {
                    source: _previewGridPath.length > 0 ? Backend.localFileToUrl(_previewGridPath) : ""
                    visible: _previewGridPath.length > 0
                    fillMode: Image.PreserveAspectFit
                    Layout.preferredHeight: 100
                    Layout.preferredWidth: 160
                }

                QQC2.Button {
                    text: "Browse…"
                    icon.name: "image-x-generic"
                    onClicked: {
                        const path = Backend.chooseFile("Select grid image", ["Images (*.png *.jpg *.jpeg *.webp *.bmp *.gif)", "All Files (*)"]);
                        if (path.length === 0)
                            return;
                        const imported = Backend.importImage(path, "", "grid");
                        if (imported.length > 0)
                            dialog._previewGridPath = imported;
                    }
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Icon:"
                spacing: Kirigami.Units.smallSpacing

                Image {
                    source: _previewIconPath.length > 0 ? Backend.localFileToUrl(_previewIconPath) : ""
                    visible: _previewIconPath.length > 0
                    fillMode: Image.PreserveAspectFit
                    Layout.preferredHeight: 100
                    Layout.preferredWidth: 100
                }

                QQC2.Button {
                    text: "Browse…"
                    icon.name: "image-x-generic"
                    onClicked: {
                        const path = Backend.chooseFile("Select icon image", ["Images (*.png *.jpg *.jpeg *.webp *.bmp *.gif)", "All Files (*)"]);
                        if (path.length === 0)
                            return;
                        const imported = Backend.importImage(path, "", "icon");
                        if (imported.length > 0)
                            dialog._previewIconPath = imported;
                    }
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Executable:"
                spacing: Kirigami.Units.smallSpacing

                QQC2.TextField {
                    id: exeField
                    Layout.fillWidth: true
                    placeholderText: "/path/to/game.exe"
                    onTextChanged: validationMessage.visible = false
                }
                QQC2.Button {
                    text: "Browse…"
                    onClicked: {
                        const path = Backend.chooseFile("Select game executable", ["Executables (*.exe)", "All Files (*)"], prefixField.text);
                        if (path.length > 0)
                            exeField.text = path;
                    }
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Wine prefix:"
                spacing: Kirigami.Units.smallSpacing

                QQC2.TextField {
                    id: prefixField
                    Layout.fillWidth: true
                    placeholderText: "Auto-derived from game name"
                    onTextEdited: dialog.prefixManuallyEdited = true
                }
                QQC2.Button {
                    text: "Browse…"
                    onClicked: {
                        const path = Backend.chooseDirectory("Select Wine prefix directory", prefixField.text);
                        if (path.length > 0) {
                            prefixField.text = path;
                            dialog.prefixManuallyEdited = true;
                        }
                    }
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Installer:"
                spacing: Kirigami.Units.smallSpacing

                QQC2.Button {
                    id: installBtn
                    text: "Run Installer…"
                    icon.name: "media-playback-start"
                    enabled: titleField.text.trim().length > 0 && protonCombo.currentIndex >= 0
                    onClicked: {
                        const path = Backend.chooseFile("Select game installer", ["Installers (*.exe *.msi)", "All Files (*)"]);
                        if (path.length === 0)
                            return;
                        installStatus.text = "Running installer…";
                        installStatus.color = Kirigami.Theme.textColor;
                        installBtn.enabled = false;
                        Backend.runInstaller(path, prefixField.text, protonCombo.currentIndex >= 0 ? protonCombo.currentText : "");
                    }
                }

                QQC2.Label {
                    id: installStatus
                    visible: text.length > 0
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            QQC2.ComboBox {
                id: protonCombo
                Kirigami.FormData.label: "Proton version:"
                model: dialog.protonBuilds
                displayText: currentIndex < 0 ? "Choose Proton version" : currentText
            }

            QQC2.TextField {
                id: wrapperField
                Kirigami.FormData.label: "Launch wrapper:"
                placeholderText: "Optional, e.g. game-performance"
            }
        }
    }

    // Override Ok to validate first
    customFooterActions: [
        Kirigami.Action {
            text: "Add"
            icon.name: "list-add"
            onTriggered: {
                if (titleField.text.trim() === "" || exeField.text.trim() === "") {
                    validationMessage.text = "Game name and executable path are required.";
                    validationMessage.visible = true;
                    return;
                }
                if (Backend.addGame(titleField.text.trim(), exeField.text.trim(), prefixField.text.trim(), protonCombo.currentIndex >= 0 ? protonCombo.currentText : "", "", dialog._previewGridPath, dialog._previewIconPath, wrapperField.text.trim()))
                    dialog.close();
            }
        },
        Kirigami.Action {
            text: "Cancel"
            icon.name: "dialog-cancel"
            onTriggered: dialog.close()
        }
    ]

    SteamGridKeyDialog {
        id: steamGridKeyDialog
        onKeyAccepted: apiKey => {
            dialog._gridArtworkDone = false;
            dialog._iconArtworkDone = false;
            dialog._previewGridPath = "";
            dialog._previewIconPath = "";
            dialog._fetchingArtwork = true;
            Backend.fetchGridArtwork(dialog._pendingFetchTitle, apiKey);
            Backend.fetchIconArtwork(dialog._pendingFetchTitle, apiKey);
        }
    }

    Connections {
        target: Backend
        function onGridPreviewReady(gameName, path) {
            if (gameName === dialog._pendingFetchTitle) {
                dialog._gridArtworkDone = true;
                dialog._previewGridPath = path;
                dialog._fetchingArtwork = !dialog._gridArtworkDone || !dialog._iconArtworkDone;
            }
        }
        function onIconPreviewReady(gameName, path) {
            if (gameName === dialog._pendingFetchTitle) {
                dialog._iconArtworkDone = true;
                dialog._previewIconPath = path;
                dialog._fetchingArtwork = !dialog._gridArtworkDone || !dialog._iconArtworkDone;
            }
        }
        function onGridPreviewFailed(gameName, error) {
            if (gameName === dialog._pendingFetchTitle) {
                dialog._gridArtworkDone = true;
                dialog._fetchingArtwork = !dialog._gridArtworkDone || !dialog._iconArtworkDone;
                validationMessage.text = error;
                validationMessage.visible = true;
            }
        }
        function onIconPreviewFailed(gameName, error) {
            if (gameName === dialog._pendingFetchTitle) {
                dialog._iconArtworkDone = true;
                dialog._fetchingArtwork = !dialog._gridArtworkDone || !dialog._iconArtworkDone;
                validationMessage.text = error;
                validationMessage.visible = true;
            }
        }
        function onInstallerStarted() {
            installStatus.text = "Running installer…";
            installStatus.color = Kirigami.Theme.textColor;
            installBtn.enabled = false;
        }
        function onInstallerFinished(success, message) {
            installBtn.enabled = titleField.text.trim().length > 0 && protonCombo.currentIndex >= 0;
            installStatus.text = message;
            installStatus.color = success ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.negativeTextColor;
        }
    }
}
