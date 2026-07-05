import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: cardRoot

    property string gameId: ""
    property string title: ""
    property string iconSource: ""
    property string launchArgs: ""
    property string prefixPath: ""
    property string protonVersion: ""
    property string umuId: ""
    property bool isRunning: false

    signal launchRequested
    signal editRequested
    signal deleteRequested
    signal fetchArtworkRequested
    signal runExeInPrefixRequested

    ContextMenu {
        id: contextMenu
        gameId: cardRoot.gameId
        onLaunchRequested: cardRoot.launchRequested()
        onEditRequested: cardRoot.editRequested()
        onDeleteRequested: cardRoot.deleteRequested()
        onFetchArtworkRequested: cardRoot.fetchArtworkRequested()
        onRunExeRequested: cardRoot.runExeInPrefixRequested()
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: contextMenu.popup()
    }

    Rectangle {
        anchors.fill: parent
        radius: Kirigami.Units.cornerRadius
        color: Kirigami.Theme.backgroundColor
        border.width: 1
        border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.15)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            Image {
                source: cardRoot.iconSource !== "" ? cardRoot.iconSource : "qrc:/res/icons/hicolor/scalable/apps/io.marlonn.carafe.svg"
                fillMode: cardRoot.iconSource !== "" ? Image.PreserveAspectCrop : Image.PreserveAspectFit
                clip: true
                Layout.fillWidth: true
                Layout.preferredHeight: Math.round(cardRoot.height * 0.85)
                sourceSize.width: 460
                sourceSize.height: 460
            }

            Item {
                Layout.fillHeight: true
            }

            RowLayout {
                spacing: Kirigami.Units.smallSpacing
                Layout.fillWidth: true

                QQC2.Button {
                    text: cardRoot.isRunning ? "Running" : "Launch"
                    enabled: !cardRoot.isRunning
                    icon.name: "media-playback-start"
                    Layout.fillWidth: true
                    onClicked: cardRoot.launchRequested()
                }
                QQC2.Button {
                    icon.name: "document-edit"
                    display: QQC2.AbstractButton.IconOnly
                    QQC2.ToolTip.text: "Edit"
                    QQC2.ToolTip.visible: hovered
                    onClicked: cardRoot.editRequested()
                }
                QQC2.Button {
                    icon.name: "edit-delete"
                    display: QQC2.AbstractButton.IconOnly
                    QQC2.ToolTip.text: "Remove"
                    QQC2.ToolTip.visible: hovered
                    onClicked: cardRoot.deleteRequested()
                }
            }
        }
    }
}
