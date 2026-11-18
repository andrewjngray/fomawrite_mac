pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Fomawrite 1.0

// Page thumbnails at the left edge of the PDF pane. Each is a PdfPage that
// renders only while on screen, so the column stays as memory-bounded as the
// pages themselves; the current page is marked and kept in view.
Rectangle {
    id: root
    objectName: "pdfThumbnails"
    required property var reader
    readonly property real thumbWidth: 96
    readonly property real thumbScale: thumbWidth / Math.max(1, reader.largestPage.width)
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

    ListView {
        id: list
        objectName: "pdfThumbnailList"
        anchors.fill: parent
        anchors.rightMargin: 1
        model: root.visible ? root.reader.pageCount : 0
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
