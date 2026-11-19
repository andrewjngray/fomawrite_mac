import QtQuick
import QtQuick.Controls

// An entry of the highlighter's menus (Cycle 163): a colour dot when it names
// a colour, then its text; ticked when it is the current choice. Not
// checkable, so a click never breaks the `ticked` binding.
CompactMenuItem {
    id: item
    property bool ticked: false
    property color swatch: "transparent"
    indicator: LineIcon {
        x: 7
        width: 16
        height: 16
        anchors.verticalCenter: parent.verticalCenter
        name: "check"
        visible: item.ticked
        ink: backend.palette.text
        opacity: item.enabled ? 1 : 0.45
    }
    contentItem: Row {
        spacing: 8
        Rectangle {
            visible: item.swatch.a > 0
            width: 12
            height: 12
            radius: 6
            anchors.verticalCenter: parent.verticalCenter
            color: item.swatch
            border.width: 1
            border.color: Qt.darker(item.swatch, 1.3)
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: item.text
            font: item.font
            color: backend.palette.text
            opacity: item.enabled ? 1 : 0.45
        }
    }
}
