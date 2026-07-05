import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import io.marlonn.carafe.backend

Kirigami.Dialog {
    id: dialog
    title: "Edit Game"
    padding: Kirigami.Units.largeSpacing

    property string gameId: ""
    property var protonBuilds: []

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
        umuField.text = game.umuId ?? "";
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
                id: umuField
                Kirigami.FormData.label: "UMU game ID:"
                placeholderText: "Optional"
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
                Backend.updateGame(dialog.gameId, {
                    title: titleField.text.trim(),
                    exePath: exeField.text.trim(),
                    launchArgs: launchArgsField.text,
                    prefixPath: prefixField.text.trim(),
                    protonVersion: protonCombo.currentIndex >= 0 ? protonCombo.currentText : "",
                    umuId: umuField.text.trim()
                });
                dialog.close();
            }
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
}
