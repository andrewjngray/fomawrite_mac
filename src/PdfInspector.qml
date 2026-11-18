pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The PDF Inspector (Preview's General tab): what the file says about itself.
// Empty fields show "–", as Preview does.
Popup {
    id: root
    objectName: "pdfInspector"
    required property var reader
    readonly property var info: reader.session && reader.revision >= 0 ? reader.session.info : ({})
    readonly property size pagePoints: visible && reader.session && reader.ready ? reader.session.pageSize(reader.currentPage) : Qt.size(0, 0)
    function shown(value) { return value === undefined || value === null || String(value).trim() === "" ? "–" : String(value); }
    function date(value) {
        if (!value) return "–";
        var parsed = new Date(value);
        return isNaN(parsed.getTime()) ? String(value) : parsed.toLocaleString(Qt.locale(), Locale.ShortFormat);
    }
    function pageSizeText() {
        if (pagePoints.width <= 0) return "–";
        var cm = function(points) { return (points / 72 * 2.54).toFixed(2); };
        var inches = function(points) { return (points / 72).toFixed(2); };
        return cm(pagePoints.width) + " × " + cm(pagePoints.height) + " cm (" + inches(pagePoints.width) + " × " + inches(pagePoints.height) + " in)";
    }
    readonly property var rows: [
        { key: "fileName", label: "File name", value: shown(info.fileName) },
        { key: "kind", label: "Kind", value: "PDF document" },
        { key: "pdfVersion", label: "Version", value: shown(info.pdfVersion) },
        { key: "pageCount", label: "Pages", value: shown(info.pageCount) },
        { key: "pageSize", label: "Page size", value: pageSizeText() },
        { key: "title", label: "Title", value: shown(info.title) },
        { key: "author", label: "Author", value: shown(info.author) },
        { key: "subject", label: "Subject", value: shown(info.subject) },
        { key: "keywords", label: "Keywords", value: shown(info.keywords) },
        { key: "producer", label: "Producer", value: shown(info.producer) },
        { key: "creator", label: "Creator", value: shown(info.creator) },
        { key: "created", label: "Created", value: date(info.created) },
        { key: "modified", label: "Modified", value: date(info.modified) },
        { key: "encrypted", label: "Encrypted", value: info.encrypted ? "Yes" : "No" }
    ]
    function showFrom(anchor) {
        var point = anchor.mapToItem(reader, anchor.width, anchor.height + 6);
        x = Math.max(8, Math.min(reader.width - width - 8, point.x - width));
        y = Math.max(8, point.y);
        open();
    }
    parent: reader
    width: Math.min(360, reader.width - 16)
    padding: 14
    popupType: Popup.Item
    focus: true
    background: Rectangle {
        radius: 10
        color: backend.palette.popover || backend.palette.panel
        border.color: backend.palette.border
    }
    onClosed: reader.focusPages()
    // One Repeater over label/value pairs, so the grid interleaves them.
    readonly property var cells: {
        var cells = [];
        for (var i = 0; i < rows.length; ++i) {
            cells.push({ name: "pdfInspectorLabel_" + rows[i].key, text: rows[i].label, value: false });
            cells.push({ name: "pdfInspectorValue_" + rows[i].key, text: rows[i].value, value: true });
        }
        return cells;
    }
    contentItem: GridLayout {
        columns: 2
        columnSpacing: 12
        rowSpacing: 6
        Repeater {
            model: root.cells
            delegate: Label {
                required property var modelData
                objectName: modelData.name
                Layout.alignment: modelData.value ? Qt.AlignLeft | Qt.AlignTop : Qt.AlignRight | Qt.AlignTop
                Layout.fillWidth: modelData.value
                Layout.maximumWidth: modelData.value ? root.availableWidth - 100 : 100
                text: modelData.text
                textFormat: Text.PlainText
                wrapMode: modelData.value ? Text.WrapAnywhere : Text.NoWrap
                color: modelData.value ? backend.palette.text : backend.palette.muted
                font.pixelSize: 12
            }
        }
    }
}
