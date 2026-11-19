pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

// The PDF's table of contents in the pane's sidebar: its outline titles,
// indented by level. A click goes to the entry's page; the entry for the
// section the current page is in is marked and kept in view.
ListView {
    id: list
    objectName: "pdfContentsList"
    required property var reader
    model: visible ? reader.outlineEntries : []
    clip: true
    spacing: 1
    topMargin: 4
    bottomMargin: 12
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}

    function keepCurrentInView() {
        if (visible && reader.currentOutlineIndex >= 0 && reader.currentOutlineIndex < count)
            positionViewAtIndex(reader.currentOutlineIndex, ListView.Contain);
    }
    Connections {
        target: list.reader
        function onCurrentOutlineIndexChanged() { list.keepCurrentInView(); }
    }
    onVisibleChanged: keepTimer.restart()
    onCountChanged: keepTimer.restart()
    Timer { id: keepTimer; interval: 0; onTriggered: list.keepCurrentInView() }

    delegate: Rectangle {
        id: entry
        required property var modelData
        required property int index
        readonly property bool current: index === list.reader.currentOutlineIndex
        objectName: current ? "pdfContentsCurrent" : "pdfContentsEntry"
        x: 4
        width: list.width - 8
        height: 26
        radius: 5
        color: current ? (backend.palette.selectedRow || backend.palette.hover) : entryMouse.containsMouse ? (backend.palette.controlHover || backend.palette.hover) : "transparent"
        Accessible.role: Accessible.ListItem
        Accessible.name: modelData.title
        Accessible.selected: current
        Text {
            objectName: "pdfContentsTitle"
            x: 8 + Math.max(0, Number(entry.modelData.level) || 0) * 12
            width: parent.width - x - 6
            anchors.verticalCenter: parent.verticalCenter
            text: entry.modelData.title
            textFormat: Text.PlainText
            elide: Text.ElideRight
            font.family: Qt.application.font.family
            font.pixelSize: 12
            font.weight: entry.current ? Font.DemiBold : Font.Normal
            color: backend.palette.text
        }
        MouseArea {
            id: entryMouse
            objectName: "pdfContentsMouse"
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                if (entry.modelData.page < 0) return;
                list.reader.goToPage(entry.modelData.page);
                list.reader.focusPages();
            }
        }
    }
}
