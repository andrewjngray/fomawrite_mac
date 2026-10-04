import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "LinkSyntax.js" as LinkSyntax

Popup {
    id: root
    objectName: "linkEditor"
    required property var hostWindow
    required property var sourceEditor
    required property var sourceViewport
    required property var documentBackend
    property bool editingExisting: false
    property string originalMarkdown: ""
    property string originalLabel: ""
    property string originalLabelSyntax: ""
    property string originalUrl: ""
    property string originalTitle: ""
    property string sourceSnapshot: ""
    property string fileSnapshot: ""
    property int documentGeneration: 0
    property int sessionGeneration: -1
    property int selectionStart: 0
    property int selectionEnd: 0
    property int savedSelectionStart: 0
    property int savedSelectionEnd: 0
    property int savedCursor: 0
    property real savedScrollY: 0
    property string error: ""
    property bool invalidated: false
    property bool applying: false
    property bool applied: false
    property bool restoreOnClose: false
    readonly property bool sessionValid: !invalidated && sessionGeneration === documentGeneration
        && fileSnapshot === String(documentBackend.fileUrl) && sourceSnapshot === sourceEditor.text
    width: Math.min(380, hostWindow.width - 32)
    padding: 18
    modal: false
    focus: true
    closePolicy: Popup.CloseOnPressOutside
    background: Rectangle { color: backend.palette.panel; border.color: backend.palette.border; radius: 12 }

    Connections {
        target: root.documentBackend
        function onDocumentLoaded() { ++root.documentGeneration; if (root.visible) root.invalidated = true; }
        function onFileUrlChanged() { if (root.visible && !root.applying) root.invalidated = true; }
    }
    Connections {
        target: root.sourceEditor
        function onTextChanged() {
            if (root.visible && !root.applying && root.sourceEditor.text !== root.sourceSnapshot) root.invalidated = true;
        }
    }
    function showFor(start, end, existing, point) {
        sourceSnapshot = sourceEditor.text;
        fileSnapshot = String(documentBackend.fileUrl);
        sessionGeneration = documentGeneration;
        savedSelectionStart = sourceEditor.selectionStart;
        savedSelectionEnd = sourceEditor.selectionEnd;
        savedCursor = sourceEditor.cursorPosition;
        savedScrollY = sourceViewport.contentY;
        selectionStart = existing ? existing.start : start;
        selectionEnd = existing ? existing.end : end;
        editingExisting = !!existing;
        originalMarkdown = existing ? existing.markdown : "";
        originalLabel = existing ? existing.label : "";
        originalLabelSyntax = existing ? existing.labelSyntax : "";
        originalUrl = existing ? existing.url : "";
        originalTitle = existing ? existing.title : "";
        linkEditorLabel.text = existing ? existing.label : sourceSnapshot.slice(start, end);
        linkEditorUrl.text = existing ? existing.url : (documentBackend.clipboardUrl() || "https://");
        linkEditorTitle.text = existing ? existing.title : "";
        error = ""; invalidated = false; applied = false; applying = false; restoreOnClose = false;
        x = Math.max(16, Math.min(hostWindow.width - width - 16, point.x - width));
        y = Math.max(16, Math.min(hostWindow.height - implicitHeight - 16, point.y + 6));
        open();
    }
    function cancelEditing() { restoreOnClose = true; close(); }
    function applyLink() {
        if (!sessionValid) {
            error = "The document changed while this link was open. Your entries are kept. Cancel and reopen the link to apply them.";
            return false;
        }
        var label = editingExisting ? linkEditorLabel.text : (linkEditorLabel.text || "link text");
        var rawDestination = linkEditorUrl.text;
        var title = linkEditorTitle.text;
        // A no-op must retain the original Markdown byte-for-byte, including
        // valid empty destinations, angle whitespace and title quote choice.
        if (editingExisting && label === originalLabel && rawDestination === originalUrl && title === originalTitle) {
            restoreOnClose = true; close(); return true;
        }
        var destination = editingExisting && rawDestination === originalUrl ? rawDestination : rawDestination.trim();
        if (!destination.length) { error = "A link destination is required."; linkEditorUrl.forceActiveFocus(); return false; }
        if (/[\x00-\x1f\x7f]/.test(destination + label + title)) {
            error = "Link text, destinations and titles must stay on one line without control characters.";
            return false;
        }
        var markdown = LinkSyntax.serialize(label, destination, title,
            editingExisting && label === originalLabel ? originalLabelSyntax : undefined);
        applying = true;
        var result = documentBackend.replaceText(selectionStart, selectionEnd, markdown);
        applying = false;
        if (result.start === undefined) { error = "This document is not ready for editing. Your entries are kept."; return false; }
        sourceEditor.cursorPosition = result.end;
        applied = true;
        restoreOnClose = true;
        close();
        return true;
    }
    onOpened: { linkEditorLabel.forceActiveFocus(); linkEditorLabel.selectAll(); }
    onClosed: {
        // Outside-click dismissal respects the clicked destination. Explicit
        // Cancel/Escape returns to the original writing selection when safe.
        if (!restoreOnClose || (!applied && !sessionValid) || !sourceEditor.visible) return;
        var wasApplied = applied, generation = documentGeneration;
        sourceEditor.forceActiveFocus();
        if (!wasApplied) {
            sourceEditor.select(savedCursor === savedSelectionStart ? savedSelectionEnd : savedSelectionStart, savedCursor);
            Qt.callLater(function() {
                if (root && generation === root.documentGeneration && root.sessionValid && root.sourceEditor.visible)
                    root.sourceViewport.contentY = root.savedScrollY;
            });
        }
    }
    Shortcut {
        sequence: "Escape"
        context: Qt.WindowShortcut
        enabled: root.visible
        onActivated: root.cancelEditing()
    }
    onImplicitHeightChanged: if (visible) y = Math.max(16, Math.min(hostWindow.height - implicitHeight - 16, y))

    component LinkField: TextField {
        id: field
        property string prompt: ""
        Layout.fillWidth: true
        Layout.preferredHeight: 36
        implicitHeight: 36
        font.family: Qt.application.font.family
        font.pixelSize: 13
        color: backend.palette.text
        leftPadding: 11; rightPadding: 11
        selectByMouse: true
        clip: true
        selectionColor: backend.palette.selection
        selectedTextColor: "white"
        placeholderText: ""
        Text {
            anchors.fill: parent; anchors.leftMargin: 11; anchors.rightMargin: 11
            verticalAlignment: Text.AlignVCenter
            text: field.prompt; visible: field.text.length === 0
            color: backend.palette.muted; font: field.font; elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 8; color: backend.palette.field
            border.width: field.activeFocus ? 2 : 1
            border.color: field.activeFocus ? backend.palette.focus : backend.palette.border
        }
        onAccepted: root.applyLink()
        Keys.onEscapePressed: function(event) { root.cancelEditing(); event.accepted = true; }
    }
    contentItem: ColumnLayout {
        spacing: 10
        Label {
            objectName: "linkEditorHeading"
            text: root.editingExisting ? "Edit link" : "Insert link"
            font.family: Qt.application.font.family; font.pixelSize: 17; font.bold: true
            color: backend.palette.text; Accessible.role: Accessible.Heading
        }
        Label { text: "Text"; color: backend.palette.muted; font.pixelSize: 12 }
        LinkField { id: linkEditorLabel; objectName: "linkEditorLabel"; prompt: "Link text"; Accessible.name: "Link text" }
        Label { text: "Destination"; color: backend.palette.muted; font.pixelSize: 12 }
        LinkField {
            id: linkEditorUrl; objectName: "linkEditorUrl"
            prompt: "https:// or a local path"; Accessible.name: "Link destination"
            inputMethodHints: Qt.ImhUrlCharactersOnly
        }
        Label { text: "Title (optional)"; color: backend.palette.muted; font.pixelSize: 12 }
        LinkField { id: linkEditorTitle; objectName: "linkEditorTitle"; prompt: "Shown by supporting readers"; Accessible.name: "Link title, optional" }
        Label {
            objectName: "linkEditorError"
            visible: root.error.length > 0; text: root.error
            Layout.fillWidth: true; wrapMode: Text.Wrap
            color: backend.palette.text; font.pixelSize: 12
            Accessible.name: text; Accessible.role: Accessible.AlertMessage
        }
        RowLayout {
            Layout.fillWidth: true; Layout.topMargin: 4; spacing: 8
            Item { Layout.fillWidth: true }
            SquareDialogButton { objectName: "linkEditorCancelButton"; text: "Cancel"; onClicked: root.cancelEditing() }
            SquareDialogButton {
                objectName: "linkEditorApplyButton"
                text: root.editingExisting ? "Save" : "Insert"
                primary: true; onClicked: root.applyLink()
            }
        }
    }
}
