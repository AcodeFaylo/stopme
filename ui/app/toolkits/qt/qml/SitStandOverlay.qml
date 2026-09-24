// SitStandOverlay.qml — Sanctuary sit/stand reminder card.
// Fills its parent (placed by SitStandShell.qml).
// Data comes from the "bridge" context property (SitStandBridge).

import QtQuick

Item {
    id: root

    PrefTokens { id: tok }

    Rectangle {
        id: card
        anchors.fill: parent
        color: tok.panel
        radius: 16
        opacity: 0
        scale: 0.94
        border.width: 1.5
        border.color: tok.edge

        // Panda on a soft accent disc, like the break warning's badge.
        Rectangle {
            id: badge
            anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
            width: 108; height: 108
            radius: 999
            color: tok.accentSoft

            Image {
                anchors.centerIn: parent
                source: bridge != null ? bridge.image : "qrc:/sanctuary/stopme-panda-standing.svg"
                width: 100; height: 100
                sourceSize: Qt.size(200, 200)
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
        }

        Column {
            anchors {
                left: badge.right; leftMargin: 18
                right: parent.right; rightMargin: 22
                top: parent.top; topMargin: 20
            }
            spacing: 4

            Text {
                width: parent.width
                text: bridge != null ? bridge.heading : qsTr("Time to stand up")
                font.pixelSize: 19
                font.family: tok.displayFamily
                font.weight: Font.DemiBold
                color: tok.ink
                elide: Text.ElideRight
                renderType: Text.NativeRendering
            }

            Text {
                width: parent.width
                text: bridge != null ? bridge.body : ""
                font.pixelSize: 12
                color: tok.mute
                wrapMode: Text.WordWrap
                lineHeight: tok.hintLineH
            }
        }

        // OK button, bottom right — the primary action style of the break windows.
        Rectangle {
            anchors { right: parent.right; rightMargin: 22; bottom: parent.bottom; bottomMargin: 18 }
            height: 34
            width: Math.max(okLabel.implicitWidth + 28, 72)
            radius: tok.actionRadius
            color: okMouse.containsMouse ? tok.accentStrong : tok.accent
            Behavior on color { ColorAnimation { duration: 120 } }

            Text {
                id: okLabel
                anchors.centerIn: parent
                text: qsTr("OK")
                font.pixelSize: 13
                font.weight: Font.Medium
                font.letterSpacing: 0.12
                color: "#FFFFFF"
            }

            Accessible.role: Accessible.Button
            Accessible.name: okLabel.text

            MouseArea {
                id: okMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: { if (bridge != null) bridge.dismiss() }
            }
        }
    }

    // ── Enter animation ───────────────────────────────────────────────────────
    Component.onCompleted: enterAnim.start()

    ParallelAnimation {
        id: enterAnim
        NumberAnimation { target: card; property: "opacity"; to: 1.0; duration: 220; easing.type: Easing.OutCubic }
        NumberAnimation { target: card; property: "scale";   to: 1.0; duration: 220; easing.type: Easing.OutCubic }
    }
}
