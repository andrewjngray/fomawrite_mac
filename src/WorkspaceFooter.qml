import QtQuick

// Every workspace column reserves the same baseline and follows its own pane.
Rectangle {
    implicitHeight: 52
    height: implicitHeight
    color: backend.palette.page
    Rectangle {
        objectName: "footerTopRule"
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        height: 1
        color: backend.palette.border
    }
}
