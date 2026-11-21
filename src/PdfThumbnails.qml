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

    property int insertionLine: -1

    function boundaryAt(y) {
        for (var i = 0; i < list.contentItem.children.length; ++i) {
            var row = list.contentItem.children[i];
            if (row.objectName !== "pdfThumbnail" || y < row.y - list.spacing / 2 || y >= row.y + row.height + list.spacing / 2)
                continue;
            return y < row.y + row.height / 2 ? row.index : row.index + 1;
        }
        return y <= list.originY + list.topMargin ? 0 : list.count;
    }

    function dropBoundary(source, point) {
        if (!visible || showingContents) return -1;
        var pos = list.mapFromItem(source, point.x, point.y);
        if (pos.x < 0 || pos.x >= list.width || pos.y < 0 || pos.y >= list.height) return -1;
        return boundaryAt(pos.y + list.contentY);
    }

    Menu {
        id: pageEditMenu
        objectName: "pdfThumbnailPageMenu"
        MenuItem { text: "Rotate Left"; onTriggered: root.reader.rotateSelectedPages(-90) }
        MenuItem { text: "Rotate Right"; onTriggered: root.reader.rotateSelectedPages(90) }
        MenuItem { text: "Delete Selected Pages"; enabled: root.reader.canRemoveSelectedPages; onTriggered: root.reader.removeSelectedPages() }
        MenuSeparator {}
        MenuItem { text: "Insert Pages from File…"; onTriggered: root.reader.choosePagesToInsert() }
        MenuItem { text: "Export Selected Pages…"; onTriggered: root.reader.choosePageExport() }
    }

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
        interactive: root.insertionLine < 0
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}
        onCountChanged: keepTimer.restart()
        delegate: Item {
            id: thumb
            required property int index
            readonly property size points: root.reader.session && root.reader.revision >= 0 ? root.reader.session.pageSize(index) : Qt.size(612, 792)
            readonly property bool current: index === root.reader.currentPage
            readonly property bool selected: root.reader.pageTargets.indexOf(index) >= 0
            Accessible.role: Accessible.ListItem
            Accessible.name: "Page " + (index + 1)
            Accessible.selected: selected
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
                color: thumb.selected ? backend.palette.selectedRow : "transparent"
                border.width: thumb.selected ? 3 : 1
                border.color: thumb.selected ? backend.palette.focus : backend.palette.border
            }
            Rectangle {
                visible: root.insertionLine === thumb.index || (thumb.index === list.count - 1 && root.insertionLine === list.count)
                y: root.insertionLine === list.count ? thumb.height - height : -height
                x: 10
                width: parent.width - 20
                height: 3
                color: backend.palette.focus
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
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                preventStealing: true
                property real pressY: 0
                property bool reordered: false
                onPressed: function(mouse) { pressY = mouse.y; reordered = false; }
                onPositionChanged: function(mouse) {
                    if (!(pressedButtons & Qt.LeftButton) || Math.abs(mouse.y - pressY) < 8 && root.insertionLine < 0) return;
                    if (!thumb.selected) root.reader.selectThumbnail(thumb.index, Qt.NoModifier);
                    var point = list.contentItem.mapFromItem(thumb, mouse.x, mouse.y);
                    root.insertionLine = root.boundaryAt(point.y);
                }
                onReleased: function(mouse) {
                    if (mouse.button !== Qt.LeftButton || root.insertionLine < 0) return;
                    reordered = true;
                    var before = root.insertionLine;
                    root.insertionLine = -1;
                    root.reader.moveSelectedPages(before);
                    root.reader.focusPages();
                }
                onCanceled: root.insertionLine = -1
                onClicked: function(mouse) {
                    if (reordered) return;
                    if (mouse.button === Qt.RightButton) {
                        if (!thumb.selected) root.reader.selectThumbnail(thumb.index, Qt.NoModifier);
                        pageEditMenu.popup(root, root.mapFromItem(thumb, mouse.x, mouse.y).x,
                                          root.mapFromItem(thumb, mouse.x, mouse.y).y);
                    } else {
                        root.reader.selectThumbnail(thumb.index, mouse.modifiers);
                        root.reader.focusPages();
                    }
                }
            }
        }
    }
}
