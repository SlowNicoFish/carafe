import QtQuick
import QtQuick.Controls
import org.kde.kirigami as Kirigami
import io.marlonn.carafe.backend

Item {
    id: root
    anchors.fill: parent

    signal editRequested(string gameId)
    signal deleteRequested(string gameId)
    signal launchRequested(string gameId)
    signal fetchArtworkRequested(string gameId)
    signal runExeInPrefixRequested(string gameId)

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.largeSpacing * 4
        visible: Backend.gameModel.count === 0
        icon.name: "applications-games"
        text: "No games yet"
        explanation: "Add your first game with the + button in the header."
    }

    ListView {
        id: listView
        anchors.fill: parent
        model: Backend.gameModel
        spacing: Kirigami.Units.smallSpacing
        clip: true
        focus: true
        keyNavigationEnabled: true

        Keys.onReturnPressed: {
            const item = currentItem as GameListItem
            if (item)
                root.launchRequested(item.gameId)
        }
        Keys.onEnterPressed: {
            const item = currentItem as GameListItem
            if (item)
                root.launchRequested(item.gameId)
        }

        ScrollBar.vertical: ScrollBar { }

        delegate: GameListItem {
            width: ListView.view.width
            gameId:     model.gameId
            title:      model.title
            iconSource: {
                const p = model.steamgridIconPath || model.iconPath
                return p ? Backend.localFileToUrl(p) : ""
            }
            isRunning:  model.isRunning
            onLaunchRequested:       root.launchRequested(model.gameId)
            onEditRequested:         root.editRequested(model.gameId)
            onDeleteRequested:       root.deleteRequested(model.gameId)
            onFetchArtworkRequested: root.fetchArtworkRequested(model.gameId)
            onRunExeInPrefixRequested: root.runExeInPrefixRequested(model.gameId)
        }
    }
}
