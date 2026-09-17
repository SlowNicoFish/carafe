import QtQuick

Item {
    id: root

    property string gameId: ""

    signal launchRequested()
    signal editRequested()
    signal deleteRequested()
    signal fetchArtworkRequested()
    signal runExeInPrefixRequested()

    function popup(parentItem, x, y) {
        if (parentItem)
            contextMenu.popup(parentItem, x, y)
        else
            contextMenu.popup()
    }

    ContextMenu {
        id: contextMenu
        gameId: root.gameId
        onLaunchRequested: root.launchRequested()
        onEditRequested: root.editRequested()
        onDeleteRequested: root.deleteRequested()
        onFetchArtworkRequested: root.fetchArtworkRequested()
        onRunExeRequested: root.runExeInPrefixRequested()
    }
}
