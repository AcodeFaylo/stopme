// SitStandShell.qml — root of the sit/stand reminder QQuickView.
// Places the card and picks SitStandOverlay.qml (Sanctuary) or
// SitStandClassic.qml (Gtk look) from bridge.classic.

import QtQuick

Item {
    id: root

    readonly property bool useClassic:   bridge != null ? bridge.classic    : false
    readonly property bool isFullscreen: bridge != null ? bridge.fullscreen : false

    readonly property int cardW:      bridge != null ? bridge.cardW : 480
    readonly property int cardH:      bridge != null ? bridge.cardH : 148
    readonly property int cardMargin: 20

    // Normal mode : the view is already the size of the card.
    // Fullscreen  : the card floats at the top centre of a transparent
    //               screen-sized window (Wayland).
    Loader {
        x:      root.isFullscreen ? (root.width - root.cardW) / 2 : 0
        y:      root.isFullscreen ? root.cardMargin : 0
        width:  root.isFullscreen ? root.cardW : root.width
        height: root.isFullscreen ? root.cardH : root.height

        source: root.useClassic ? Qt.resolvedUrl("SitStandClassic.qml")
                                : Qt.resolvedUrl("SitStandOverlay.qml")
    }
}
