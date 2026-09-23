// RestBreakOverlay.qml — Sanctuary-style rest-break window
// Loaded by QmlRestBreakWindow into a QQuickView.
// All data comes from the C++ "bridge" context property (RestBreakBridge).
//
// Two visual modes:
//   blockMode 2 (All)   — warm full-screen background, centred break card
//   blockMode 0/1       — transparent full-screen view, floating white card (centered)

import QtQuick

Item {
    id: root

    // Historical Gtk msgids — reuses the existing po translations. The
    // mnemonic underscore ("_Skip") is stripped for display.
    readonly property string txtSkip:     qsTr("_Skip").replace("_", "")
    readonly property string txtPostpone: qsTr("_Postpone").replace("_", "")
    readonly property string txtShutdown: qsTr("Shutdown")
    readonly property string txtSleep:    qsTr("Suspend")

    PrefTokens { id: tok }

    // ── Bridge bindings ──────────────────────────────────────────────────────
    readonly property int        blockMode:       bridge != null ? bridge.blockMode          : 1
    readonly property bool       canPostpone:     bridge != null ? bridge.canPostpone        : true
    readonly property bool       canSkip:         bridge != null ? bridge.canSkip            : true
    readonly property bool       lockable:        bridge != null ? bridge.lockable           : false
    readonly property bool       shutdownable:    bridge != null ? bridge.shutdownable       : false
    readonly property bool       sleepable:       bridge != null ? bridge.sleepable          : false
    readonly property bool       isLocked:        bridge != null ? bridge.isLocked           : false
    readonly property double     lockProgress:     bridge != null ? bridge.lockProgress      : 0.0
    readonly property double     breakProgress:   bridge != null ? bridge.breakProgress      : 1.0
    readonly property string     breakTimeShort:  bridge != null ? bridge.breakTimeShort     : "5:00"

    readonly property int breakTimeColonIndex: root.breakTimeShort.lastIndexOf(":")
    readonly property string breakTimeBeforeColon: root.breakTimeColonIndex >= 0 ? root.breakTimeShort.slice(0, root.breakTimeColonIndex) : "0"
    readonly property string breakTimeAfterColon: root.breakTimeColonIndex >= 0 ? root.breakTimeShort.slice(root.breakTimeColonIndex + 1) : root.breakTimeShort

    // ════════════════════════════════════════════════════════════════════════
    // CARD LAYOUT  (blockMode 0 = Off, 1 = Input)
    // Transparent full-screen view; a white rounded card is centered inside.
    // ════════════════════════════════════════════════════════════════════════
    Item {
        id: cardLayout
        anchors.fill: parent
        visible: root.blockMode !== 2

        Rectangle {
            id: card
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 1040)
            height: cardCol.implicitHeight
            color: tok.panel
            radius: 24
            border.color: tok.edge
            border.width: 0.5

            Column {
                id: cardCol
                width: parent.width

                // ── Compact header ───────────────────────────────────────────
                Item {
                    width: parent.width
                    height: 52

                    // Left: ● breadcrumb
                    Row {
                        anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
                        spacing: 0

                        Rectangle {
                            width: 6; height: 6; radius: 999; color: tok.accent2
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Item { width: 8; height: 1 }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Rest break").toUpperCase()
                            font.pixelSize: 11; font.weight: Font.DemiBold
                            font.letterSpacing: 0.5; color: tok.mute
                        }
                    }

                    // Right: lock | shutdown | sleep
                    Row {
                        anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }
                        spacing: 4

                        Rectangle {
                            visible: root.lockable
                            width: 28; height: 28; radius: tok.actionRadius
                            color: tok.actionBg; border.color: tok.actionEdge; border.width: 1
                            anchors.verticalCenter: parent.verticalCenter
                            Text { anchors.centerIn: parent; text: "🔒"; font.pixelSize: 12 }
                            Accessible.role: Accessible.Button
                            Accessible.name: qsTr("Lock screen")
                            MouseArea {
                                anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: { if (bridge != null) bridge.requestLock() }
                            }
                        }

                        Rectangle {
                            visible: root.shutdownable
                            width: 28; height: 28; radius: tok.actionRadius
                            color: tok.actionBg; border.color: tok.actionEdge; border.width: 1
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                anchors.centerIn: parent; text: "⏻"
                                font.pixelSize: 15; font.weight: Font.Bold; color: tok.ink2
                            }
                            Accessible.role: Accessible.Button; Accessible.name: root.txtShutdown
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: confirmDlg.ask("shutdown", root.txtShutdown, qsTr("Are you sure you want to shut down the computer?")) }
                        }

                        Rectangle {
                            visible: root.sleepable
                            width: 28; height: 28; radius: tok.actionRadius
                            color: tok.actionBg; border.color: tok.actionEdge; border.width: 1
                            anchors.verticalCenter: parent.verticalCenter
                            Text {
                                anchors.centerIn: parent; text: "☾"
                                font.pixelSize: 17; font.weight: Font.Bold; color: tok.ink2
                            }
                            Accessible.role: Accessible.Button; Accessible.name: root.txtSleep
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: confirmDlg.ask("sleep", root.txtSleep, qsTr("Are you sure you want to put the computer to sleep?")) }
                        }
                    }
                }

                // Separator
                Rectangle { width: parent.width; height: 1; color: tok.edge }

                // ── Lock strip ───────────────────────────────────────────────
                Item {
                    width: parent.width
                    height: root.isLocked ? cardLockCol.implicitHeight + 16 : 0
                    clip: true
                    Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

                    Column {
                        id: cardLockCol
                        anchors {
                            left: parent.left; right: parent.right
                            leftMargin: 24; rightMargin: 24
                            bottom: parent.bottom; bottomMargin: 8
                        }
                        spacing: 6
                        Text {
                            width: parent.width; horizontalAlignment: Text.AlignHCenter
                            text: qsTr("Postpone and skip will unlock after resting")
                            font.pixelSize: 11; color: tok.mute
                        }
                        Rectangle {
                            width: parent.width; height: 4; radius: 2; color: tok.track
                            Rectangle {
                                anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                                width: Math.max(4, parent.width * root.lockProgress)
                                radius: 2; color: tok.accent
                                Behavior on width { NumberAnimation { duration: 500 } }
                            }
                        }
                    }
                }

                // ── Body — countdown ring ────────────────────────────────────
                Item {
                    id: cardBody
                    width: parent.width
                    height: cardRingCol.implicitHeight

                    Column {
                        id: cardRingCol
                        width: parent.width
                        topPadding: 32; bottomPadding: 36; spacing: 4

                        Image {
                            anchors.horizontalCenter: parent.horizontalCenter
                            source: "qrc:/sanctuary/stopme-panda-face.svg"
                            width: 64; height: 64
                            sourceSize: Qt.size(128, 128)
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }

                        Item { width: 1; height: 12 }

                        Canvas {
                            id: cardRingCanvas
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 280; height: 280

                            property double prog: root.breakProgress
                            onProgChanged: requestPaint()
                            Component.onCompleted: requestPaint()

                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.clearRect(0, 0, width, height);
                                var cx = 140, cy = 140, r = 133, sw = 8;
                                ctx.beginPath();
                                ctx.arc(cx, cy, r, 0, 2 * Math.PI);
                                ctx.strokeStyle = tok.track.toString();
                                ctx.lineWidth = sw; ctx.stroke();
                                ctx.beginPath();
                                ctx.arc(cx, cy, r, -Math.PI / 2,
                                        -Math.PI / 2 + 2 * Math.PI * Math.max(prog, 0.001));
                                ctx.strokeStyle = tok.accent.toString();
                                ctx.lineWidth = sw; ctx.lineCap = "round"; ctx.stroke();
                            }

                            Item {
                                anchors.centerIn: parent
                                width: 240; height: 84

                                Text {
                                    anchors { right: parent.horizontalCenter; rightMargin: 10; verticalCenter: parent.verticalCenter }
                                    width: 100
                                    text: root.breakTimeBeforeColon
                                    font.pixelSize: 70; font.family: tok.displayFamily; color: tok.ink
                                    horizontalAlignment: Text.AlignRight
                                    font.features: {"tnum": 1}
                                }
                                Text {
                                    anchors.centerIn: parent
                                    text: ":"
                                    font.pixelSize: 70; font.family: tok.displayFamily; color: tok.ink
                                    font.features: {"tnum": 1}
                                }
                                Text {
                                    anchors { left: parent.horizontalCenter; leftMargin: 10; verticalCenter: parent.verticalCenter }
                                    width: 100
                                    text: root.breakTimeAfterColon
                                    font.pixelSize: 70; font.family: tok.displayFamily; color: tok.ink
                                    horizontalAlignment: Text.AlignLeft
                                    font.features: {"tnum": 1}
                                }
                            }
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("Please stand up and walk away from your computer")
                            font.pixelSize: 13; color: tok.mute
                        }

                        Item { width: 1; height: 16 }

                        // Action buttons — mirroring micro-break card layout
                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 8

                            Rectangle {
                                visible: root.canSkip
                                height: 34; width: skipLabelR.implicitWidth + 28
                                radius: tok.actionRadius; color: tok.actionBg
                                border.color: tok.actionEdge; border.width: 1
                                Text {
                                    id: skipLabelR; anchors.centerIn: parent
                                    text: root.txtSkip; font.pixelSize: 13; font.weight: Font.Medium
                                    font.letterSpacing: 0.12; color: tok.ink2
                                }
                                Accessible.role: Accessible.Button; Accessible.name: root.txtSkip
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (bridge != null) bridge.requestSkip() } }
                            }

                            Rectangle {
                                visible: root.canPostpone
                                height: 34; width: ringPostponeLbl.implicitWidth + 28
                                radius: tok.actionRadius; color: tok.actionBg; border.color: tok.actionEdge; border.width: 1
                                opacity: root.canPostpone ? 1.0 : 0.4
                                anchors.verticalCenter: parent.verticalCenter
                                Text {
                                    id: ringPostponeLbl; anchors.centerIn: parent
                                    text: root.txtPostpone; font.pixelSize: 13; font.weight: Font.Medium; color: tok.ink2
                                }
                                Accessible.role: Accessible.Button; Accessible.name: root.txtPostpone
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (bridge != null) bridge.requestPostpone() } }
                            }
                        }
                    }
                }
            }
        }

        // Card enter animation
        Component.onCompleted: {
            card.opacity = 0
            card.scale   = 0.98
            cardEnterAnim.start()
        }
        SequentialAnimation {
            id: cardEnterAnim
            ParallelAnimation {
                NumberAnimation { target: card; property: "opacity"; to: 1.0; duration: 260; easing.type: Easing.OutCubic }
                NumberAnimation { target: card; property: "scale";   to: 1.0; duration: 260; easing.type: Easing.OutCubic }
            }
        }
    }

    // ════════════════════════════════════════════════════════════════════════
    // FULL-SCREEN LAYOUT  (blockMode 2 = Block input + screen)
    // Warm background fills the screen; the break card sits in the middle.
    // ════════════════════════════════════════════════════════════════════════
    Item {
        id: fullScreenLayout
        anchors.fill: parent
        visible: root.blockMode === 2

        Rectangle { anchors.fill: parent; color: tok.bg; z: 0 }

        Item {
            id: contentArea
            anchors { fill: parent; leftMargin: 48; rightMargin: 48; topMargin: 32; bottomMargin: 40 }
            z: 1

            // ── Header strip ──────────────────────────────────────────────────
            Item {
                id: headerStrip
                anchors { top: parent.top; left: parent.left; right: parent.right }
                height: 36

                Row {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8
                    Rectangle { width: 6; height: 6; radius: 999; color: tok.accent2; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("Rest break").toUpperCase()
                        font.pixelSize: 12; font.weight: Font.DemiBold; font.letterSpacing: 0.5; color: tok.mute
                    }
                }

                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4

                    Rectangle {
                        visible: root.lockable
                        width: 28; height: 28; radius: tok.actionRadius
                        color: tok.actionBg; border.color: tok.actionEdge; border.width: 1
                        anchors.verticalCenter: parent.verticalCenter
                        Text { anchors.centerIn: parent; text: "🔒"; font.pixelSize: 12 }
                        Accessible.role: Accessible.Button; Accessible.name: qsTr("Lock screen")
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (bridge != null) bridge.requestLock() } }
                    }

                    Rectangle {
                        visible: root.shutdownable
                        width: 28; height: 28; radius: tok.actionRadius
                        color: tok.actionBg; border.color: tok.actionEdge; border.width: 1
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            anchors.centerIn: parent; text: "⏻"
                            font.pixelSize: 15; font.weight: Font.Bold; color: tok.ink2
                        }
                        Accessible.role: Accessible.Button; Accessible.name: root.txtShutdown
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: confirmDlg.ask("shutdown", root.txtShutdown, qsTr("Are you sure you want to shut down the computer?")) }
                    }

                    Rectangle {
                        visible: root.sleepable
                        width: 28; height: 28; radius: tok.actionRadius
                        color: tok.actionBg; border.color: tok.actionEdge; border.width: 1
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            anchors.centerIn: parent; text: "☾"
                            font.pixelSize: 17; font.weight: Font.Bold; color: tok.ink2
                        }
                        Accessible.role: Accessible.Button; Accessible.name: root.txtSleep
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: confirmDlg.ask("sleep", root.txtSleep, qsTr("Are you sure you want to put the computer to sleep?")) }
                    }

                    Rectangle {
                        width: 1; height: 18; color: tok.edge
                        anchors.verticalCenter: parent.verticalCenter
                        visible: (root.lockable || root.shutdownable || root.sleepable) && (root.canPostpone || root.canSkip)
                    }

                    Rectangle {
                        visible: root.canSkip
                        height: 28; width: skipLabelFs.implicitWidth + 20
                        radius: tok.actionRadius; color: tok.actionBg
                        border.color: tok.actionEdge; border.width: 1
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            id: skipLabelFs; anchors.centerIn: parent
                            text: root.txtSkip; font.pixelSize: 12; font.weight: Font.Medium; color: tok.ink2
                        }
                        Accessible.role: Accessible.Button; Accessible.name: root.txtSkip
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (bridge != null) bridge.requestSkip() } }
                    }

                    Rectangle {
                        visible: root.canPostpone
                        height: 28; width: postponeLabel.implicitWidth + 20
                        radius: tok.actionRadius; color: tok.actionBg; border.color: tok.actionEdge; border.width: 1
                        opacity: root.canPostpone ? 1.0 : 0.4
                        anchors.verticalCenter: parent.verticalCenter
                        Text { id: postponeLabel; anchors.centerIn: parent; text: root.txtPostpone; font.pixelSize: 12; font.weight: Font.Medium; color: tok.ink2 }
                        Accessible.role: Accessible.Button; Accessible.name: root.txtPostpone
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (bridge != null) bridge.requestPostpone() } }
                    }
                }
            }

            // ── Lock strip ────────────────────────────────────────────────────
            Item {
                id: lockStrip
                anchors { top: headerStrip.bottom; left: parent.left; right: parent.right }
                height: root.isLocked ? lockStripCol.implicitHeight + 12 : 0
                clip: true
                Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

                Column {
                    id: lockStripCol
                    anchors { left: parent.left; right: parent.right; bottom: parent.bottom; bottomMargin: 6 }
                    spacing: 6
                    Text {
                        width: parent.width; horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Postpone and skip will unlock after resting")
                        font.pixelSize: 11; color: tok.mute
                    }
                    Rectangle {
                        width: parent.width; height: 4; radius: 2; color: tok.track
                        Rectangle {
                            anchors { left: parent.left; top: parent.top; bottom: parent.bottom }
                            width: Math.max(4, parent.width * root.lockProgress)
                            radius: 2; color: tok.accent
                            Behavior on width { NumberAnimation { duration: 500 } }
                        }
                    }
                }
            }

            // ── Body area ─────────────────────────────────────────────────────
            Item {
                id: bodyArea
                anchors { top: lockStrip.bottom; topMargin: 20; left: parent.left; right: parent.right; bottom: parent.bottom }

                Item {
                    id: centerCol
                    anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; bottom: parent.bottom }
                    width: Math.min(parent.width, 1060)

                    Rectangle {
                        id: breakCard
                        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
                        height: Math.min(parent.height - 24, 680)
                        radius: 24; color: tok.panel; border.color: tok.edge; border.width: 0.5

                        // Ring view
                        Item {
                            anchors.fill: parent

                            Canvas {
                                id: ringCanvas
                                anchors.centerIn: parent
                                width: 320; height: 320

                                property double prog: root.breakProgress
                                onProgChanged: requestPaint()
                                Component.onCompleted: requestPaint()

                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.clearRect(0, 0, width, height);
                                    var cx = 160, cy = 160, r = 152, sw = 9;
                                    ctx.beginPath(); ctx.arc(cx, cy, r, 0, 2 * Math.PI); ctx.strokeStyle = tok.track.toString(); ctx.lineWidth = sw; ctx.stroke();
                                    ctx.beginPath(); ctx.arc(cx, cy, r, -Math.PI / 2, -Math.PI / 2 + 2 * Math.PI * Math.max(prog, 0.001)); ctx.strokeStyle = tok.accent.toString(); ctx.lineWidth = sw; ctx.lineCap = "round"; ctx.stroke();
                                }
                            }

                            // The ring is full inside, so the panda sits on top of it
                            Image {
                                anchors { horizontalCenter: parent.horizontalCenter; bottom: ringCanvas.top; bottomMargin: 20 }
                                source: "qrc:/sanctuary/stopme-panda-face.svg"
                                width: 72; height: 72
                                sourceSize: Qt.size(144, 144)
                                fillMode: Image.PreserveAspectFit
                                smooth: true
                            }

                            Column {
                                anchors.centerIn: parent
                                spacing: 4
                                Item {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: 280; height: 96

                                    Text {
                                        anchors { right: parent.horizontalCenter; rightMargin: 11; verticalCenter: parent.verticalCenter }
                                        width: 118
                                        text: root.breakTimeBeforeColon
                                        font.pixelSize: 80; font.family: tok.displayFamily; color: tok.ink
                                        horizontalAlignment: Text.AlignRight
                                        font.features: {"tnum": 1}
                                    }
                                    Text {
                                        anchors.centerIn: parent
                                        text: ":"
                                        font.pixelSize: 80; font.family: tok.displayFamily; color: tok.ink
                                        font.features: {"tnum": 1}
                                    }
                                    Text {
                                        anchors { left: parent.horizontalCenter; leftMargin: 11; verticalCenter: parent.verticalCenter }
                                        width: 118
                                        text: root.breakTimeAfterColon
                                        font.pixelSize: 80; font.family: tok.displayFamily; color: tok.ink
                                        horizontalAlignment: Text.AlignLeft
                                        font.features: {"tnum": 1}
                                    }
                                }
                            }

                            // Below the ring, where the text has the room it needs
                            Column {
                                anchors { horizontalCenter: parent.horizontalCenter; top: ringCanvas.bottom; topMargin: 20 }
                                spacing: 4
                                Text { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("Please stand up and walk away from your computer"); font.pixelSize: 13; color: tok.mute }
                                Item { width: 1; height: 16 }
                                Rectangle {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    visible: root.canSkip; height: 36; width: fsSkipLabel.implicitWidth + 28
                                    radius: tok.actionRadius; color: tok.actionBg; border.color: tok.actionEdge; border.width: 1
                                    Text { id: fsSkipLabel; anchors.centerIn: parent; text: qsTr("End break early"); font.pixelSize: 13; font.weight: Font.Medium; color: tok.ink2 }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { if (bridge != null) bridge.requestSkip() } }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Full-screen enter animation
        Component.onCompleted: {
            contentArea.opacity = 0
            contentArea.scale   = 0.98
            fsEnterAnim.start()
        }
        SequentialAnimation {
            id: fsEnterAnim
            ParallelAnimation {
                NumberAnimation { target: contentArea; property: "opacity"; to: 1.0; duration: 260; easing.type: Easing.OutCubic }
                NumberAnimation { target: contentArea; property: "scale";   to: 1.0; duration: 260; easing.type: Easing.OutCubic }
            }
        }
    }

    ConfirmDialog {
        id: confirmDlg
        anchors.fill: parent
        z: 200
        onConfirmed: (action) => {
            if (action === "shutdown") { if (bridge != null) bridge.requestShutdown() }
            else if (action === "sleep")    { if (bridge != null) bridge.requestSleep() }
        }
    }
}
