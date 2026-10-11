import QtQuick

// A single resting surface owns the segments; children use grouped: true.
Item {
    id: group
    default property alias content: contentRow.data
    property real horizontalPadding: 1
    property alias spacing: contentRow.spacing
    implicitWidth: contentRow.implicitWidth + horizontalPadding * 2
    implicitHeight: 34
    // Keep implementation children outside the public default content alias.
    data: [
        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: backend.palette.control || backend.palette.field
            border.color: backend.palette.toolbarBorder || backend.palette.border
            border.width: 1
        },
        // A disabled segment (an arrow at the end of its history) passes presses to
        // what lies beneath; this keeps them, and double-clicks, off the header's
        // window-drag and zoom area.
        MouseArea { anchors.fill: parent; acceptedButtons: Qt.LeftButton },
        Row {
            id: contentRow
            anchors.verticalCenter: parent.verticalCenter
            x: group.horizontalPadding
            spacing: 0
        }
    ]
}
