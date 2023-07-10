/*
    SPDX-FileCopyrightText: 2016, 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kirigami 2.20 as Kirigami

Item {
    id: area

    signal activated
    signal contextMenuRequested(var pos)

    required property Item dragParent
    property int dragPixmapSize: Kirigami.Units.iconSizes.large

    readonly property alias dragging: dragHandler.active
    readonly property alias hovered: hoverHandler.hovered

    HoverHandler {
        id: hoverHandler
        cursorShape: tapHandler.pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
    }

    TapHandler {
        id: tapHandler
        acceptedButtons: Qt.LeftButton

        onTapped: {
            area.activated();
        }
    }

    TapHandler {
        id: menuTapHandler
        acceptedButtons: Qt.LeftButton
        acceptedDevices: PointerDevice.TouchScreen | PointerDevice.Stylus
        onLongPressed: area.contextMenuRequested(point.position)
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        gesturePolicy: TapHandler.WithinBounds

        onPressedChanged: if (pressed) {
            menuTapHandler.longPressed()
        }
    }

    DragHandler {
        id: dragHandler

        onActiveChanged: if (active) {
            area.dragParent.grabToImage(result => {
                area.dragParent.Drag.imageSource = result.url;
                area.dragParent.Drag.active = dragHandler.active;
            }, Qt.size(area.dragPixmapSize, area.dragPixmapSize));
        } else {
            area.dragParent.Drag.imageSource = "";
            area.dragParent.Drag.active = false;
        }
    }
}
