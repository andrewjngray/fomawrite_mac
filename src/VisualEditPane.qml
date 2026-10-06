// The LEFT Visual Edit surface (a bounded, formatted editing projection of
// the Markdown source). The read-only RIGHT output pane is PublishingPreview.qml.
// Renamed from PreviewPane.qml in Cycle 133; objectNames are unchanged.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    objectName: "previewPane"
    property string visualEditorObjectName: "visualEditor"
    property string scrollObjectName: "previewScroll"
    property string renderedObjectName: "renderedPreview"
    property bool allowVisualEdit: true
    property bool showFooter: true
    property real bottomInset: 0
    property bool suspendViewportUpdates: false
    // Timers finish before their deferred parse/style callbacks. Keep viewport
    // restoration suspended until the latest callbacks have actually completed.
    property bool previewRefreshInFlight: false
    property bool visualRefreshInFlight: false
    readonly property bool viewportRefreshPending: refreshTimer.running || previewRefreshInFlight
        || (visualEditEnabled && (visualRefreshTimer.running || visualRefreshInFlight))
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
    signal templateMenuRequested(var anchor)
    property bool visualEditorFocused: visualText.activeFocus
    property string visualStatus: ""
    property string visualNotice: ""
    onVisualStatusChanged: {
        visualNotice = visualEditEnabled && visualStatus.indexOf("Source") >= 0
                && visualStatus.indexOf("Edit text,") !== 0 ? visualStatus : "";
    }
    property var visualImages: []
    property var visualTableCells: []
    readonly property var activeVisualImage: {
        for (var i = 0; i < visualImages.length; ++i) {
            var image = visualImages[i];
            if (visualText.cursorPosition >= image.visualStart
                    && visualText.cursorPosition <= image.visualStart + image.visualLength)
                return image;
        }
        return null;
    }
    property string visualSnapshot: ""
    property string visualSourceSnapshot: ""
    property bool synchronizingVisualText: false
    signal scrollFractionChanged(real fraction)
    signal viewportInteraction()
    signal writingActivity()
    function focusVisualEditor() { visualText.forceActiveFocus(); }
    function focusRenderedSurface() {
        if (visualEditEnabled) visualText.forceActiveFocus();
        else previewText.forceActiveFocus();
    }
    function viewportFraction() {
        return Math.max(0, Math.min(1, previewScroll.contentY
            / Math.max(1, previewScroll.contentHeight - previewScroll.height)));
    }
    function captureReadingAnchor() {
        var field = visualEditEnabled ? visualText : previewText;
        var position = Math.max(0, field.positionAt(0, Math.max(0, previewScroll.contentY - field.y + 1)));
        return { visual: visualEditEnabled, position: position,
            offset: field.y + field.positionToRectangle(position).y - previewScroll.contentY };
    }
    function restoreReadingAnchor(anchor) {
        if (anchor.visual !== visualEditEnabled || viewportRefreshPending) return;
        var field = visualEditEnabled ? visualText : previewText;
        var target = field.y + field.positionToRectangle(Math.max(0, Math.min(field.length, anchor.position))).y - anchor.offset;
        previewScroll.contentY = Math.max(0, Math.min(Math.max(0, previewScroll.contentHeight - previewScroll.height), target));
    }
    function readingViewportKey() {
        return [viewportGeometry(), previewScroll.contentY, visualEditEnabled].join(":");
    }
    function viewportGeometry() {
        return [previewScroll.width, previewScroll.height, previewScroll.contentHeight].join(":");
    }
    function stopViewportMotion() { previewScroll.cancelFlick(); }
    function scrollToFraction(fraction) { previewScroll.contentY = Math.max(0, previewScroll.contentHeight - previewScroll.height) * fraction; }
    signal anchorNavigationFailed(string anchor)
    function jumpToAnchor(anchor) {
        var decoded;
        try { decoded = decodeURIComponent(anchor); }
        catch (error) { root.anchorNavigationFailed(anchor); return false; }
        var position = renderer.previewAnchorPosition(previewText.textDocument, decoded);
        if (position < 0) { root.anchorNavigationFailed(decoded); return false; }
        previewScroll.contentY = Math.max(0, Math.min(previewScroll.contentHeight - previewScroll.height, previewText.positionToRectangle(position).y));
        return true;
    }
    function navigateToAnchor(anchor) {
        pendingAnchor = anchor;
        reload();
    }
    function navigateToSourcePosition(position) {
        if (!allowVisualEdit || !visualEditEnabled) return;
        var visualPosition = renderer.visualPositionForSource(position);
        loadVisualProjection(visualPosition, visualPosition);
        Qt.callLater(function() {
            if (!root.visible || !root.visualEditEnabled) return;
            root.focusVisualEditor();
            root.ensureVisualCursorVisible();
        });
    }
    signal layoutRequested(int mode)
    signal linkRequested(url link)
    color: backend.palette.page
    property string renderedMarkdown: ""
    property int previewRefreshRevision: 0
    property int visualRefreshRevision: 0
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
        var revision = ++previewRefreshRevision;
        previewRefreshInFlight = true;
        renderedMarkdown = "";
        Qt.callLater(function() {
            if (!root || revision !== root.previewRefreshRevision) return;
            try {
                renderedMarkdown = renderer.previewMarkdown(markdown);
            } catch (error) {
                if (revision === root.previewRefreshRevision)
                    root.previewRefreshInFlight = false;
                throw error;
            }
            Qt.callLater(function() {
                if (!root || revision !== root.previewRefreshRevision) return;
                try {
                    root.renderer.stylePreview(previewText.textDocument);
                    if (root.pendingAnchor !== "") {
                        var anchor = root.pendingAnchor;
                        root.pendingAnchor = "";
                        root.jumpToAnchor(anchor);
                    }
                } finally {
                    if (revision === root.previewRefreshRevision)
                        root.previewRefreshInFlight = false;
                }
            });
        });
    }
    function reload() {
        // Force a fresh Markdown parse even when the source has not changed.
        refreshTimer.stop();
        // Invalidate queued parse/style completions before clearing the text.
        // Otherwise an old completion can consume a new pending heading while
        // its replacement preview is still empty.
        var revision = ++previewRefreshRevision;
        previewRefreshInFlight = true;
        renderedMarkdown = "";
        Qt.callLater(function() {
            if (root && revision === root.previewRefreshRevision) root.refresh();
        });
    }
    function loadVisualProjection(selectionStart, selectionEnd) {
        var revision = ++visualRefreshRevision;
        visualRefreshInFlight = false;
        if (!visualEditEnabled) {
            visualRefreshTimer.stop();
            return;
        }
        if (visualText.inputMethodComposing) {
            visualRefreshTimer.restart();
            return;
        }
        // Keep the field's own post-edit selection across a canonical refresh.
        // This is also bounded for source/external changes that shorten text.
        var first = selectionStart === undefined ? visualText.selectionStart : selectionStart;
        var last = selectionEnd === undefined ? visualText.selectionEnd : selectionEnd;
        var projection = renderer.visualProjection();
        synchronizingVisualText = true;
        visualSourceSnapshot = projection.source || "";
        visualSnapshot = projection.visualText || "";
        visualImages = projection.images || [];
        visualTableCells = projection.tableCells || [];
        visualText.text = visualSnapshot;
        visualRefreshInFlight = true;
        Qt.callLater(function() {
            if (!root || revision !== root.visualRefreshRevision) return;
            try {
                if (!root.visualEditEnabled) return;
                if (visualText.inputMethodComposing) { visualRefreshTimer.restart(); return; }
                root.renderer.styleVisualEditor(visualText.textDocument, root.visualTextSize, root.visualTypeface);
            } finally {
                if (revision === root.visualRefreshRevision)
                    root.visualRefreshInFlight = false;
            }
        });
        first = Math.max(0, Math.min(first, visualSnapshot.length));
        last = Math.max(0, Math.min(last, visualSnapshot.length));
        if (first === last)
            visualText.cursorPosition = last;
        else
            visualText.select(first, last);
        synchronizingVisualText = false;
        Qt.callLater(ensureVisualCursorVisible);
    }
    function ensureVisualCursorVisible() {
        if (!visualEditEnabled || !visualText.activeFocus || synchronizingVisualText || suspendViewportUpdates)
            return;
        var caret = visualText.cursorRectangle;
        var top = visualText.y + caret.y;
        var bottom = top + caret.height;
        var target = previewScroll.contentY;
        if (top < target + 20) target = top - 20;
        else if (bottom > target + previewScroll.height - 20)
            target = bottom - previewScroll.height + 20;
        previewScroll.contentY = Math.max(0, Math.min(target,
            Math.max(0, previewScroll.contentHeight - previewScroll.height)));
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
        // Minimal string diffs can split a UTF-16 surrogate pair when two
        // emoji share their high surrogate. Include the whole code point.
        if (prefix > 0 && prefix < before.length
                && before.charCodeAt(prefix) >= 0xDC00 && before.charCodeAt(prefix) <= 0xDFFF)
            --prefix;
        if (beforeEnd < before.length && beforeEnd > prefix
                && before.charCodeAt(beforeEnd) >= 0xDC00 && before.charCodeAt(beforeEnd) <= 0xDFFF) {
            ++beforeEnd;
            ++afterEnd;
        }
        return { start: prefix, length: beforeEnd - prefix,
                 replacement: after.slice(prefix, afterEnd) };
    }
    function applyVisualTextChange() {
        if (!allowVisualEdit || !visualEditEnabled || synchronizingVisualText || visualText.inputMethodComposing
                || visualText.text === visualSnapshot)
            return;
        var change = visualDiff(visualSnapshot, visualText.text);
        if (renderer.applyVisualEdit(change.start, change.length, change.replacement,
                                     visualSourceSnapshot)) {
            root.writingActivity();
            visualStatus = "Applied to Markdown source.";
            var first = visualText.selectionStart;
            var last = visualText.selectionEnd;
            root.loadVisualProjection(first, last);
        } else {
            visualStatus = "That range is source-only or changed elsewhere. Edit it in Source.";
            loadVisualProjection(change.start, change.start);
        }
    }
    function navigateVisualTable(backwards) {
        if (!allowVisualEdit || !visualEditEnabled || visualText.inputMethodComposing) return false;
        var result = renderer.navigateVisualTable(visualText.selectionStart, visualText.selectionEnd,
                                                  backwards, visualSourceSnapshot);
        if (!result.handled) return false;
        if (result.reason === "moved") {
            visualText.cursorPosition = result.cursor;
            visualStatus = "Table cell. Tab and Shift+Tab move between cells; F6 leaves the editor.";
            ensureVisualCursorVisible();
        } else if (result.reason === "boundary") {
            visualStatus = "Table boundary. F6 leaves the editor; use Source to change its structure.";
        } else if (result.reason === "stale") {
            visualStatus = "The table changed elsewhere. Wait for the view to refresh or use Source.";
        } else {
            visualStatus = "Place the caret in one table cell to navigate. Use Source for structural selections.";
        }
        return true;
    }
    function insertVisualBreak(shift) {
        if (!allowVisualEdit || !visualEditEnabled) return;
        if (visualText.selectionStart !== visualText.selectionEnd) {
            visualStatus = "Use Source to replace a selection with a new block.";
            return;
        }
        var result = renderer.applyVisualBreak(visualText.cursorPosition, !!shift, visualSourceSnapshot);
        if (result.applied) {
            visualStatus = "Break applied to Markdown source.";
            loadVisualProjection(result.cursor, result.cursor);
        } else {
            visualStatus = "This break needs Source editing to preserve Markdown.";
        }
    }
    onVisualEditEnabledChanged: {
        visualStatus = visualEditEnabled
            ? "Edit text, simple table cells and image captions. Return splits or continues simple list items; unsupported structures stay in Source."
            : "Rendered preview is read-only.";
        var revision = ++visualRefreshRevision;
        visualRefreshTimer.stop();
        visualRefreshInFlight = visualEditEnabled;
        if (visualEditEnabled) {
            Qt.callLater(function() {
                if (root && revision === root.visualRefreshRevision && root.visualEditEnabled)
                    root.loadVisualProjection();
            });
        }
    }
    Connections { target: root.renderer; function onOutputStyleChanged() { root.reload(); } }
    Component.onCompleted: refresh()

    Timer { id: refreshTimer; interval: 120; onTriggered: root.refresh() }
    Timer { id: visualRefreshTimer; interval: 80; onTriggered: root.loadVisualProjection() }
    Flickable {
        id: previewScroll
        objectName: root.scrollObjectName
        anchors.fill: parent
        anchors.bottomMargin: root.showFooter ? previewFooter.height : root.bottomInset
        anchors.topMargin: imageContext.visible ? imageContext.height : 0
        clip: true
        contentWidth: width
        contentHeight: Math.max(height, root.visualEditEnabled
            ? visualText.y + visualText.implicitHeight + 64
            : previewText.implicitHeight + 100)
        boundsBehavior: Flickable.StopAtBounds
        WheelHandler { target: null; blocking: false; onWheel: root.writingActivity() }
        onMovementStarted: { root.viewportInteraction(); root.writingActivity(); }
        onContentYChanged: if (root.visible && !root.suspendViewportUpdates)
            root.scrollFractionChanged(contentY / Math.max(1, contentHeight - height))
        ScrollBar.vertical: ScrollBar {}
        TextEdit {
            id: previewText
            objectName: root.renderedObjectName
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
            Keys.onPressed: { root.viewportInteraction(); root.writingActivity(); }
            Accessible.name: "Preview: read-only rendered Markdown"
            visible: !root.visualEditEnabled
        }
        TextEdit {
            id: visualText
            objectName: root.visualEditorObjectName
            visible: root.visualEditEnabled
            readOnly: !root.allowVisualEdit || !root.visualEditEnabled
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
            onPreeditTextChanged: if (activeFocus && preeditText.length) root.writingActivity()
            onTextChanged: root.applyVisualTextChange()
            onInputMethodComposingChanged: if (!inputMethodComposing) { root.applyVisualTextChange(); visualRefreshTimer.restart(); }
            onCursorRectangleChanged: Qt.callLater(root.ensureVisualCursorVisible)
            onActiveFocusChanged: if (activeFocus) Qt.callLater(root.ensureVisualCursorVisible)
            Keys.priority: Keys.BeforeItem
            Keys.onPressed: function(event) {
                if ((event.text.length && !(event.modifiers & (Qt.ControlModifier | Qt.MetaModifier)))
                    || event.key === Qt.Key_Backspace || event.key === Qt.Key_Delete
                    || event.key === Qt.Key_Up || event.key === Qt.Key_Down || event.key === Qt.Key_PageUp || event.key === Qt.Key_PageDown) root.writingActivity();
                root.viewportInteraction();
                if ((event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab)
                        && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))) {
                    event.accepted = root.navigateVisualTable(event.key === Qt.Key_Backtab || !!(event.modifiers & Qt.ShiftModifier));
                    if (event.accepted) return;
                }
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
                root.viewportInteraction();
                root.insertVisualBreak(event.modifiers & Qt.ShiftModifier);
                event.accepted = true;
            }
            Keys.onEnterPressed: function(event) {
                root.viewportInteraction();
                root.insertVisualBreak(event.modifiers & Qt.ShiftModifier);
                event.accepted = true;
            }
            Accessible.name: "Visual Edit: editable Markdown text"
            Accessible.description: "Edits supported text, image descriptions and simple table cells. Tab and Shift+Tab move between table cells; F6 leaves the editor. Return splits or continues simple list items. Other structures use Source."
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
        id: imageContext
        objectName: "visualImageContext"
        visible: root.visualEditEnabled && root.activeVisualImage !== null
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        height: 44
        color: backend.palette.panel
        RowLayout {
            anchors.fill: parent; anchors.margins: 6; spacing: 8
            Label {
                text: "Image · " + (root.activeVisualImage ? root.activeVisualImage.altText || "No description" : "")
                elide: Text.ElideRight
                color: backend.palette.text
                font.pixelSize: 12
                Layout.fillWidth: true
                Layout.minimumWidth: 0
            }
            ChromeButton {
                objectName: "visualImageInspectButton"
                text: "Inspect image…"; tonal: true
                hint: "Preview image and its Markdown destination"
                onClicked: imageInspector.open()
            }
        }
    }
    Popup {
        id: imageInspector
        objectName: "visualImageInspector"
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(440, parent ? parent.width - 32 : 440)
        padding: 20
        modal: true; focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: backend.palette.panel; radius: 12; border.color: backend.palette.border }
        onClosed: if (root.visualEditEnabled && root.visible) root.focusVisualEditor()
        contentItem: ColumnLayout {
            spacing: 12
            Label { text: "Image"; font.pixelSize: 18; font.bold: true; color: backend.palette.text }
            Image {
                id: inspectedImage
                objectName: "visualImageThumbnail"
                Layout.fillWidth: true; Layout.preferredHeight: 180
                source: root.activeVisualImage ? root.activeVisualImage.previewUrl || "" : ""
                sourceSize.width: 760; sourceSize.height: 360
                fillMode: Image.PreserveAspectFit
                Accessible.name: root.activeVisualImage ? root.activeVisualImage.altText || "Image preview" : "Image preview"
            }
            Label {
                visible: inspectedImage.status !== Image.Ready
                text: inspectedImage.status === Image.Loading ? "Loading image…" : "Preview unavailable. Remote images remain in the document."
                wrapMode: Text.WordWrap; Layout.fillWidth: true
                color: backend.palette.muted; font.pixelSize: 12
            }
            Label {
                text: root.activeVisualImage ? root.activeVisualImage.destination : ""
                wrapMode: Text.WrapAnywhere; Layout.fillWidth: true
                color: backend.palette.muted; font.pixelSize: 12
            }
            Label {
                text: "Edit an existing description in Visual Edit. Use Source to add or remove a description, or change the destination."
                wrapMode: Text.WordWrap; Layout.fillWidth: true
                color: backend.palette.text; font.pixelSize: 13
            }
            RowLayout {
                Item { Layout.fillWidth: true }
                ChromeButton { text: "Edit in Source"; tonal: true; onClicked: { imageInspector.close(); root.sourceEditRequested(); } }
                ChromeButton { text: "Done"; tonal: true; onClicked: imageInspector.close() }
            }
        }
    }
    Rectangle {
        id: visualNoticePanel
        objectName: "visualEditNotice"
        visible: root.visualEditEnabled && root.visualNotice !== ""
        anchors.left: parent.left; anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        anchors.bottomMargin: (root.showFooter ? previewFooter.height : root.bottomInset) + 12
        height: visualNoticeText.implicitHeight + 60
        z: 4
        radius: 8
        color: backend.palette.panel
        border.color: backend.palette.controlBorder
        Label {
            id: visualNoticeText
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            anchors.margins: 12
            text: root.visualNotice
            wrapMode: Text.WordWrap
            font.pixelSize: 13
            color: backend.palette.text
            Accessible.name: text
        }
        Row {
            anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.margins: 8; spacing: 6
            ChromeButton {
                objectName: "visualNoticeSourceButton"
                text: "Edit in Source"; tonal: true
                onClicked: { root.visualNotice = ""; root.sourceEditRequested(); }
            }
            ChromeButton { text: "Dismiss"; tonal: true; onClicked: root.visualNotice = "" }
        }
    }
    WorkspaceFooter {
        id: previewFooter
        objectName: "previewFooter"
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        visible: root.showFooter
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12; anchors.rightMargin: 12
            spacing: 6
            FooterButton {
                id: templateButton
                objectName: "previewTemplateButton"
                visible: !root.visualEditEnabled
                text: (root.width >= 560 ? "Preview · " : "") + root.renderer.outputTemplateName
                hint: "Choose preview template. Current template: " + root.renderer.outputTemplateName
                menuIndicator: true
                Layout.fillWidth: true
                Layout.minimumWidth: 72
                Layout.maximumWidth: implicitWidth
                onClicked: root.templateMenuRequested(templateButton)
            }
            Label {
                visible: root.visualEditEnabled && root.width >= 560
                text: "Visual Edit"
                font.family: Qt.application.font.family
                font.pixelSize: 13
                color: backend.palette.muted
                elide: Text.ElideRight
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Accessible.name: root.visualStatus
                ToolTip.visible: statusHover.hovered
                ToolTip.text: root.visualStatus
                HoverHandler { id: statusHover }
            }
            Item { Layout.fillWidth: true; Layout.minimumWidth: 0 }
            FooterButton {
                objectName: "visualEditToggle"
                visible: root.allowVisualEdit
                text: "Visual Edit"
                hint: root.visualEditEnabled ? "Switch to read-only Preview" : "Switch to Visual Edit for supported text"
                checkable: true
                checked: root.visualEditEnabled
                Layout.minimumWidth: implicitWidth
                onClicked: {
                    root.visualEditEnabled = !root.visualEditEnabled;
                    if (root.visualEditEnabled)
                        Qt.callLater(root.focusVisualEditor);
                }
            }
            FooterButton {
                objectName: "visualEditSourceButton"
                visible: root.visualEditEnabled
                text: "Source"
                hint: "Edit Markdown source"
                Layout.minimumWidth: implicitWidth
                onClicked: root.sourceEditRequested()
            }
            ToolbarGroup {
                objectName: "previewLayoutControls"
                Layout.minimumWidth: implicitWidth
                FooterButton {
                    objectName: "previewSplitButton"
                    text: "Split"
                    hint: "Show Source and Preview side by side"
                    grouped: true
                    checked: root.layoutMode === 1
                    Accessible.checkable: true
                    Accessible.checked: checked
                    onClicked: root.layoutRequested(1)
                }
                FooterButton {
                    objectName: "previewFullButton"
                    text: "Full"
                    hint: root.visualEditEnabled ? "Show Visual Edit across the document area" : "Show read-only Preview across the document area"
                    grouped: true
                    checked: root.layoutMode === 2
                    Accessible.checkable: true
                    Accessible.checked: checked
                    onClicked: root.layoutRequested(2)
                }
            }
        }
    }

}
