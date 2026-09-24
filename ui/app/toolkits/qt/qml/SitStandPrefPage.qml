import QtQuick

// SitStandPrefPage — content for Timers > Sit / stand preferences.
// Expects context property: sitStandPrefBridge (SitStandPrefBridge)
Item {
    id: root

    property var bridge: typeof sitStandPrefBridge !== "undefined" ? sitStandPrefBridge : null

    implicitWidth:  parent ? parent.width : 500
    implicitHeight: col.implicitHeight

    Column {
        id: col
        anchors { left: parent.left; right: parent.right; top: parent.top }
        spacing: 24

        PrefToggleRow {
            width: parent.width
            label: qsTr("Remind me to switch between sitting and standing")
            hint:  qsTr("The first reminder asks you to stand up; after that they alternate.")
            checked: root.bridge ? root.bridge.enabled : false
            onToggled: (v) => { if (root.bridge) root.bridge.setEnabled(v) }
        }

        PrefGroup {
            width: parent.width
            visible: root.bridge ? root.bridge.enabled : true
            title: qsTr("Timing")

            PrefTimeControl {
                width: parent.width
                label: qsTr("Time between reminders")
                hint:  qsTr("Time away from the computer does not count: after five minutes away, the countdown starts over.")
                value: root.bridge ? root.bridge.intervalDisplay : "30:00"
                sliderValue: root.bridge ? root.bridge.intervalNorm : 0.217
                secondsStep: 60
                ticks: [
                    { at: 0.000, label: "5m"  },
                    { at: 0.217, label: "30m" },
                    { at: 0.478, label: "1h"  },
                    { at: 1.000, label: "2h"  },
                ]
                onIncrement:   { if (root.bridge) root.bridge.incrementInterval() }
                onDecrement:   { if (root.bridge) root.bridge.decrementInterval() }
                onSliderMoved: (v)    => { if (root.bridge) root.bridge.setIntervalNorm(v) }
                onCommitted:   (secs) => { if (root.bridge) root.bridge.setIntervalSeconds(secs) }
            }
        }
    }
}
