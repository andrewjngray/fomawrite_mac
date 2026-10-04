import QtQuick
import QtQuick.Controls

// Labeled sibling of the header tools: same font, capsule and interaction states.
ToolbarButton {
    id: control
    property bool menuIndicator: false
    property bool alignLeft: menuIndicator
    implicitWidth: Math.max(34, labelMetrics.advanceWidth + 24
                            + (menuIndicator ? 18 : 0) + (iconName !== "" ? 24 : 0))
    leftPadding: 12
    rightPadding: 12
    TextMetrics { id: labelMetrics; text: control.text; font: control.font }
    contentItem: Item {
        opacity: control.enabled ? 1 : 0.45
        LineIcon {
            visible: control.iconName !== ""
            name: control.iconName; ink: control.iconColor
            width: control.iconSize; height: control.iconSize
            strokeWidth: control.iconStrokeWidth
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            objectName: "footerButtonLabel"
            text: control.text
            font: control.font
            color: control.iconColor
            anchors.verticalCenter: parent.verticalCenter
            x: control.iconName !== "" ? 24 : 0
            width: Math.max(0, parent.width - x - (control.menuIndicator ? 18 : 0))
            horizontalAlignment: control.alignLeft ? Text.AlignLeft : Text.AlignHCenter
            elide: Text.ElideRight
        }
        LineIcon {
            visible: control.menuIndicator
            name: "down"; ink: control.iconColor
            width: 12; height: 12
            anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
        }
    }
}
