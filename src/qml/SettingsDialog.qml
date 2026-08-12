import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import io.marlonn.carafe.backend

Kirigami.Dialog {
    id: dialog
    title: "Settings"
    padding: Kirigami.Units.largeSpacing

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
            echoMode: TextInput.Password
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
