import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: listItemRoot
    implicitHeight: Kirigami.Units.gridUnit * 5

    property string gameId: ""
    property string title: ""
    property string iconSource: ""
    property bool isRunning: false

    signal launchRequested()
    signal editRequested()
    signal deleteRequested()
    signal fetchArtworkRequested()
    signal runExeInPrefixRequested()

    ContextMenu {
        id: contextMenu
        gameId: listItemRoot.gameId
        onLaunchRequested: listItemRoot.launchRequested()
        onEditRequested: listItemRoot.editRequested()
        onDeleteRequested: listItemRoot.deleteRequested()
        onFetchArtworkRequested: listItemRoot.fetchArtworkRequested()
        onRunExeRequested: listItemRoot.runExeInPrefixRequested()
    }

    MouseArea {
        id: interactionArea
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        hoverEnabled: true
        onClicked: contextMenu.popup()
    }

    Rectangle {
        anchors.fill: parent
        radius: Kirigami.Units.cornerRadius
        color: interactionArea.containsMouse
            ? Kirigami.Theme.alternateBackgroundColor
            : Kirigami.Theme.backgroundColor
        border.width: 1
        border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.15)

        RowLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            Image {
                source: listItemRoot.iconSource !== "" ? listItemRoot.iconSource : "qrc:/res/carafe_placeholder.png"
                Layout.preferredWidth: Kirigami.Units.iconSizes.huge
                Layout.preferredHeight: Kirigami.Units.iconSizes.huge
                fillMode: Image.PreserveAspectFit
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label {
                    text: listItemRoot.title
                    font.pointSize: Kirigami.Theme.defaultFont.pointSize
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            QQC2.Button {
                text: listItemRoot.isRunning ? "Running" : "Launch"
                icon.name: "media-playback-start"
                enabled: !listItemRoot.isRunning
                onClicked: listItemRoot.launchRequested()
            }

            QQC2.ToolButton {
                id: menuButton
                icon.name: "open-menu-symbolic"
                QQC2.ToolTip.text: "More actions"
                QQC2.ToolTip.visible: hovered
                onClicked: contextMenu.popup(menuButton, 0, menuButton.height)
            }
        }
    }

    Accessible.name: listItemRoot.title
    Accessible.role: Accessible.ListItem
}
