pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Fomawrite 1.0

// The PDF pane's sidebar: page thumbnails, or (when the PDF has an outline)
// its table of contents, chosen with the Thumbnails | Contents switch at the
// top as in Preview. Each thumbnail is a PdfPage that renders only while on
// screen, so the column stays as memory-bounded as the pages themselves; the
// current page (and the current section) is marked and kept in view.
Rectangle {
    id: root
    objectName: "pdfThumbnails"
    required property var reader
    readonly property real thumbWidth: 96
    readonly property real thumbScale: thumbWidth / Math.max(1, reader.largestPage.width)
    readonly property bool showingContents: reader.sidebarView === "contents"
    color: backend.palette.panel
    clip: true

    function keepCurrentInView() {
        if (visible && list.count > 0) list.positionViewAtIndex(reader.currentPage, ListView.Contain);
    }
    Connections {
        target: root.reader
        function onCurrentPageChanged() { root.keepCurrentInView(); }
    }
    onVisibleChanged: keepTimer.restart()
    Timer { id: keepTimer; interval: 0; onTriggered: root.keepCurrentInView() }

    Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: backend.palette.border }

    Rectangle {
        id: switcher
        objectName: "pdfSidebarSwitch"
        visible: root.reader.hasOutline
        x: 10
        y: 8
        width: parent.width - 21
        height: visible ? 26 : 0
        radius: 7
        color: backend.palette.control || backend.palette.field
        border.width: 1
        border.color: backend.palette.toolbarBorder || backend.palette.border
        Row {
            anchors.fill: parent
            anchors.margins: 2
            Repeater {
                model: [{ view: "thumbnails", text: "Thumbnails" }, { view: "contents", text: "Contents" }]
                Rectangle {
                    id: tab
                    required property var modelData
                    readonly property bool selected: root.reader.sidebarView === modelData.view
                    objectName: modelData.view === "contents" ? "pdfSidebarContentsTab" : "pdfSidebarThumbnailsTab"
                    width: switcher.width / 2 - 2
                    height: switcher.height - 4
                    radius: 5
                    color: selected ? (backend.palette.controlSelected || backend.palette.selectedRow) : tabMouse.containsMouse ? (backend.palette.controlHover || backend.palette.hover) : "transparent"
                    Accessible.role: Accessible.PageTab
                    Accessible.name: modelData.text
                    Accessible.checked: selected
                    Text {
                        anchors.centerIn: parent
                        width: parent.width - 6
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideRight
                        text: tab.modelData.text
                        font.family: Qt.application.font.family
                        font.pixelSize: 12
                        font.weight: tab.selected ? Font.DemiBold : Font.Normal
                        color: backend.palette.text
                    }
                    MouseArea {
                        id: tabMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.reader.showSidebarView(tab.modelData.view)
                    }
                }
            }
        }
    }

    PdfContents {
        id: contents
        reader: root.reader
        visible: root.showingContents
        anchors.top: switcher.bottom
        anchors.topMargin: switcher.visible ? 6 : 0
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 1
    }

    ListView {
        id: list
        objectName: "pdfThumbnailList"
        visible: !root.showingContents
        anchors.top: switcher.bottom
        anchors.topMargin: switcher.visible ? 2 : 0
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 1
        model: root.visible && !root.showingContents ? root.reader.pageCount : 0
        spacing: 8
        topMargin: 12
        bottomMargin: 12
        cacheBuffer: 0
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}
        onCountChanged: keepTimer.restart()
        delegate: Item {
            id: thumb
            required property int index
            readonly property size points: root.reader.session && root.reader.revision >= 0 ? root.reader.session.pageSize(index) : Qt.size(612, 792)
            readonly property bool current: index === root.reader.currentPage
            objectName: "pdfThumbnail"
            width: list.width
            height: preview.height + 30
            Rectangle {
                objectName: thumb.current ? "pdfThumbnailCurrent" : "pdfThumbnailFrame"
                anchors.horizontalCenter: preview.horizontalCenter
                y: preview.y - 4
                width: preview.width + 8
                height: preview.height + 8
                radius: 4
                color: "transparent"
                border.width: thumb.current ? 3 : 1
                border.color: thumb.current ? backend.palette.focus : backend.palette.border
            }
            PdfPage {
                id: preview
                objectName: "pdfThumbnailPage"
                session: root.reader.session
                page: thumb.index
                x: (thumb.width - width) / 2
                y: 4
                width: Math.max(1, thumb.points.width * root.thumbScale)
                height: Math.max(1, thumb.points.height * root.thumbScale)
                wanted: thumb.y + thumb.height > list.contentY && thumb.y < list.contentY + list.height
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                y: preview.y + preview.height + 6
                text: root.reader.session && root.reader.revision >= 0 ? root.reader.session.pageLabel(thumb.index) : ""
                font.pixelSize: 11
                font.weight: thumb.current ? Font.DemiBold : Font.Normal
                color: thumb.current ? backend.palette.text : backend.palette.muted
            }
            MouseArea {
                objectName: "pdfThumbnailMouse"
                anchors.fill: parent
                onClicked: { root.reader.goToPage(thumb.index); root.reader.focusPages(); }
            }
        }
    }
}
