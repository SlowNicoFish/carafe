import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import io.marlonn.carafe.backend

Kirigami.Dialog {
    id: dialog
    title: "Settings"
    padding: Kirigami.Units.largeSpacing

    property var settings: ({})
    signal settingsSaved(var settings)

    onOpened: {
        const s = Backend.settings;
        defaultProtonCombo.currentIndex = defaultProtonCombo.find(s.defaultProton ?? "");
        apiKeyField.text = s.steamgridApiKey ?? "";
        defaultArgsField.text = s.defaultLaunchArgs ?? "";
        extraProtonField.text = s.extraProtonPaths ?? "";
    }

    customFooterActions: [
        Kirigami.Action {
            text: "Save"
            icon.name: "document-save"
            onTriggered: {
                settingsSaved({
                    defaultProton: defaultProtonCombo.currentIndex >= 0 ? defaultProtonCombo.currentText : "",
                    steamgridApiKey: apiKeyField.text,
                    defaultLaunchArgs: defaultArgsField.text,
                    extraProtonPaths: extraProtonField.text
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

        QQC2.TextArea {
            id: extraProtonField
            Kirigami.FormData.label: "Extra Proton search paths:"
            placeholderText: "One path per line"
            Layout.fillWidth: true
            implicitHeight: Kirigami.Units.gridUnit * 5
            wrapMode: TextEdit.NoWrap
        }
    }
}
