import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: "Settings"
    padding: Kirigami.Units.largeSpacing
    property bool savedApiKeyAvailable: false
    property string savedKeyMask: "saved-key-mask"

    onClosed: apiKeyField.clear()

    onOpened: {
        Backend.reloadProtonBuilds();
        defaultProtonCombo.currentIndex = defaultProtonCombo.find(Backend.defaultProton ?? "");
        savedApiKeyAvailable = Backend.hasSavedApiKey();
        apiKeyField.text = savedApiKeyAvailable ? savedKeyMask : "";
        defaultArgsField.text = Backend.defaultLaunchArgs ?? "";
        defaultWrapperField.text = Backend.defaultWrapperCommand ?? "";
    }

    customFooterActions: [
        Kirigami.Action {
            text: "Save"
            icon.name: "document-save"
            onTriggered: {
                if (Backend.saveSettings({
                    defaultProton: defaultProtonCombo.currentIndex >= 0 ? defaultProtonCombo.currentText : (Backend.defaultProton ?? ""),
                    steamgridApiKey: apiKeyField.text === dialog.savedKeyMask ? "" : apiKeyField.text,
                    removeSteamgridApiKey: dialog.savedApiKeyAvailable && apiKeyField.text.length === 0,
                    defaultLaunchArgs: defaultArgsField.text,
                    defaultWrapperCommand: defaultWrapperField.text
                })) {
                    apiKeyField.clear();
                    dialog.close();
                }
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

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Information
            text: "KWallet is unavailable. Saved API keys require KWallet. Enter a one-time key when fetching artwork."
            visible: !Backend.keyringAvailable()
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Information
            text: dialog.savedApiKeyAvailable ? "API key saved in KWallet. Leave the dots unchanged to keep it, or clear the field to remove it." : "No API key is saved. Enter one to save it in KWallet."
            visible: Backend.keyringAvailable()
        }

        QQC2.TextField {
            id: apiKeyField
            Kirigami.FormData.label: "SteamGridDB API key:"
            placeholderText: dialog.savedApiKeyAvailable ? "Leave unchanged to keep, clear to remove" : "Enter a key to save"
            echoMode: TextInput.Password
            passwordMaskDelay: 0
            visible: Backend.keyringAvailable()
            onActiveFocusChanged: {
                if (activeFocus && text === dialog.savedKeyMask)
                    selectAll();
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
