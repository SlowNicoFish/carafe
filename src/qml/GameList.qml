import QtQuick
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

    ListView {
        anchors.fill: parent
        model: Backend.gameModel
        spacing: Kirigami.Units.smallSpacing
        clip: true

        delegate: GameListItem {
            width: ListView.view.width
            gameId:     model.gameId
            title:      model.title
            iconSource: {
                const p = model.steamgridIconPath || model.iconPath
                return p ? "file://" + p : ""
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
