import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool darkMode: false
    property string iconName: ""
    property bool alignLeft: false
    property string hint: text
    implicitHeight: 28
    implicitWidth: Math.max(28, label.implicitWidth + (iconName ? 18 + (text ? 6 : 0) : 0) + 16)
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
    ToolTip.delay: 700
    ToolTip.visible: hovered && hint !== ""
    ToolTip.text: hint
    contentItem: Item {
        readonly property color ink: !control.enabled ? "#96999e" : control.checked ? (control.darkMode ? "#8eb5f0" : "#244f88") : control.darkMode ? "#d5d8dd" : "#40444b"
        LineIcon {
            visible: control.iconName !== ""
            name: control.iconName
            ink: parent.ink
            anchors.verticalCenter: parent.verticalCenter
            x: control.alignLeft ? 0 : control.text ? Math.max(0, (parent.width - label.implicitWidth - 24) / 2) : (parent.width - width) / 2
        }
        Text {
            id: label
            text: control.text
            x: control.iconName ? 24 : 0
            width: Math.max(0, parent.width - x)
            anchors.verticalCenter: parent.verticalCenter
            font.family: "Helvetica Neue"
            font.pixelSize: 12
            font.bold: control.checked
            color: parent.ink
            horizontalAlignment: control.alignLeft ? Text.AlignLeft : Text.AlignHCenter
            elide: Text.ElideRight
        }
    }
    background: Rectangle {
        radius: 12
        color: control.checked ? (control.darkMode ? "#293c57" : "#e5ecf7") : control.down || control.hovered ? (control.darkMode ? "#303339" : "#e8eaed") : "transparent"
        border.width: control.activeFocus ? 1 : 0
        border.color: "#426da7"
    }
}
