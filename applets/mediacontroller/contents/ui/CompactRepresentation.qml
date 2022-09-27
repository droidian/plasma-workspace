/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PC3
import org.kde.plasma.plasmoid 2.0

/**
 * [Album Art][Now Playing]
 */
MouseArea {
    id: compactRepresentation

    Layout.preferredWidth: !inTray && !isVertical ? (iconLoader.active ? iconLoader.implicitWidth : playerRow.width) : -1

    readonly property bool isVertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property bool inPanel: [PlasmaCore.Types.TopEdge, PlasmaCore.Types.RightEdge, PlasmaCore.Types.BottomEdge, PlasmaCore.Types.LeftEdge].includes(Plasmoid.location)
    readonly property bool inTray: parent.objectName === "org.kde.desktop-CompactApplet"

    acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.BackButton | Qt.ForwardButton
    hoverEnabled: true

    onWheel: {
        const service = mpris2Source.serviceForSource(mpris2Source.current)
        const operation = service.operationDescription("ChangeVolume")
        operation.delta = (wheel.angleDelta.y / 120) * (volumePercentStep / 100)
        operation.showOSD = true
        service.startOperationCall(operation)
    }

    onClicked: {
        switch (mouse.button) {
        case Qt.MiddleButton:
            root.togglePlaying()
            break
        case Qt.BackButton:
            root.action_previous()
            break
        case Qt.ForwardButton:
            root.action_next()
            break
        default:
            Plasmoid.expanded = !Plasmoid.expanded
        }
    }

    Loader {
        id: iconLoader
        anchors.fill: parent
        visible: active

        active: inTray || !root.track
        sourceComponent: PlasmaCore.IconItem {
            active: compactRepresentation.containsMouse
            source: {
                if (root.isPlaying) {
                    return "media-playback-playing";
                } else if (root.state === "paused") {
                    return "media-playback-paused";
                } else {
                    return "media-playback-stopped";
                }
            }
        }
    }

    Loader {
        id: playerRow

        width: isVertical ? parent.width : inPanel ? item.implicitWidth : Math.min(item.implicitWidth, compactRepresentation.parent.width)
        height: isVertical ? parent.width : parent.height
        visible: active

        active: !iconLoader.active
        sourceComponent: RowLayout {
            spacing: PlasmaCore.Units.smallSpacing

            AlbumArtStackView {
                id: albumArt

                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: {
                    if (!inPanel) {
                        return Math.min(compactRepresentation.width, compactRepresentation.height);
                    }
                    return isVertical ? compactRepresentation.width : compactRepresentation.height;
                }
                Layout.preferredHeight: Layout.preferredWidth

                inCompactRepresentation: true

                Connections {
                    target: root

                    function onAlbumArtChanged() {
                        albumArt.loadAlbumArt();
                    }
                }

                Component.onCompleted: albumArt.loadAlbumArt()
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                visible: !isVertical

                spacing: 0

                // Song Title
                PC3.Label {
                    id: songTitle

                    Layout.fillWidth: true
                    Layout.maximumWidth: compactRepresentation.inPanel ? PlasmaCore.Units.gridUnit * 10 : -1

                    elide: Text.ElideRight
                    maximumLineCount: 1
                    text: root.track
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }

                // Song Artist
                PC3.Label {
                    id: songArtist

                    Layout.fillWidth: true
                    Layout.maximumWidth: songTitle.Layout.maximumWidth
                    visible: root.artist && playerRow.height >= songTitle.contentHeight + contentHeight

                    elide: Text.ElideRight
                    font.pointSize: PlasmaCore.Theme.smallestFont.pointSize
                    maximumLineCount: 1
                    text: root.artist
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
