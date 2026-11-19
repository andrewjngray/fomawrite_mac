pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

// The unplaced list (Cycle 164): marks and notes in the notes file that cannot
// be drawn on the page, one row each (kind, page, the start of the quote or
// note). A row shows its item in the notes editor. Styled as the Inspector.
Popup {
    id: root
    objectName: "pdfUnplacedList"
    required property var reader
    readonly property var items: reader.unplacedItems
    function kindName(kind) { return kind === "strike" ? "Strikethrough" : reader.markVerb(kind); }
    function start(item) {
        var text = (item.kind === "note" ? item.text : item.quote) || "";
        text = text.replace(/\s+/g, " ").trim();
        return text.length > 60 ? text.slice(0, 60).trim() + "…" : text;
    }
    function showFrom(anchor) {
        var point = anchor.mapToItem(reader, anchor.width, anchor.height + 6);
        x = Math.max(8, Math.min(reader.width - width - 8, point.x - width));
        y = Math.max(8, point.y);
        open();
    }
    onItemsChanged: if (items.length === 0) close()
    parent: reader
    width: Math.min(360, reader.width - 16)
    padding: 8
    popupType: Popup.Item
    focus: true
    background: Rectangle {
        radius: 10
        color: backend.palette.popover || backend.palette.panel
        border.color: backend.palette.border
    }
    onClosed: reader.focusPages()
    contentItem: Column {
        spacing: 2
        Label {
            width: parent.width
            leftPadding: 6
            rightPadding: 6
            bottomPadding: 4
            text: "Not shown on the page. They stay in the notes."
            wrapMode: Text.Wrap
            color: backend.palette.muted
            font.pixelSize: 12
        }
        Repeater {
            model: root.items
            delegate: Rectangle {
                id: row
                required property var modelData
                objectName: "pdfUnplacedRow"
                readonly property int itemIndex: modelData.index
                readonly property string kindText: root.kindName(modelData.kind)
                readonly property string pageText: modelData.page >= 0 ? "p. " + (modelData.page + 1) : "no page"
                readonly property string startText: root.start(modelData)
                width: root.availableWidth
                height: Math.max(32, line.implicitHeight + 12)
                radius: 6
                color: area.containsMouse ? (backend.palette.hover || backend.palette.selectedRow) : "transparent"
                Accessible.role: Accessible.Button
                Accessible.name: kindText + ", " + pageText + (startText !== "" ? ", " + startText : "")
                Label {
                    id: line
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    verticalAlignment: Text.AlignVCenter
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    text: row.kindText + " · " + row.pageText + (row.startText !== "" ? "  " + row.startText : "")
                    color: backend.palette.text
                    font.pixelSize: 12
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { root.close(); root.reader.revealUnplaced(row.itemIndex); }
                }
            }
        }
    }
}
