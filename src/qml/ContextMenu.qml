import QtQuick
import QtQuick.Controls as QQC2

QQC2.Menu {
    id: contextMenu
    property string gameId: ""
    property bool isRunning: false

    signal launchRequested
    signal stopRequested
    signal editRequested
    signal deleteRequested
    signal fetchArtworkRequested
    signal runExeRequested

    QQC2.MenuItem {
        visible: !contextMenu.isRunning
        text: "Launch"
        icon.name: "media-playback-start"
        onTriggered: contextMenu.launchRequested()
    }
    QQC2.MenuItem {
        visible: contextMenu.isRunning
        text: "Stop"
        icon.name: "media-playback-stop"
        onTriggered: contextMenu.stopRequested()
    }
    QQC2.MenuItem {
        enabled: !contextMenu.isRunning
        text: "Edit"
        icon.name: "document-edit"
        onTriggered: contextMenu.editRequested()
    }
    QQC2.MenuItem {
        text: "Fetch Artwork"
        icon.name: "download"
        onTriggered: contextMenu.fetchArtworkRequested()
    }
    QQC2.MenuItem {
        enabled: !contextMenu.isRunning
        text: "Run Executable…"
        icon.name: "system-run"
        onTriggered: contextMenu.runExeRequested()
    }
    QQC2.MenuItem {
        enabled: !contextMenu.isRunning
        text: "Remove"
        icon.name: "edit-delete"
        onTriggered: contextMenu.deleteRequested()
    }
}
