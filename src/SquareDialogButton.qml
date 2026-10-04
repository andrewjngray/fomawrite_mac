import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool primary: false
    property bool darkMode: true
    // Use readable ink on both the dark accent and the light dark-theme accent.
    function linearChannel(value) {
        return value <= 0.04045 ? value / 12.92 : Math.pow((value + 0.055) / 1.055, 2.4)
    }
    readonly property real accentLuminance: 0.2126 * linearChannel(activeColor.r)
        + 0.7152 * linearChannel(activeColor.g) + 0.0722 * linearChannel(activeColor.b)
    property color labelColor: primary ? (accentLuminance > 0.179 ? "#000000" : "#ffffff") : backend.palette.text
    property color activeColor: backend.palette.focus
    property real textScale: 1

    implicitWidth: Math.max(88, label.implicitWidth + 32)
    implicitHeight: 36
    leftPadding: 16
    rightPadding: 16
    topPadding: 7
    bottomPadding: 7
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    focusPolicy: Qt.StrongFocus
    Accessible.name: text
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()

    contentItem: Label {
        id: label
        text: control.text
        color: control.labelColor
        opacity: control.enabled ? 1 : 0.45
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.family: Qt.application.font.family
        font.pixelSize: Math.round(13 * control.textScale)
    }
    background: Rectangle {
        radius: 6
        color: control.primary
            ? (control.down ? Qt.darker(control.activeColor, 1.18) : control.hovered && control.enabled ? Qt.lighter(control.activeColor, 1.12) : control.activeColor)
            : control.down ? (backend.palette.controlPressed || backend.palette.selectedRow)
            : control.hovered && control.enabled ? (backend.palette.controlHover || backend.palette.hover)
            : (backend.palette.control || backend.palette.field)
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? backend.palette.focus
            : control.primary ? control.activeColor : backend.palette.border
        opacity: control.enabled ? 1 : 0.55
    }
}
