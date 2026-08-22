import QtQuick
import QtQuick.Controls as QQC2

QQC2.Menu {
    id: contextMenu
    property string gameId: ""
    signal launchRequested()
    signal editRequested()
    signal deleteRequested()
    signal fetchArtworkRequested()
    signal runExeRequested()

    QQC2.MenuItem {
        text: "Launch"
        icon.name: "media-playback-start"
        onTriggered: contextMenu.launchRequested()
    }
    QQC2.MenuItem {
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
        text: "Run Executable…"
        icon.name: "system-run"
        onTriggered: contextMenu.runExeRequested()
    }
    QQC2.MenuItem {
        text: "Remove"
        icon.name: "edit-delete"
        onTriggered: contextMenu.deleteRequested()
    }
}
