import QtQuick
import QtQuick.Controls as QQC2

QQC2.Menu {
    id: contextMenu
    property string gameId: ""
    signal launchRequested(string gameId)
    signal editRequested(string gameId)
    signal deleteRequested(string gameId)
    signal fetchArtworkRequested(string gameId)
    signal runExeRequested(string gameId)

    QQC2.MenuItem {
        text: "Launch"
        onTriggered: contextMenu.launchRequested(contextMenu.gameId)
    }
    QQC2.MenuItem {
        text: "Edit"
        onTriggered: contextMenu.editRequested(contextMenu.gameId)
    }
    QQC2.MenuItem {
        text: "Fetch Artwork"
        icon.name: "download"
        onTriggered: contextMenu.fetchArtworkRequested(contextMenu.gameId)
    }
    QQC2.MenuItem {
        text: "Run Executable…"
        icon.name: "system-run"
        onTriggered: contextMenu.runExeRequested(contextMenu.gameId)
    }
    QQC2.MenuItem {
        text: "Delete"
        onTriggered: contextMenu.deleteRequested(contextMenu.gameId)
    }
}
