import QtQuick

QtObject {
    property bool   enabled:         true
    property string intervalDisplay: "30:00"
    property double intervalNorm:    0.217

    function setEnabled(v) {}
    function incrementInterval() {}
    function decrementInterval() {}
    function setIntervalNorm(v) {}
    function setIntervalSeconds(v) {}
}
