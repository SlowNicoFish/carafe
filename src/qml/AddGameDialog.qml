import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import io.marlonn.carafe.backend

Kirigami.Dialog {
    id: dialog
    title: "Add Game"
    padding: Kirigami.Units.largeSpacing

    property var protonBuilds: []
    property bool prefixManuallyEdited: false
    property bool _fetchingArtwork: false
    property string _previewGridPath: ""
    property string _previewIconPath: ""

    // Reset fields when the dialog opens
    // Except for the Proton version, which is set to the default if available
    onOpened: {
        Backend.reloadProtonBuilds()
        titleField.text = ""
        exeField.text = ""
        prefixField.text = ""
        prefixManuallyEdited = false
        _fetchingArtwork = false
        _previewGridPath = ""
        _previewIconPath = ""
        const defaultProton = Backend.defaultProton ?? "";
        protonCombo.currentIndex = defaultProton !== ""
            ? protonCombo.find(defaultProton)
            : -1;
        installStatus.text = ""
        titleField.forceActiveFocus()
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
                        validationMessage.visible = false
                        if (!dialog.prefixManuallyEdited) {
                            prefixField.text = text.trim().length > 0
                                ? Backend.suggestPrefix(text.trim())
                                : ""
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
                        _previewGridPath = "";
                        _previewIconPath = "";
                        _fetchingArtwork = true;
                        Backend.fetchGridArtwork(title);
                        Backend.fetchIconArtwork(title);
                    }
                }
            }

            RowLayout {
                Kirigami.FormData.label: "Artwork:"
                visible: _previewGridPath.length > 0 || _previewIconPath.length > 0
                spacing: Kirigami.Units.largeSpacing

                Image {
                    source: _previewGridPath.length > 0 ? "file://" + _previewGridPath : ""
                    visible: _previewGridPath.length > 0
                    fillMode: Image.PreserveAspectFit
                    Layout.preferredHeight: 100
                    Layout.preferredWidth: 160
                }

                Image {
                    source: _previewIconPath.length > 0 ? "file://" + _previewIconPath : ""
                    visible: _previewIconPath.length > 0
                    fillMode: Image.PreserveAspectFit
                    Layout.preferredHeight: 100
                    Layout.preferredWidth: 160
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
                    onClicked: exeDialog.open()
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
                    onClicked: prefixDialog.open()
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
                    onClicked: installerFileDialog.open()
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
        }
    }

    // Override Ok to validate first
    customFooterActions: [
        Kirigami.Action {
            text: "Add"
            icon.name: "list-add"
            onTriggered: {
                if (titleField.text.trim() === "" || exeField.text.trim() === "") {
                    validationMessage.visible = true
                    return
                }
                Backend.addGame(titleField.text.trim(),
                                exeField.text.trim(),
                                prefixField.text.trim(),
                                protonCombo.currentIndex >= 0 ? protonCombo.currentText : "",
                                "",
                                dialog._previewGridPath,
                                dialog._previewIconPath)
                 dialog.close()
            }
        },
        Kirigami.Action {
            text: "Cancel"
            icon.name: "dialog-cancel"
            onTriggered: dialog.close()
        }
    ]

    function urlToPath(url) {
        return Backend.urlToLocalFile(url)
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
        onAccepted: {
            prefixField.text = dialog.urlToPath(selectedFolder)
            dialog.prefixManuallyEdited = true
        }
    }

    FileDialog {
        id: installerFileDialog
        title: "Select game installer"
        nameFilters: ["Installers (*.exe *.msi)", "All Files (*)"]
        onAccepted: {
            installStatus.text = "Running installer…"
            installBtn.enabled = false
            Backend.runInstaller(dialog.urlToPath(selectedFile),
                                 prefixField.text,
                                 protonCombo.currentIndex >= 0 ? protonCombo.currentText : "")
         }
    }

    Connections {
        target: Backend
        function onGridPreviewReady(gameName, path) {
            if (gameName === titleField.text.trim()) {
                dialog._previewGridPath = path;
                dialog._fetchingArtwork = false;
            }
        }
        function onIconPreviewReady(gameName, path) {
            if (gameName === titleField.text.trim()) {
                dialog._previewIconPath = path;
                dialog._fetchingArtwork = false;
            }
        }
        function onInstallerStarted() {
            installStatus.text = "Running installer…"
            installBtn.enabled = false
        }
        function onInstallerFinished(success, message) {
            installBtn.enabled = titleField.text.trim().length > 0 && protonCombo.currentIndex >= 0
            installStatus.text = message
            installStatus.color = success
                ? Kirigami.Theme.positiveTextColor
                : Kirigami.Theme.negativeTextColor
            if (success)
                exeDialog.currentFolder = "file://" + prefixField.text
        }
    }
}
