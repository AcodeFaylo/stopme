// SitStandClassic.qml — Gtk-faithful sit/stand reminder card: the panda,
// a bold heading, a line of text and an OK button, on the grey card of the
// classic break windows. Fills its parent (placed by SitStandShell.qml).

import QtQuick

Item {
    id: root

    readonly property color colBg:     "#E8E8E8"
    readonly property color colBorder: "#8A8885"
    readonly property color colInk:    "#1A1A1A"
    readonly property color colBtn:    "#D4D0C8"

    Rectangle {
        id: card
        anchors.fill: parent
        color: colBg
        radius: 2
        border.color: colBorder
        border.width: 1

        Image {
            id: panda
            anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
            source: bridge != null ? bridge.image : "qrc:/sanctuary/stopme-panda-standing.svg"
            width: 96; height: 96
            sourceSize: Qt.size(192, 192)
            fillMode: Image.PreserveAspectFit
            smooth: true
        }

        Column {
            anchors {
                left: panda.right; leftMargin: 12
                right: parent.right; rightMargin: 12
                top: parent.top; topMargin: 14
            }
            spacing: 6

            Text {
                width: parent.width
                text: bridge != null ? bridge.heading : qsTr("Time to stand up")
                font.pixelSize: 15
                font.bold: true
                color: colInk
                elide: Text.ElideRight
                renderType: Text.NativeRendering
            }

            Text {
                width: parent.width
                text: bridge != null ? bridge.body : ""
                font.pixelSize: 12
                color: colInk
                wrapMode: Text.WordWrap
                renderType: Text.NativeRendering
            }
        }

        // OK button, bottom right — the classic break windows' button style.
        Rectangle {
            id: okButton
            anchors { right: parent.right; rightMargin: 12; bottom: parent.bottom; bottomMargin: 12 }
            height: 28
            width: Math.max(okLabel.implicitWidth + 24, 88)
            color: okMouse.containsMouse ? "#C0BBAF" : colBtn
            border.color: "#888888"
            border.width: 1

            Text {
                id: okLabel
                anchors.centerIn: parent
                text: qsTr("OK")
                font.pixelSize: 12
                color: colInk
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

    // Subtle fade-in when loaded
    Component.onCompleted: {
        card.opacity = 0
        fadeIn.start()
    }

    NumberAnimation {
        id: fadeIn
        target: card
        property: "opacity"
        to: 1.0
        duration: 120
        easing.type: Easing.OutCubic
    }
}
