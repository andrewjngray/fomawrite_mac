import QtQuick
import QtQuick.Controls
import QtQuick.Window

// Header actions share a 32px target. Groups supply the resting capsule.
Button {
    id: control
    property string iconName: ""
    property string hint: text
    property bool grouped: false
    readonly property var segments: grouped && parent ? Array.prototype.filter.call(parent.children, function(child) { return child.visible && child.grouped === true }) : []
    readonly property bool firstSegment: grouped && segments.length > 0 && segments[0] === control
    readonly property bool lastSegment: grouped && segments.length > 0 && segments[segments.length - 1] === control
    property bool darkMode: false
    property color iconColor: backend.palette.text
    property int tooltipDelay: 700
    readonly property bool windowActive: !Window.window || Window.window.active
    readonly property bool focusIndicated: activeFocus && windowActive
    readonly property color restColor: backend.palette.control || backend.palette.field
    readonly property color hoverColor: backend.palette.controlHover || backend.palette.hover
    readonly property color pressedColor: backend.palette.controlPressed || backend.palette.selectedRow
    readonly property color selectedColor: backend.palette.controlSelected || backend.palette.selectedRow

    implicitWidth: grouped ? 30 : Math.max(32, label.implicitWidth + (iconName !== "" && text !== "" ? 22 : 0) + 16)
    implicitHeight: grouped ? 30 : 32
    padding: 0
    leftPadding: 8
    rightPadding: 8
    leftInset: 0
    rightInset: 0
    topInset: 0
    bottomInset: 0
    focusPolicy: Qt.StrongFocus
    font.family: Qt.application.font.family
    font.pixelSize: 13
    font.weight: Font.Normal
    Accessible.name: hint
    property bool tooltipReady: false
    onHoveredChanged: {
        tooltipReady = false
        if (hovered) hoverDelay.restart()
        else hoverDelay.stop()
    }
    onHintChanged: { tooltipReady = false; if (hovered) hoverDelay.restart() }
    Timer { id: hoverDelay; interval: control.tooltipDelay; onTriggered: control.tooltipReady = control.hovered }
    ToolTip.visible: hovered && tooltipReady && hint !== ""
    ToolTip.delay: 0
    ToolTip.text: hint

    contentItem: Item {
        opacity: control.enabled ? 1 : 0.45
        LineIcon {
            visible: control.iconName !== ""
            name: control.iconName
            ink: control.iconColor
            width: 16
            height: 16
            anchors.verticalCenter: parent.verticalCenter
            x: control.text !== "" ? 0 : (parent.width - width) / 2
        }
        Text {
            id: label
            visible: control.text !== ""
            text: control.text
            font: control.font
            color: backend.palette.text
            anchors.verticalCenter: parent.verticalCenter
            x: control.iconName !== "" ? 22 : 0
            width: Math.max(0, parent.width - x)
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }
    }
    background: Rectangle {
        radius: 0
        topLeftRadius: !control.grouped || control.firstSegment ? height / 2 : 0
        bottomLeftRadius: topLeftRadius
        topRightRadius: !control.grouped || control.lastSegment ? height / 2 : 0
        bottomRightRadius: topRightRadius
        color: control.down ? control.pressedColor
            : control.checked ? (control.windowActive ? control.selectedColor : backend.palette.selectedRow)
            : control.hovered && control.enabled ? control.hoverColor
            : control.grouped ? "transparent" : control.restColor
        border.width: control.focusIndicated ? 2 : control.grouped ? 0 : 1
        border.color: control.focusIndicated ? backend.palette.focus : backend.palette.border
        opacity: control.enabled ? 1 : 0.55
        Rectangle {
            objectName: "toolbarSegmentDivider"
            visible: control.grouped && !control.firstSegment
            width: 1
            height: parent.height
            color: backend.palette.border
        }
    }
}
