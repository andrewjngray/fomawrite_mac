import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool darkMode: false
    property color iconColor: iconName === "folder" ? (backend.palette.folder) : "transparent"
    font.pixelSize: 14
    property string iconName: ""
    property bool alignLeft: false
    property string hint: text
    property int tooltipDelay: 700
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
    ToolTip.delay: control.tooltipDelay
    ToolTip.visible: hovered && hint !== ""
    ToolTip.text: hint
    contentItem: Item {
        readonly property color ink: !control.enabled ? "#96999e" : control.checked ? (backend.palette.text) : backend.palette.muted
        LineIcon {
            visible: control.iconName !== ""
            name: control.iconName
            ink: control.enabled && control.iconColor.a > 0 ? control.iconColor : parent.ink
            anchors.verticalCenter: parent.verticalCenter
            x: control.alignLeft ? 0 : control.text ? Math.max(0, (parent.width - label.implicitWidth - 24) / 2) : (parent.width - width) / 2
        }
        Text {
            id: label
            text: control.text
            x: control.iconName ? 24 : 0
            width: Math.max(0, parent.width - x)
            anchors.verticalCenter: parent.verticalCenter
            font.family: Qt.platform.os === "osx" ? Qt.application.font.family : "Helvetica Neue"
            font.pixelSize: control.font.pixelSize
            font.bold: control.font.bold
            color: parent.ink
            horizontalAlignment: control.alignLeft ? Text.AlignLeft : Text.AlignHCenter
            elide: Text.ElideRight
        }
    }
    background: Rectangle {
        radius: 8
        color: control.checked ? (backend.palette.hover) : control.down || control.hovered ? (backend.palette.hover) : "transparent"
        border.width: control.visualFocus ? 1 : 0
        border.color: backend.palette.focus
    }
}
