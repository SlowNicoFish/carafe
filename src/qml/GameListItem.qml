import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Item {
    id: listItemRoot
    width: parent?.width ?? 0
    implicitHeight: Kirigami.Units.gridUnit * 5

    property string gameId: ""
    property string title: ""
    property string subtitle: ""
    property string iconSource: ""
    property bool isRunning: false

    signal launchRequested
    signal editRequested
    signal deleteRequested
    signal fetchArtworkRequested

    Rectangle {
        anchors.fill: parent
        radius: Kirigami.Units.cornerRadius
        color: Kirigami.Theme.backgroundColor
        border.width: 1
        border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.15)

        RowLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            Image {
                source: listItemRoot.iconSource !== "" ? listItemRoot.iconSource : "qrc:/res/icons/hicolor/scalable/apps/io.marlonn.carafe.svg"
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
                onClicked: itemMenu.open()

                QQC2.Menu {
                    id: itemMenu
                    y: menuButton.height
                    QQC2.MenuItem {
                        text: "Edit"
                        icon.name: "document-edit"
                        onTriggered: editRequested()
                    }
                    QQC2.MenuItem {
                        text: "Fetch Artwork"
                        icon.name: "download"
                        onTriggered: fetchArtworkRequested()
                    }
                    QQC2.MenuItem {
                        text: "Remove"
                        icon.name: "edit-delete"
                        onTriggered: deleteRequested()
                    }
                }
            }
        }
    }
}
