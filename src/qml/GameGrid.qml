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

    // Target card width; actual width fills evenly across available space.
    readonly property int targetCardWidth: 280
    readonly property int columns: Math.max(1, Math.floor(width / targetCardWidth))
    readonly property int cellW: Math.floor(width / columns)
    readonly property int cellH: Math.round(cellW * 1.5)

    GridView {
        anchors.fill: parent
        model: Backend.gameModel
        cellWidth: root.cellW
        cellHeight: root.cellH
        clip: true

        delegate: GameCard {
            width: root.cellW - Kirigami.Units.largeSpacing
            height: root.cellH - Kirigami.Units.largeSpacing
            gameId: model.gameId
            title: model.title
            iconSource: {
                const p = model.gridPath;
                return p ? "file://" + p : "";
            }
            launchArgs: model.launchArgs
            prefixPath: model.prefixPath
            protonVersion: model.protonVersion
            umuId: model.umuId
            isRunning: model.isRunning
            onLaunchRequested: root.launchRequested(model.gameId)
            onEditRequested: root.editRequested(model.gameId)
            onDeleteRequested: root.deleteRequested(model.gameId)
            onFetchArtworkRequested: root.fetchArtworkRequested(model.gameId)
            onRunExeInPrefixRequested: root.runExeInPrefixRequested(model.gameId)
        }
    }
}
