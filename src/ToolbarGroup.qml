import QtQuick

// A single resting surface owns the segments; children use grouped: true.
Item {
    id: group
    default property alias content: contentRow.data
    property real horizontalPadding: 2
    property alias spacing: contentRow.spacing
    implicitWidth: contentRow.implicitWidth + horizontalPadding * 2
    implicitHeight: 32
    // Keep implementation children outside the public default content alias.
    data: [
        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: backend.palette.control || backend.palette.field
            border.color: backend.palette.border
            border.width: 1
        },
        Row {
            id: contentRow
            anchors.verticalCenter: parent.verticalCenter
            x: group.horizontalPadding
            spacing: 0
        }
    ]
}
