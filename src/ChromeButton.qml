import QtQuick
import QtQuick.Controls

// Text/list actions retain their rounded-row geometry; header actions use ToolbarButton.
Button {
    id: control
    property bool darkMode: false
    property bool tonal: false
    property color iconColor: iconName === "folder" ? backend.palette.folder : "transparent"
    property string iconName: ""
    property bool alignLeft: false
    property string hint: text
    property int tooltipDelay: 700
    font.family: Qt.application.font.family
    font.pixelSize: 13
    implicitHeight: 28
    implicitWidth: Math.max(28, label.implicitWidth + (iconName ? 16 + (text ? 8 : 0) : 0) + 16)
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
    property bool tooltipReady: false
    onHoveredChanged: { tooltipReady = false; if (hovered) hoverDelay.restart(); else hoverDelay.stop() }
    onHintChanged: { tooltipReady = false; if (hovered) hoverDelay.restart() }
    Timer { id: hoverDelay; interval: control.tooltipDelay; onTriggered: control.tooltipReady = control.hovered }
    ToolTip.delay: 0
    ToolTip.visible: hovered && tooltipReady && hint !== ""
    ToolTip.text: hint
    contentItem: Item {
        readonly property color ink: control.checked || control.alignLeft ? backend.palette.text : backend.palette.muted
        opacity: control.enabled ? 1 : 0.45
        LineIcon {
            visible: control.iconName !== ""
            name: control.iconName
            ink: control.iconColor.a > 0 ? control.iconColor : parent.ink
            width: 16
            height: 16
            anchors.verticalCenter: parent.verticalCenter
            x: control.alignLeft ? 0 : control.text ? Math.max(0, (parent.width - label.implicitWidth - 24) / 2) : (parent.width - width) / 2
        }
        Text {
            id: label
            text: control.text
            x: control.iconName ? 24 : 0
            width: Math.max(0, parent.width - x)
            anchors.verticalCenter: parent.verticalCenter
            font: control.font
            color: parent.ink
            horizontalAlignment: control.alignLeft ? Text.AlignLeft : Text.AlignHCenter
            elide: Text.ElideRight
        }
    }
    background: Rectangle {
        radius: 6
        color: control.down ? (backend.palette.controlPressed || backend.palette.selectedRow)
            : control.checked ? backend.palette.selectedRow
            : control.hovered && control.enabled ? backend.palette.hover
            : control.tonal ? (backend.palette.control || backend.palette.field) : "transparent"
        border.width: control.activeFocus ? 2 : control.tonal ? 1 : 0
        border.color: control.activeFocus ? backend.palette.focus : backend.palette.border
    }
}
