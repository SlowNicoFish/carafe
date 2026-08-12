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
        radius: Kirigami.Units.largeSpacing * 0.9
        color: Kirigami.Theme.backgroundColor
        border.width: 1
        border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.1)

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            Rectangle {
                radius: Kirigami.Units.largeSpacing * 0.9
                clip: true
                color: "transparent"
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 120

                Image {
                    anchors.fill: parent
                    source: cardRoot.iconSource !== "" ? cardRoot.iconSource : "qrc:/res/icons/hicolor/scalable/apps/io.marlonn.carafe.svg"
                    fillMode: cardRoot.iconSource !== "" ? Image.PreserveAspectCrop : Image.PreserveAspectFit
                    sourceSize.width: 600
                    sourceSize.height: 900
                    smooth: true
                }
            }

            QQC2.Label {
                text: cardRoot.title
                Layout.fillWidth: true
                Layout.maximumHeight: 42
                Layout.minimumHeight: 20
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
                color: Kirigami.Theme.textColor
                font.pointSize: Kirigami.Theme.defaultFont.pointSize
            }

            RowLayout {
                spacing: Kirigami.Units.smallSpacing
                Layout.fillWidth: true
                Layout.minimumHeight: 32
                Layout.maximumHeight: 32

                QQC2.Button {
                    text: cardRoot.isRunning ? "Running" : "Launch"
                    enabled: !cardRoot.isRunning
                    icon.name: "media-playback-start"
                    Layout.fillWidth: true
                    Layout.minimumHeight: 32
                    onClicked: cardRoot.launchRequested()
                }
                QQC2.Button {
                    icon.name: "document-edit"
                    display: QQC2.AbstractButton.IconOnly
                    Layout.minimumWidth: 32
                    Layout.minimumHeight: 32
                    QQC2.ToolTip.text: "Edit"
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: cardRoot.editRequested()
                }
                QQC2.Button {
                    icon.name: "edit-delete"
                    display: QQC2.AbstractButton.IconOnly
                    Layout.minimumWidth: 32
                    Layout.minimumHeight: 32
                    QQC2.ToolTip.text: "Remove"
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    onClicked: cardRoot.deleteRequested()
                }
            }
        }
    }
}
