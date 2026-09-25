import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    objectName: "previewPane"
    property string visualEditorObjectName: "visualEditor"
    property bool allowVisualEdit: true
    required property var renderer
    property string markdown: ""
    property url documentBaseUrl
    property bool darkMode: false
    property string typeface: "Helvetica Neue"
    property int textSize: 17
    property int layoutMode: 1
    property string pendingAnchor: ""
    property bool visualEditEnabled: false
    signal editorUndoRequested()
    signal editorRedoRequested()
    signal sourceEditRequested()
    property bool visualEditorFocused: visualText.activeFocus
    property string visualStatus: ""
    property string visualSnapshot: ""
    property string visualSourceSnapshot: ""
    property bool synchronizingVisualText: false
    signal scrollFractionChanged(real fraction)
    function scrollToFraction(fraction) { previewScroll.contentY = Math.max(0, previewScroll.contentHeight - previewScroll.height) * fraction; }
    function jumpToAnchor(anchor) {
        var position = renderer.previewAnchorPosition(previewText.textDocument, decodeURIComponent(anchor));
        if (position < 0) return false;
        previewScroll.contentY = Math.max(0, Math.min(previewScroll.contentHeight - previewScroll.height, previewText.positionToRectangle(position).y));
        return true;
    }
    function navigateToAnchor(anchor) {
        pendingAnchor = anchor;
        reload();
    }
    signal layoutRequested(int mode)
    signal linkRequested(url link)
    color: backend.palette.page
    property string renderedMarkdown: ""
    onMarkdownChanged: {
        refreshTimer.restart()
        if (visualEditEnabled)
            visualRefreshTimer.restart()
    }
    onTextSizeChanged: { refreshTimer.restart(); if (visualEditEnabled) visualRefreshTimer.restart(); }
    onTypefaceChanged: { refreshTimer.restart(); if (visualEditEnabled) visualRefreshTimer.restart(); }
    onDarkModeChanged: { refreshTimer.restart(); if (visualEditEnabled) visualRefreshTimer.restart(); }
    function refresh() {
        // Reparse when font size/theme changes too: normalized HTML contains
        // explicit formatting from the previous preview pass.
        renderedMarkdown = "";
        Qt.callLater(function() {
            renderedMarkdown = renderer.previewMarkdown(markdown);
            Qt.callLater(function() {
                root.renderer.stylePreview(previewText.textDocument);
                if (root.pendingAnchor !== "") {
                    var anchor = root.pendingAnchor;
                    root.pendingAnchor = "";
                    root.jumpToAnchor(anchor);
                }
            });
        });
    }
    function reload() {
        // Force a fresh Markdown parse even when the source has not changed.
        refreshTimer.stop();
        renderedMarkdown = "";
        Qt.callLater(refresh);
    }
    function loadVisualProjection(selectionStart, selectionEnd) {
        if (!visualEditEnabled)
            return;
        // Keep the field's own post-edit selection across a canonical refresh.
        // This is also bounded for source/external changes that shorten text.
        var first = selectionStart === undefined ? visualText.selectionStart : selectionStart;
        var last = selectionEnd === undefined ? visualText.selectionEnd : selectionEnd;
        var projection = renderer.visualProjection();
        synchronizingVisualText = true;
        visualSourceSnapshot = projection.source || "";
        visualSnapshot = projection.visualText || "";
        visualText.text = visualSnapshot;
        Qt.callLater(function() { if (root.visualEditEnabled) root.renderer.styleVisualEditor(visualText.textDocument, root.textSize); });
        first = Math.max(0, Math.min(first, visualSnapshot.length));
        last = Math.max(0, Math.min(last, visualSnapshot.length));
        if (first === last)
            visualText.cursorPosition = last;
        else
            visualText.select(first, last);
        synchronizingVisualText = false;
    }
    function visualDiff(before, after) {
        var prefix = 0;
        var maximumPrefix = Math.min(before.length, after.length);
        while (prefix < maximumPrefix && before.charAt(prefix) === after.charAt(prefix))
            ++prefix;
        var beforeEnd = before.length;
        var afterEnd = after.length;
        while (beforeEnd > prefix && afterEnd > prefix
               && before.charAt(beforeEnd - 1) === after.charAt(afterEnd - 1)) {
            --beforeEnd;
            --afterEnd;
        }
        return { start: prefix, length: beforeEnd - prefix,
                 replacement: after.slice(prefix, afterEnd) };
    }
    function applyVisualTextChange() {
        if (synchronizingVisualText || visualText.inputMethodComposing
                || visualText.text === visualSnapshot)
            return;
        var change = visualDiff(visualSnapshot, visualText.text);
        if (change.replacement.indexOf("\n") >= 0 || change.replacement.indexOf("\r") >= 0) {
            visualStatus = "Visual editing supports inline changes only. Use Source for line breaks.";
            loadVisualProjection(change.start, change.start);
            return;
        }
        if (renderer.applyVisualEdit(change.start, change.length, change.replacement,
                                     visualSourceSnapshot)) {
            visualStatus = "Applied to Markdown source.";
            var first = visualText.selectionStart;
            var last = visualText.selectionEnd;
            root.loadVisualProjection(first, last);
        } else {
            visualStatus = "That range is source-only or changed elsewhere. Edit it in Source.";
            loadVisualProjection(change.start, change.start);
        }
    }
    onVisualEditEnabledChanged: {
        visualStatus = visualEditEnabled
            ? "Visual editing is limited to inline text in supported blocks."
            : "Rendered preview is read-only.";
        if (visualEditEnabled)
            Qt.callLater(loadVisualProjection);
    }
    Connections { target: root.renderer; function onOutputStyleChanged() { root.reload(); } }
    Component.onCompleted: refresh()

    Timer { id: refreshTimer; interval: 120; onTriggered: root.refresh() }
    Timer { id: visualRefreshTimer; interval: 80; onTriggered: root.loadVisualProjection() }
    Flickable {
        id: previewScroll
        objectName: "previewScroll"
        anchors.fill: parent
        anchors.bottomMargin: 34
        clip: true
        contentWidth: width
        contentHeight: Math.max(height, Math.max(previewText.implicitHeight, visualText.implicitHeight) + 100)
        boundsBehavior: Flickable.StopAtBounds
        onContentYChanged: root.scrollFractionChanged(contentY / Math.max(1, contentHeight - height))
        ScrollBar.vertical: ScrollBar {}
        TextEdit {
            id: previewText
            objectName: "renderedPreview"
            x: Math.max(22, (previewScroll.width - 740) / 2)
            y: 10
            width: Math.max(100, Math.min(740, previewScroll.width - 44))
            height: implicitHeight
            readOnly: true
            selectByMouse: true
            textFormat: TextEdit.MarkdownText
            text: root.renderedMarkdown
            baseUrl: root.documentBaseUrl
            wrapMode: TextEdit.Wrap
            font.family: root.typeface
            font.pixelSize: root.textSize
            color: backend.palette.text
            selectionColor: backend.palette.selection
            selectedTextColor: "white"
            onLinkActivated: function(link) { if (String(link).charAt(0) === "#") root.jumpToAnchor(String(link).slice(1)); else root.linkRequested(link); }
            Accessible.name: "Rendered Markdown preview"
            visible: !root.visualEditEnabled
        }
        TextEdit {
            id: visualText
            objectName: root.visualEditorObjectName
            visible: root.visualEditEnabled
            x: previewText.x
            y: previewText.y
            width: previewText.width
            height: implicitHeight
            textFormat: TextEdit.PlainText
            wrapMode: TextEdit.Wrap
            selectByMouse: true
            persistentSelection: true
            activeFocusOnPress: true
            color: backend.palette.text
            selectionColor: backend.palette.selection
            selectedTextColor: "white"
            font.family: root.typeface
            font.pixelSize: root.textSize
            onTextChanged: root.applyVisualTextChange()
            onInputMethodComposingChanged: if (!inputMethodComposing) root.applyVisualTextChange()
            Keys.priority: Keys.BeforeItem
            Keys.onPressed: function(event) {
                if (!(event.modifiers & Qt.ControlModifier))
                    return;
                if (event.key === Qt.Key_Y
                        || (event.key === Qt.Key_Z && (event.modifiers & Qt.ShiftModifier))) {
                    root.editorRedoRequested();
                    event.accepted = true;
                } else if (event.key === Qt.Key_Z) {
                    root.editorUndoRequested();
                    event.accepted = true;
                }
            }
            Keys.onReturnPressed: function(event) {
                root.visualStatus = "Visual editing supports inline changes only. Use Source for line breaks.";
                event.accepted = true;
            }
            Keys.onEnterPressed: function(event) {
                root.visualStatus = "Visual editing supports inline changes only. Use Source for line breaks.";
                event.accepted = true;
            }
            Accessible.name: "Visual Markdown editor"
            Accessible.description: "Edits supported inline text and keeps Markdown source canonical"
        }
        Label {
            anchors.centerIn: parent
            visible: root.markdown.length === 0 && !root.visualEditEnabled
            text: "Your words, beautifully read.\nStart writing to see a live preview."
            horizontalAlignment: Text.AlignHCenter
            color: backend.palette.muted
            font.pixelSize: 15
            lineHeight: 1.5
        }
    }
    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 34; color: backend.palette.panel
        Rectangle { width: parent.width; height: 1; color: backend.palette.border }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
            Label {
                text: root.visualEditEnabled ? root.visualStatus : root.renderer.outputTemplateName
                font.pixelSize: 11
                color: backend.palette.muted
                elide: Text.ElideRight
                Layout.maximumWidth: Math.max(120, parent.width - 270)
            }
            Item { Layout.fillWidth: true }
            ChromeButton {
                objectName: "visualEditToggle"
                visible: root.allowVisualEdit
                text: "Visual Edit"
                hint: "Edit supported rendered text"
                darkMode: root.darkMode
                checkable: true
                checked: root.visualEditEnabled
                onClicked: root.visualEditEnabled = !root.visualEditEnabled
            }
            ChromeButton {
                objectName: "visualEditSourceButton"
                visible: root.visualEditEnabled
                text: "Source"
                hint: "Edit Markdown source"
                darkMode: root.darkMode
                onClicked: root.sourceEditRequested()
            }
            ChromeButton { text: "Split"; hint: "Split layout"; darkMode: root.darkMode; checked: root.layoutMode === 1; onClicked: root.layoutRequested(1) }
            ChromeButton { text: "Full"; hint: "Preview layout"; darkMode: root.darkMode; checked: root.layoutMode === 2; onClicked: root.layoutRequested(2) }
        }
    }

}
