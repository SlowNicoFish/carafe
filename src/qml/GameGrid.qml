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

    readonly property int targetCardWidth: 220
    readonly property int columns: Math.max(1, Math.floor(width / targetCardWidth))
    readonly property int cellW: Math.floor(width / columns)
    readonly property int cellH: Math.round(cellW * 1.5)

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.largeSpacing * 4
        visible: Backend.gameModel.count === 0
        icon.name: "applications-games"
        text: "No games yet"
        explanation: "Add your first game with the + button in the header."
    }

    GridView {
        id: gridView
        anchors.fill: parent
        model: Backend.gameModel
        cellWidth: root.cellW
        cellHeight: root.cellH
        clip: true
        focus: true
        keyNavigationEnabled: true

        Keys.onReturnPressed: {
            const card = currentItem as GameCard
            if (card)
                root.launchRequested(card.gameId)
        }
        Keys.onEnterPressed: {
            const card = currentItem as GameCard
            if (card)
                root.launchRequested(card.gameId)
        }

        ScrollBar.vertical: ScrollBar { }

        delegate: GameCard {
            width: root.cellW - Kirigami.Units.largeSpacing
            height: root.cellH - Kirigami.Units.largeSpacing
            gameId: model.gameId
            title: model.title
            iconSource: {
                const p = model.gridPath
                return p ? Backend.localFileToUrl(p) : "qrc:/res/carafe_placeholder.png";
            }
            launchArgs: model.launchArgs
            prefixPath: model.prefixPath
            protonVersion: model.protonVersion
            umuId: model.umuId
            isRunning: model.isRunning
            onSelectRequested: gridView.currentIndex = index
            onLaunchRequested: root.launchRequested(model.gameId)
            onEditRequested: root.editRequested(model.gameId)
            onDeleteRequested: root.deleteRequested(model.gameId)
            onFetchArtworkRequested: root.fetchArtworkRequested(model.gameId)
            onRunExeInPrefixRequested: root.runExeInPrefixRequested(model.gameId)
        }
    }
}
