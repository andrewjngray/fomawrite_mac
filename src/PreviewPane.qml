import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    objectName: "previewPane"
    property string visualEditorObjectName: "visualEditor"
    property bool allowVisualEdit: true
    property bool showFooter: true
    property bool tonalLayoutButtons: false
    required property var renderer
    property string markdown: ""
    property url documentBaseUrl
    property bool darkMode: false
    property string typeface: "Helvetica Neue"
    property string visualTypeface: typeface
    property int visualTopInset: 32
    property int visualSideInset: 24
    property int textSize: 17
    // Editing should remain legible even when a compact output style is selected.
    property int visualTextSize: Math.max(18, textSize)
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
    function focusVisualEditor() { visualText.forceActiveFocus(); }
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
    onVisualTextSizeChanged: if (visualEditEnabled) visualRefreshTimer.restart()
    onVisualTypefaceChanged: if (visualEditEnabled) visualRefreshTimer.restart()
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
        Qt.callLater(function() { if (root.visualEditEnabled) root.renderer.styleVisualEditor(visualText.textDocument, root.visualTextSize, root.visualTypeface); });
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
    function insertVisualBreak(shift) {
        var position = visualText.cursorPosition;
        var inserted = shift ? "\n" : "\n\n";
        if (visualText.selectionStart === visualText.selectionEnd
                && root.renderer.applyVisualEdit(position, 0, inserted, root.visualSourceSnapshot)) {
            root.visualStatus = "Paragraph break applied to Markdown source.";
            root.loadVisualProjection(position + inserted.length, position + inserted.length);
        } else {
            root.visualStatus = "This break needs Source editing to preserve Markdown.";
        }
    }
    onVisualEditEnabledChanged: {
        visualStatus = visualEditEnabled
            ? "Visual editing supports text and simple paragraph breaks. Other structures remain source-only."
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
        anchors.bottomMargin: root.showFooter ? 48 : 0
        clip: true
        contentWidth: width
        contentHeight: Math.max(height, root.visualEditEnabled
            ? visualText.y + visualText.implicitHeight + 64
            : Math.max(previewText.implicitHeight, visualText.implicitHeight) + 100)
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
            Accessible.name: "Preview: read-only rendered Markdown"
            visible: !root.visualEditEnabled
        }
        TextEdit {
            id: visualText
            objectName: root.visualEditorObjectName
            visible: root.visualEditEnabled
            x: Math.max(root.visualSideInset, (previewScroll.width - 720) / 2)
            y: root.visualTopInset
            width: Math.max(100, Math.min(720, previewScroll.width - root.visualSideInset * 2))
            height: implicitHeight
            textFormat: TextEdit.PlainText
            wrapMode: TextEdit.Wrap
            selectByMouse: true
            persistentSelection: true
            activeFocusOnPress: true
            color: backend.palette.text
            selectionColor: backend.palette.selection
            selectedTextColor: "white"
            font.family: root.visualTypeface
            font.pixelSize: root.visualTextSize
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
                root.insertVisualBreak(event.modifiers & Qt.ShiftModifier);
                event.accepted = true;
            }
            Keys.onEnterPressed: function(event) {
                root.insertVisualBreak(event.modifiers & Qt.ShiftModifier);
                event.accepted = true;
            }
            Accessible.name: "Visual Edit: editable Markdown text"
            Accessible.description: "Edits supported text and simple paragraph breaks while keeping Markdown source canonical"
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
    component FooterButton: ChromeButton {
        id: footerButton
        tonal: true
        implicitHeight: 32
        // Measure unelided text independently of the width assigned by the
        // layout. A control's fitted contentItem width is not its text width.
        implicitWidth: Math.max(text === "Visual Edit" ? 92 : text === "Source" ? 68 : 52,
                                footerLabelMetrics.width + 16)
        Layout.minimumWidth: implicitWidth
        Layout.preferredWidth: implicitWidth
        Layout.maximumWidth: implicitWidth
        Layout.minimumHeight: 32
        font.pixelSize: 12
        TextMetrics {
            id: footerLabelMetrics
            text: footerButton.text
            font: footerButton.font
        }
        contentItem: Text {
            id: footerLabel
            text: footerButton.text
            font: footerButton.font
            color: backend.palette.text
            opacity: footerButton.enabled ? 1 : 0.45
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 6
            color: footerButton.down ? backend.palette.controlPressed
                : footerButton.checked ? backend.palette.controlSelected
                : footerButton.hovered && footerButton.enabled ? backend.palette.controlHover
                : backend.palette.control
            border.width: footerButton.activeFocus ? 2 : 1
            border.color: footerButton.activeFocus ? backend.palette.focus : backend.palette.controlBorder
            opacity: footerButton.enabled ? 1 : 0.55
        }
    }
    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        visible: root.showFooter
        height: 48; color: backend.palette.panel
        Rectangle { width: parent.width; height: 1; color: backend.palette.border }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
            spacing: 4
            Label {
                visible: root.width >= 560
                text: root.visualEditEnabled ? root.visualStatus : "Preview · " + root.renderer.outputTemplateName
                font.family: Qt.application.font.family
                font.pixelSize: 12
                color: backend.palette.muted
                elide: Text.ElideRight
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Accessible.name: text
                ToolTip.visible: statusHover.hovered
                ToolTip.text: text
                HoverHandler { id: statusHover }
            }
            Item { Layout.fillWidth: true }
            FooterButton {
                objectName: "visualEditToggle"
                visible: root.allowVisualEdit
                text: "Visual Edit"
                hint: root.visualEditEnabled ? "Switch to read-only Preview" : "Switch to Visual Edit for supported text"
                darkMode: root.darkMode
                checkable: true
                checked: root.visualEditEnabled
                onClicked: root.visualEditEnabled = !root.visualEditEnabled
            }
            FooterButton {
                objectName: "visualEditSourceButton"
                visible: root.visualEditEnabled
                text: "Source"
                hint: "Edit Markdown source"
                darkMode: root.darkMode
                onClicked: root.sourceEditRequested()
            }
            FooterButton { text: "Split"; hint: "Show Source and Preview side by side"; darkMode: root.darkMode; checked: root.layoutMode === 1; onClicked: root.layoutRequested(1) }
            FooterButton { text: "Full"; hint: root.visualEditEnabled ? "Show Visual Edit across the document area" : "Show read-only Preview across the document area"; darkMode: root.darkMode; checked: root.layoutMode === 2; onClicked: root.layoutRequested(2) }
        }
    }

}
