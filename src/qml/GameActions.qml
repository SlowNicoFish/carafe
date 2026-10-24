import QtQuick

Item {
    id: root

    property string gameId: ""
    property bool isRunning: false

    signal launchRequested
    signal stopRequested
    signal editRequested
    signal deleteRequested
    signal fetchArtworkRequested
    signal runExeInPrefixRequested

    function popup(parentItem, x, y) {
        if (parentItem)
            contextMenu.popup(parentItem, x, y);
        else
            contextMenu.popup();
    }

    ContextMenu {
        id: contextMenu
        gameId: root.gameId
        isRunning: root.isRunning
        onLaunchRequested: root.launchRequested()
        onStopRequested: root.stopRequested()
        onEditRequested: root.editRequested()
        onDeleteRequested: root.deleteRequested()
        onFetchArtworkRequested: root.fetchArtworkRequested()
        onRunExeRequested: root.runExeInPrefixRequested()
    }
}
