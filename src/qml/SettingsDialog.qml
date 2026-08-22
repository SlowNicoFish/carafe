import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import io.marlonn.carafe.backend

Kirigami.Dialog {
    id: dialog
    title: "Settings"
    padding: Kirigami.Units.largeSpacing

    property bool revealApiKey: false

    onOpened: {
        defaultProtonCombo.currentIndex = defaultProtonCombo.find(Backend.defaultProton ?? "");
        apiKeyField.text = Backend.steamgridApiKey ?? "";
        defaultArgsField.text = Backend.defaultLaunchArgs ?? "";
        defaultWrapperField.text = Backend.defaultWrapperCommand ?? "";
    }

    customFooterActions: [
        Kirigami.Action {
            text: "Save"
            icon.name: "document-save"
            onTriggered: {
                Backend.saveSettings({
                    defaultProton: defaultProtonCombo.currentIndex >= 0 ? defaultProtonCombo.currentText : "",
                    steamgridApiKey: apiKeyField.text,
                    defaultLaunchArgs: defaultArgsField.text,
                    defaultWrapperCommand: defaultWrapperField.text
                });
                dialog.close();
            }
        },
        Kirigami.Action {
            text: "Cancel"
            icon.name: "dialog-cancel"
            onTriggered: dialog.close()
        }
    ]

    Kirigami.FormLayout {
        QQC2.ComboBox {
            id: defaultProtonCombo
            Kirigami.FormData.label: "Default Proton version:"
            model: Backend.protonBuilds
            displayText: currentIndex < 0 ? "None" : currentText
        }

        QQC2.TextField {
            id: apiKeyField
            Kirigami.FormData.label: "SteamGridDB API key:"
            placeholderText: "Paste your API key here"
            echoMode: dialog.revealApiKey ? TextInput.Normal : TextInput.Password
            rightPadding: revealButton.width + Kirigami.Units.smallSpacing

            QQC2.ToolButton {
                id: revealButton
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                icon.name: dialog.revealApiKey ? "password-show-off" : "password-show-on"
                onClicked: dialog.revealApiKey = !dialog.revealApiKey
                QQC2.ToolTip.text: dialog.revealApiKey ? "Hide API key" : "Show API key"
                QQC2.ToolTip.visible: hovered
                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
            }
        }

        QQC2.TextField {
            id: defaultArgsField
            Kirigami.FormData.label: "Default launch arguments:"
            placeholderText: "Optional"
        }

        QQC2.TextField {
            id: defaultWrapperField
            Kirigami.FormData.label: "Default launch wrapper:"
            placeholderText: "Optional, e.g. game-performance"
        }
    }
}
