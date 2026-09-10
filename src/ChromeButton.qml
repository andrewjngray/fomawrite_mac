import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool darkMode: false
    property string hint: text
    implicitHeight: 28
    implicitWidth: Math.max(28, label.implicitWidth + 16)
    padding: 0
    leftPadding: 8
    rightPadding: 8
    topPadding: 0
    bottomPadding: 0
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    focusPolicy: Qt.StrongFocus
    Accessible.name: hint
    ToolTip.visible: hovered && hint !== ""
    ToolTip.text: hint
    contentItem: Text {
        id: label
        text: control.text
        font.family: "Helvetica Neue"
        font.pixelSize: 12
        font.bold: control.checked
        color: !control.enabled ? "#96999e" : control.checked ? "#008bad" : control.darkMode ? "#d5d8dd" : "#40444b"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 5
        color: control.checked ? (control.darkMode ? "#263e47" : "#e4f3f7") : control.down || control.hovered ? (control.darkMode ? "#303339" : "#e8eaed") : "transparent"
        border.width: control.activeFocus ? 1 : 0
        border.color: "#08a5c8"
    }
}
