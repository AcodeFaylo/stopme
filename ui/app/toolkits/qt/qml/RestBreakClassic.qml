// RestBreakClassic.qml — GTK-faithful classic rest-break window.
// Loaded by RestBreakShell.qml when bridge.classic is true.
// All data comes from the C++ "bridge" context property (RestBreakBridge).

import QtQuick

Item {
    id: root

    // Historical Gtk msgids — reuses the existing po translations. The
    // mnemonic underscore ("_Skip") is stripped for display.
    readonly property string txtSkip:     qsTr("_Skip").replace("_", "")
    readonly property string txtPostpone: qsTr("_Postpone").replace("_", "")
    readonly property string txtShutdown: qsTr("Shutdown")
    readonly property string txtSleep:    qsTr("Suspend")

    // ── Design tokens ────────────────────────────────────────────────────────
    readonly property color colBg:      "#E8E8E8"
    readonly property color colBar:     "#5566FF"
    readonly property color colTimeBar: "#9AA8FF"   // matches .workrave-timebar-inactive in default.css
    readonly property color colBorder:  "#AAAAAA"
    readonly property color colWarn:    "#FFB020"
    readonly property color colInk:     "#1A1A1A"
    readonly property color colInk2:    "#444444"
    readonly property color colBtn:     "#D4D0C8"
    readonly property color colBtnTxt:  "#1A1A1A"

    // ── Bridge bindings ──────────────────────────────────────────────────────
    readonly property int    blockMode:    bridge != null ? bridge.blockMode        : 1
    readonly property bool   userActive:   bridge != null ? bridge.userActive       : false
    readonly property bool   isNatural:    bridge != null ? bridge.isNatural        : false
    readonly property bool   canPostpone:  bridge != null ? bridge.canPostpone      : true
    readonly property bool   canSkip:      bridge != null ? bridge.canSkip          : true
    readonly property bool   isLocked:     bridge != null ? bridge.isLocked         : false
    readonly property double lockProg:     bridge != null ? bridge.lockProgress     : 0.0
    readonly property bool   lockable:     bridge != null ? bridge.lockable         : false
    readonly property bool   shutdownable: bridge != null ? bridge.shutdownable     : false
    readonly property bool   sleepable:    bridge != null ? bridge.sleepable        : false
    // barProgress = elapsed fraction (0→1); timebar fills left-to-right
    readonly property double barProgress:  bridge != null ? bridge.breakProgress    : 0.0
    readonly property string timeLeft:     bridge != null ? bridge.breakTimeShort   : "5:00"

    // ── Flashing border ──────────────────────────────────────────────────────
    property bool flashState: false
    Timer {
        interval: 500; repeat: true
        running: root.userActive
        onTriggered: root.flashState = !root.flashState
        onRunningChanged: if (!running) root.flashState = false
    }
    readonly property color borderCol: root.userActive ? (root.flashState ? colWarn : colBg) : colBorder
    readonly property int   borderW:   root.userActive ? 6 : 1

    // ── Dim backdrop (blockMode All) ─────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.45)
        visible: root.blockMode === 2
        z: 0
    }

    // ── Card ─────────────────────────────────────────────────────────────────
    Rectangle {
        id: card
        z: 1
        width: Math.min(parent.width - 48, 440)
        color: colBg
        radius: 0
        border.color: root.borderCol
        border.width: root.borderW

        anchors.horizontalCenter: parent.horizontalCenter
        y: root.blockMode === 2 ? (parent.height - height) / 2
                                 : Math.max(24, (parent.height - height) / 2)

        Behavior on border.color { ColorAnimation { duration: 80 } }

        Column {
            id: cardContent
            anchors { top: parent.top; left: parent.left; right: parent.right }
            padding: 12
            spacing: 0

            // ── Body: info panel ─────────────────────────────────────────────
            Item {
                width: parent.width - 24
                height: infoPanel.height
                anchors.horizontalCenter: parent.horizontalCenter

                Row {
                    id: infoPanel
                    width: parent.width
                    spacing: 12

                    Image {
                        source: "qrc:/sanctuary/rest-break.png"
                        width: 64; height: 64
                        fillMode: Image.PreserveAspectFit
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4
                        width: parent.width - 76

                        Text {
                            width: parent.width
                            text: root.isNatural ? qsTr("Natural rest break") : qsTr("Rest break")
                            font.pixelSize: 15; font.bold: true; color: colInk
                        }

                        Text {
                            width: parent.width
                            text: root.isNatural
                                  ? qsTr("This is your natural rest break.")
                                  : qsTr("This is your rest break. Make sure you stand up and\nwalk away from your computer on a regular basis. Just\nwalk around for a few minutes, stretch, and relax.")
                            font.pixelSize: 13; color: colInk
                            wrapMode: Text.WordWrap; lineHeight: 1.4
                        }
                    }
                }
            }

            Item { width: 1; height: 12 }

            // ── TimeBar — white background, lightgreen fill, black text, like
            // the Gtk TimeBar widget ─────────────────────────────────────────
            Rectangle {
                width: parent.width - 24
                height: 22
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#FFFFFF"
                border.color: "#8F8F8F"; border.width: 1
                clip: true

                Rectangle {
                    x: 1; y: 1
                    width: Math.max(0, (parent.width - 2) * root.barProgress)
                    height: parent.height - 2
                    color: colTimeBar
                    Behavior on width { NumberAnimation { duration: 500 } }
                }

                Text {
                    anchors.centerIn: parent
                    text: bridge != null ? bridge.breakTime : qsTr("Rest break for {}")
                    font.pixelSize: 12
                    color: colInk
                }
            }

            // ── Lock progress bar (bare bar, no label) ───────────────────────
            Item {
                width: parent.width - 24
                height: root.isLocked ? 8 : 0
                anchors.horizontalCenter: parent.horizontalCenter
                clip: true
                Behavior on height { NumberAnimation { duration: 200 } }

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 4; color: "#C0C0C0"
                    Rectangle {
                        width: Math.max(4, parent.width * root.lockProg)
                        height: parent.height; color: colBar
                        Behavior on width { NumberAnimation { duration: 500 } }
                    }
                }
            }

            // Gtk packs the timebar and button box with 6px padding each plus
            // 6px box spacing: 18px gap in total.
            Item { width: 1; height: 18 }

            // ── Button row ───────────────────────────────────────────────────
            Item {
                width: parent.width - 24
                height: 28 + 8
                anchors.horizontalCenter: parent.horizontalCenter

                // Left-aligned: Gtk-style "Lock..." dropdown (or single button)
                ClassicSysOperMenu {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    lockable: root.lockable
                    shutdownable: root.shutdownable
                    sleepable: root.sleepable
                    onLockRequested: { if (bridge != null) bridge.requestLock() }
                    onShutdownRequested: confirmDlg.ask("shutdown", root.txtShutdown, qsTr("Are you sure you want to shut down the computer?"))
                    onSleepRequested: confirmDlg.ask("sleep", root.txtSleep, qsTr("Are you sure you want to put the computer to sleep?"))
                }

                // Right-aligned: Skip / Postpone (Gtk order)
                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6

                    ClassicButton {
                        visible: root.canSkip
                        enabled: root.canSkip
                        label: root.txtSkip
                        onClicked: { if (bridge != null) bridge.requestSkip() }
                    }
                    ClassicButton {
                        visible: root.canPostpone
                        enabled: root.canPostpone
                        label: root.txtPostpone
                        onClicked: { if (bridge != null) bridge.requestPostpone() }
                    }
                }
            }

            Item { width: 1; height: 8 }
        }

        height: cardContent.implicitHeight
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

    // ── Standard button ──────────────────────────────────────────────────────
    component ClassicButton: Rectangle {
        property string label: ""
        signal clicked()
        property bool hovered: false

        height: 28
        width: Math.max(btnLbl.implicitWidth + 24, 88)
        radius: 0
        color: hovered ? "#C0BBAF" : colBtn
        border.color: "#888888"; border.width: 1

        Text {
            id: btnLbl
            anchors.centerIn: parent
            text: parent.label
            font.pixelSize: 12
            color: parent.enabled ? colBtnTxt : "#888888"
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            hoverEnabled: true
            onEntered: parent.hovered = true
            onExited:  parent.hovered = false
            onClicked: parent.clicked()
            enabled: parent.enabled
        }
    }
}
