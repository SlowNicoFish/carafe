import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: dialog
    title: "SteamGridDB API Key"
    padding: Kirigami.Units.largeSpacing

    signal keyAccepted(string apiKey)

    onClosed: apiKeyField.clear()

    function openForRequest() {
        if (Backend.hasSavedApiKey()) {
            dialog.keyAccepted("");
            return;
        }
        apiKeyField.clear();
        dialog.open();
    }

    customFooterActions: [
        Kirigami.Action {
            text: "Continue"
            icon.name: "dialog-ok"
            enabled: apiKeyField.text.trim().length > 0
            onTriggered: {
                const key = apiKeyField.text.trim();
                apiKeyField.clear();
                dialog.close();
                dialog.keyAccepted(key);
            }
        },
        Kirigami.Action {
            text: "Cancel"
            icon.name: "dialog-cancel"
            onTriggered: dialog.close()
        }
    ]

    Kirigami.FormLayout {
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Information
            text: Backend.keyringAvailable() ? "No API key is saved in KWallet. Enter a key to use for this fetch, or save one in Settings." : "KWallet is unavailable. Enter an API key to use for this fetch."
            visible: true
        }

        QQC2.TextField {
            id: apiKeyField
            Kirigami.FormData.label: "API key:"
            placeholderText: "Enter SteamGridDB API key"
            echoMode: TextInput.Password
            passwordMaskDelay: 0
            onAccepted: {
                if (text.trim().length === 0)
                    return;
                const key = text.trim();
                clear();
                dialog.close();
                dialog.keyAccepted(key);
            }
        }
    }
}
