import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs as Dialogs
import Qt.labs.platform as Platform
import QtQuick.Layouts
import QtQuick.Window
import "EditorMutations.js" as EditorMutations
import "LinkSyntax.js" as LinkSyntax

ApplicationWindow {
    id: win
    width: 1280
    height: 820
    minimumWidth: 720
    minimumHeight: 520
    flags: isMac ? Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint : Qt.Window
    topPadding: 0
    visible: true
    font.family: Qt.platform.os === "osx" ? Qt.application.font.family : "Helvetica Neue"
    font.pixelSize: 13
    title: (backend.modified ? "* " : "") + backend.fileName + " - Fomawrite"

    readonly property bool isMac: Qt.platform.os === "osx"
    readonly property bool darkMode: backend.darkMode
    readonly property color pageColor: backend.themeBackground
    readonly property color textColor: backend.themeForeground
    readonly property color strongTextColor: backend.themeForeground
    readonly property color mutedColor: backend.palette.muted
    readonly property var appBackend: backend
    readonly property color selectionFill: backend.themeSelection
    // The desktop's text size knob (GNOME's text-scaling-factor, which
    // `omarchy display text size` drives) anchored so its 12px default leaves
    // the app at the sizes it was designed around.
    readonly property real textScale: backend.textScale
    readonly property string activeWritingAppearance: ["editorial", "book", "code"].indexOf(workspaceSettings.writingAppearance) >= 0
        ? workspaceSettings.writingAppearance : "manuscript"
    readonly property bool codeAppearance: activeWritingAppearance === "code"
    onActiveWritingAppearanceChanged: Qt.callLater(function() { backend.setSourceAppearance(activeWritingAppearance); codeDecorationsTimer.restart(); editorFlick.contentX = 0; })
    onDarkModeChanged: Qt.callLater(function() { backend.setSourceAppearance(activeWritingAppearance); })
    readonly property string editorFontFamily: activeWritingAppearance === "editorial" ? Qt.application.font.family
        : activeWritingAppearance === "book" ? "Georgia" : codeAppearance ? "Menlo" : "iA Writer Mono S"
    readonly property int writingBasePixelSize: codeAppearance ? 15 : activeWritingAppearance === "manuscript" ? 18 : activeWritingAppearance === "book" ? 20 : 19
    readonly property int editorFontPixelSize: scaledSize(writingBasePixelSize * paneZoom.sourceZoom / 100)
    readonly property int editorWidth: codeAppearance ? Math.max(180, editorPane.width - 112) : Math.min(
        scaledSize(680),
        Math.max(180, editorPane.width - 64))
    property int tabInset: 0
    Timer { interval: 250; repeat: true; running: win.visible && win.isMac; onTriggered: win.tabInset = backend.nativeTabInset() }
    readonly property int workspaceCursor: editor.cursorPosition
    function restoreWorkspaceCursor(position) {
        Qt.callLater(function() { editor.cursorPosition = Math.max(0, Math.min(position, editor.length)); editorFlick.ensureCursorVisible(); });
    }
    property bool documentChromeHidden: false
    readonly property bool documentChromeMenuOpen: sourceAppearanceMenu.opened || previewTemplateMenu.opened
        || writingOptions.opened || workspaceMenu.opened || formatQuickMenu.opened || libraryActions.opened
        || topChrome.zoomMenuOpen
    function writingActivity() {
        if (!workspaceSettings.autoHideChrome || documentChromeMenuOpen) return;
        var item = activeFocusItem;
        var keyboardChrome = item && (item.focusReason === Qt.TabFocusReason
            || item.focusReason === Qt.BacktabFocusReason || item.focusReason === Qt.ShortcutFocusReason)
            && (isInside(item, topChrome) || isInside(item, documentFooter));
        if (keyboardChrome) return;
        topChrome.keyboardReveal = false;
        documentFooter.keyboardReveal = false;
        topChrome.pointerReveal = false;
        documentFooter.pointerReveal = false;
        documentChromeHidden = true;
    }
    function undoEditing() {
        // The Live editor keeps its own history; undoing the mirrored document
        // underneath it would diverge the two. Route to the page instead.
        if (canFormatLive) { backend.editorBridge.requestUndo(); return; }
        if (editTarget === editor) editor.cursorPosition = backend.undoSource(editor.cursorPosition);
        else editTarget.undo();
    }
    function redoEditing() {
        if (canFormatLive) { backend.editorBridge.requestRedo(); return; }
        if (editTarget === editor) editor.cursorPosition = backend.redoSource(editor.cursorPosition);
        else editTarget.redo();
    }
    function performDocumentEdit(action) {
        var before = editor.text;
        action();
        if (editor.text !== before) writingActivity();
    }
    function revealDocumentChrome() {
        documentChromeHidden = false;
        topChrome.keyboardReveal = true;
        documentFooter.keyboardReveal = true;
    }
    property bool synchronizingScroll: false
    property var documentViewportTransition: null
    property var zoomViewportCheckpoint: null
    readonly property bool changingDocumentView: documentViewportTransition !== null

    // A view change can rewrap both documents several times while SplitView
    // settles. Keep their reading positions independent during that reflow;
    // programmatic clamps must not become synchronized-scroll gestures.
    function captureSourceReadingAnchor() {
        var position = Math.max(0, editor.positionAt(0, Math.max(0, editorFlick.contentY - editor.y + 1)));
        return { position: position, offset: editor.y + editor.positionToRectangle(position).y - editorFlick.contentY };
    }
    function restoreSourceReadingAnchor(anchor) {
        var y = editor.y + editor.positionToRectangle(Math.max(0, Math.min(editor.length, anchor.position))).y - anchor.offset;
        editorFlick.contentY = Math.max(0, Math.min(Math.max(0, editorFlick.contentHeight - editorFlick.height), y));
    }
    function readingViewportKey() {
        return [editorFlick.width, editorFlick.height, editorFlick.contentHeight,
                editorFlick.contentY, previewPane.readingViewportKey()].join(":");
    }
    function beginDocumentViewportTransition(preserveReadingAnchor) {
        if (!documentViewportTransition) {
            editorFlick.cancelFlick();
            wheelScroll.stop();
            previewPane.stopViewportMotion();
            documentViewportTransition = {
                sourceFraction: Math.max(0, Math.min(1, editorFlick.contentY
                    / Math.max(1, editorFlick.contentHeight - editorFlick.height))),
                previewFraction: previewPane.viewportFraction(),
                geometry: "", stablePasses: 0, passes: 0
            };
        }
        if (preserveReadingAnchor) {
            if (!documentViewportTransition.sourceAnchor) {
                // Rewrapping changes the first character on a visible line. Keep
                // the original logical anchor through an uninterrupted zoom
                // series so preset/reset cycles cannot drift one line at a time.
                var checkpoint = zoomViewportCheckpoint;
                var reuse = checkpoint && checkpoint.key === readingViewportKey();
                documentViewportTransition.sourceAnchor = reuse ? checkpoint.sourceAnchor : captureSourceReadingAnchor();
                documentViewportTransition.previewAnchor = reuse ? checkpoint.previewAnchor : previewPane.captureReadingAnchor();
            }
        } else {
            zoomViewportCheckpoint = null;
            documentViewportTransition.sourceAnchor = null;
            documentViewportTransition.previewAnchor = null;
        }
        documentViewportTransition.geometry = "";
        documentViewportTransition.stablePasses = 0;
        documentViewportTransition.passes = 0;
        documentViewportTransition.focusApplied = false;
        documentViewportSettle.restart();
    }
    function cancelDocumentViewportTransition() {
        zoomViewportCheckpoint = null;
        documentViewportSettle.stop();
        documentViewportTransition = null;
    }
    function settleDocumentViewportTransition() {
        var state = documentViewportTransition;
        if (!state) return;
        if (!state.focusApplied && state.focusTarget) {
            state.focusApplied = true;
            // Restore editing focus while caret-driven scrolling is suspended.
            // The saved viewport is reapplied after focus and deferred layout.
            // Never pull focus out of an auxiliary field (link editor, Find,
            // library filter) the writer moved into while the view settled.
            var auxiliaryFocused = activeFocusItem && typeof activeFocusItem.cut === "function"
                && activeFocusItem !== editor && !isInside(activeFocusItem, editorPane) && !isInside(activeFocusItem, previewPane);
            if (auxiliaryFocused) {
                // keep the writer's focus
            } else if (state.focusTarget === "live" && workspaceLayout.liveEditEnabled && liveEditorLoader.item)
                liveEditorLoader.item.focusLive();
            else if (state.focusTarget === "source" && win.sourceEditorVisible)
                editor.forceActiveFocus();
            else if (previewPane.visible)
                previewPane.focusRenderedSurface();
        }
        var geometry = [editorFlick.width, editorFlick.height, editorFlick.contentHeight,
                        previewPane.viewportGeometry()].join(":");
        if (state.sourceAnchor) restoreSourceReadingAnchor(state.sourceAnchor);
        else editorFlick.contentY = state.sourceFraction * Math.max(0, editorFlick.contentHeight - editorFlick.height);
        if (state.previewAnchor) previewPane.restoreReadingAnchor(state.previewAnchor);
        else previewPane.scrollToFraction(state.previewFraction);
        state.stablePasses = geometry === state.geometry ? state.stablePasses + 1 : 0;
        state.geometry = geometry;
        ++state.passes;
        // Two unchanged frames include deferred rich-text styling and layout.
        // A bound also releases the transaction if a display keeps resizing.
        var settled = state.stablePasses >= 2 && !previewPane.viewportRefreshPending;
        if (settled || state.passes >= 16) {
            cancelDocumentViewportTransition();
            if (settled && state.sourceAnchor && state.previewAnchor)
                zoomViewportCheckpoint = { key: readingViewportKey(),
                    sourceAnchor: state.sourceAnchor, previewAnchor: state.previewAnchor };
        }
    }
    Timer {
        id: documentViewportSettle
        interval: 16
        repeat: true
        onTriggered: win.settleDocumentViewportTransition()
    }
    property bool closeConfirmed: false
    property bool searchOpen: false
    property bool searchUpdating: false
    property var searchMatches: []
    property int searchMatchIndex: -1
    property int searchAnchor: 0
    property url pendingOpenUrl
    property string pendingOpenFragment: ""
    property string navigationNotice: ""
    property string pendingAction: ""
    property bool replaceOpen: false
    property bool awaitingPendingSave: false
    property string pendingFileName: ""
    property int pendingHistoryDirection: 0
    property var completionItems: []
    property int completionStart: -1
    property int completionEnd: -1
    property string completionPrefix: ""
    property var documentStatistics: ({ words: 0, characters: 0, charactersWithoutSpaces: 0,
                                        sentences: 0, readingMinutes: 0, speakingMinutes: 0,
                                        tasks: 0, humanWords: 0, aiWords: 0, referenceWords: 0 })

    function refreshDocumentStatistics() {
        documentStatistics = backend.documentStatistics(editor.text);
    }
    function compactStatistic(metric) {
        var value = documentStatistics[metric] || 0;
        switch (metric) {
        case "characters": return value + " chars";
        case "charactersWithoutSpaces": return value + " chars no spaces";
        case "sentences": return value + " sentences";
        case "readingMinutes": return value + " min read";
        case "speakingMinutes": return value + " min speak";
        case "tasks": return value + " tasks";
        case "humanWords": return value + " human";
        case "aiWords": return value + " AI";
        case "referenceWords": return value + " reference";
        default: return value + " words";
        }
    }
    function compactToolbarStatistics() {
        var values = [];
        if (workspaceSettings.toolbarCharacters) values.push(compactStatistic("characters"));
        if (workspaceSettings.toolbarCharactersNoSpaces) values.push(compactStatistic("charactersWithoutSpaces"));
        if (workspaceSettings.toolbarWords) values.push(compactStatistic("words"));
        if (workspaceSettings.toolbarSentences) values.push(compactStatistic("sentences"));
        if (workspaceSettings.toolbarReadingTime) values.push(compactStatistic("readingMinutes"));
        if (workspaceSettings.toolbarSpeakingTime) values.push(compactStatistic("speakingMinutes"));
        if (workspaceSettings.toolbarTasks) values.push(compactStatistic("tasks"));
        if (workspaceSettings.toolbarHuman) values.push(compactStatistic("humanWords"));
        if (workspaceSettings.toolbarAI) values.push(compactStatistic("aiWords"));
        if (workspaceSettings.toolbarReference) values.push(compactStatistic("referenceWords"));
        return values.length > 0 ? values.join("  ·  ") : "No statistics selected";
    }

    Timer {
        id: statisticsRefreshTimer
        interval: 120
        onTriggered: win.refreshDocumentStatistics()
    }

    Timer {
        id: customReviewRefreshTimer
        interval: 200
        onTriggered: backend.setStyleReviewWords(workspaceSettings.reviewWords,
                                                  workspaceSettings.styleCheckCustom,
                                                  workspaceSettings.styleCheckFillers)
    }
    function applyStyleReviewNow() {
        customReviewRefreshTimer.stop();
        backend.setStyleReviewWords(workspaceSettings.reviewWords,
                                    workspaceSettings.styleCheckCustom,
                                    workspaceSettings.styleCheckFillers);
    }
    AboutDialog { id: aboutDialog }

    Connections {
        target: backend
        function onDocumentStatisticsChanged() { statisticsRefreshTimer.restart(); }
        function onDocumentLoaded() { statisticsRefreshTimer.restart(); }
    }

    function isInside(item, ancestor) {
        while (item) {
            if (item === ancestor) return true;
            item = item.parent;
        }
        return false;
    }
    // Source owns formatting only while editing, or while traversing its
    // explicit formatting controls. An unrelated field never borrows a stale
    // Source selection merely because the editor remains visible.
    property bool sourceFormattingOwned: false
    readonly property bool sourceEditorVisible: editorPane.visible && !workspaceLayout.liveEditEnabled
    onSourceEditorVisibleChanged: if (!sourceEditorVisible && completionPopup) completionPopup.close()
    // Format commands reach the Live editor while it has focus (see Backend::liveWrapSelection).
    readonly property bool canFormatLive: workspaceLayout.liveEditEnabled && liveEditorLoader.item !== null && liveEditorLoader.item.hasLiveFocus
    readonly property bool canFormatSource: sourceEditorVisible && sourceFormattingOwned
        && !editor.readOnly && !editor.inputMethodComposing
    function isSourceFormattingChrome(item) {
        var current = item;
        while (current) {
            if (current.sourceFormattingControl === true) return true;
            current = current.parent;
        }
        for (var menu of [formatQuickMenu, formatPopover]) {
            if (menu && menu.visible && menu.contentItem && isInside(item, menu.contentItem.parent))
                return true;
        }
        return false;
    }
    function updateSourceFormattingOwner() {
        var item = activeFocusItem;
        if (sourceEditorVisible && isInside(item, editor)) sourceFormattingOwned = true;
        else if (!isSourceFormattingChrome(item)) sourceFormattingOwned = false;
    }

    // F6 offers an explicit route out of the text editor, where Tab is writing input.
    function firstWorkspaceControl(item) {
        if (!item || !item.visible || !item.enabled) return null;
        if (item.activeFocusOnTab) return item;
        for (var i = 0; i < item.children.length; ++i) {
            var child = firstWorkspaceControl(item.children[i]);
            if (child) return child;
        }
        return null;
    }
    function focusWorkspaceRegion(region) {
        if (region === "source" && editorPane.visible) {
            if (workspaceLayout.liveEditEnabled && liveEditorLoader.item) liveEditorLoader.item.focusLive();
            else { editor.forceActiveFocus(Qt.TabFocusReason); editorFlick.ensureCursorVisible(); }
            return;
        }
        if (region === "preview" && previewPane.visible) {
            // Read-only Preview has no per-pane footer to receive tab focus.
            // Enter the visible writing surface explicitly in both modes.
            previewPane.focusRenderedSurface();
            return;
        }
        var pane = region === "organizer" ? organizerPane
            : region === "files" ? libraryPane : region === "preview" ? previewPane : null;
        var target = pane ? firstWorkspaceControl(pane) : null;
        if (target) target.forceActiveFocus(Qt.TabFocusReason);
        else { win.revealDocumentChrome(); topChrome.focusWorkspaceControl(); }
    }
    function focusWritingSurface() {
        if (editorPane.visible && workspaceLayout.liveEditEnabled) { if (liveEditorLoader.item) liveEditorLoader.item.focusLive(); return; }
        focusWorkspaceRegion(editorPane.visible ? "source" : "preview");
    }
    readonly property bool workspaceOwnsKeyboard: !navigationDrawer.visible && (!activeFocusItem
        || isInside(activeFocusItem, workspaceSplit) || isInside(activeFocusItem, topChrome)
        || isInside(activeFocusItem, documentFooter))
    function cycleWorkspaceFocus(reverse) {
        var regions = [];
        if (organizerSlot.visible) regions.push("organizer");
        if (filesSlot.visible) regions.push("files");
        if (editorPane.visible) regions.push("source");
        if (previewPane.visible) regions.push("preview");
        regions.push("toolbar");
        var current = isInside(activeFocusItem, organizerPane) ? "organizer"
            : isInside(activeFocusItem, libraryPane) ? "files"
            : isInside(activeFocusItem, editorPane) ? "source"
            : isInside(activeFocusItem, previewPane) ? "preview" : "toolbar";
        var index = regions.indexOf(current);
        focusWorkspaceRegion(regions[(index + (reverse ? regions.length - 1 : 1)) % regions.length]);
    }
    function rescueHiddenWorkspaceFocus() {
        if (navigationDrawer.visible) return;
        var item = activeFocusItem;
        if (item && !item.visible && (isInside(item, workspaceSplit) || isInside(item, topChrome)
                                     || isInside(item, documentFooter)))
            focusWritingSurface();
    }
    Connections {
        target: documentFooter
        function onLayoutModeChanged() { Qt.callLater(win.rescueHiddenWorkspaceFocus); }
    }
    Shortcut {
        sequence: "F6"; context: Qt.WindowShortcut; enabled: win.workspaceOwnsKeyboard
        onActivated: win.cycleWorkspaceFocus(false)
    }
    Shortcut {
        sequence: "Shift+F6"; context: Qt.WindowShortcut; enabled: win.workspaceOwnsKeyboard
        onActivated: win.cycleWorkspaceFocus(true)
    }
    Shortcut {
        sequence: "Alt+M"; context: Qt.WindowShortcut
        onActivated: { win.revealDocumentChrome(); topChrome.focusWorkspaceControl(); }
    }
    property string lastZoomPane: "source"
    property string lastWritingSurface: "source"
    function updateWritingSurfaceFromFocus() {
        // Bound activeFocus flags can lag the window's focus-item notification.
        // Classify the actual editor item; toolbar/menu focus keeps the last
        // writing surface rather than borrowing a stale flag.
        var item = activeFocusItem;
        if (isInside(item, editor)) { lastWritingSurface = "source"; lastZoomPane = "source"; }
        else if (isInside(item, previewPane)) lastZoomPane = "preview";
    }
    onActiveFocusItemChanged: {
        // Pointer focus from a previous toolbar click must not pin the bars open.
        var keyboardFocus = activeFocusItem && (activeFocusItem.focusReason === Qt.TabFocusReason
            || activeFocusItem.focusReason === Qt.BacktabFocusReason || activeFocusItem.focusReason === Qt.ShortcutFocusReason);
        topChrome.keyboardReveal = keyboardFocus && isInside(activeFocusItem, topChrome);
        documentFooter.keyboardReveal = keyboardFocus && isInside(activeFocusItem, documentFooter);
        updateWritingSurfaceFromFocus();
        updateSourceFormattingOwner();
    }

    WorkspaceLayout {
        id: workspaceLayout
        objectName: "workspaceLayout"
        availableWidth: Math.max(0, win.width - 3)
        activeSurface: win.lastWritingSurface
    }
    Connections {
        target: workspaceLayout
        function onLiveEditEnabledChanged() {
            win.sourceFormattingOwned = false;
            Qt.callLater(win.rescueHiddenWorkspaceFocus);
        }
        function onEffectiveOrganizerVisibleChanged() { Qt.callLater(win.rescueHiddenWorkspaceFocus); }
        function onEffectiveFilesVisibleChanged() { Qt.callLater(win.rescueHiddenWorkspaceFocus); }
        function onEffectiveLayoutModeChanged() {
            if (workspaceLayout.effectiveLayoutMode === 2) win.sourceFormattingOwned = false;
            Qt.callLater(win.rescueHiddenWorkspaceFocus);
        }
    }
    readonly property var workspacePaneState: workspaceLayout.saveState()
    // The approved presentation is adopted once, including checkpointed windows.
    // Future launches restore each window's subsequently chosen layout as usual.
    readonly property bool adoptReferenceWorkspace: typeof referenceWorkspaceUpgrade !== "undefined" && referenceWorkspaceUpgrade
    function restoreWorkspacePaneState(state) {
        workspaceLayout.restoreState(state);
        workspaceSettings.showMarkup = true;
        if (adoptReferenceWorkspace) applyReferenceWorkspace();
    }
    function openAnchoredMenu(menu, anchor) {
        var point = anchor.mapToItem(win.contentItem, 0, anchor.height + 5);
        menu.parent = win.contentItem;
        menu.x = Math.max(8, Math.min(win.width - menu.width - 8, point.x));
        // Reserve the top margin and trigger gap before sizing the scrolling
        // theme menu. Position using its capped height, not the full item list.
        var anchorTop = anchor.mapToItem(win.contentItem, 0, 0).y;
        if (menu === previewTemplateMenu) menu.maximumPopupHeight = Math.max(0, anchorTop - 13);
        var menuHeight = menu.height;
        // Footer menus open above their button so the trigger stays visible and
        // the opening click does not land over the last menu command.
        var above = anchorTop - menuHeight - 5;
        var top = point.y + menuHeight > win.contentItem.height - 8 && above >= 8
                ? above : point.y;
        menu.y = Math.max(8, Math.min(win.contentItem.height - menuHeight - 8, top));
        menu.open();
    }
    function hideWorkspacePane(pane) {
        if (pane === "organizer") workspaceLayout.organizerVisible = false;
        else workspaceLayout.filesVisible = false;
        navigationDrawer.close();
        focusWritingSurface();
    }
    function toggleWorkspacePane(pane) {
        var effective = pane === "organizer" ? workspaceLayout.effectiveOrganizerVisible : workspaceLayout.effectiveFilesVisible;
        if (effective) { hideWorkspacePane(pane); return; }
        if (pane === "organizer") workspaceLayout.organizerVisible = true;
        else workspaceLayout.filesVisible = true;
        Qt.callLater(function() {
            // A second toggle can hide the pane before this deferred layout check.
            // Do not reopen temporary navigation for a canceled reveal request.
            var requested = pane === "organizer" ? workspaceLayout.organizerVisible : workspaceLayout.filesVisible;
            if (!requested) return;
            var docked = pane === "organizer" ? workspaceLayout.effectiveOrganizerVisible : workspaceLayout.effectiveFilesVisible;
            if (!docked) {
                navigationDrawer.paneName = pane;
                navigationDrawer.open();
            }
        });
    }
    function equalizeDocumentPanes() {
        if (workspaceLayout.effectiveLayoutMode !== 1) return;
        beginDocumentViewportTransition(true);
        var total = editorPane.width + previewPane.width;
        workspaceLayout.updateWidth("preview", Math.max(320, Math.min(total / 2, total - 480)));
    }
    // Layout and editing choice are independent. Preview-only is a reading
    // action; returning to editing restores the last chosen arrangement.
    function setDocumentView(mode) {
        if (mode === 1 && !documentFooter.canSplit) return;
        if (mode === 2 && searchOpen) closeSearch(false);
        beginDocumentViewportTransition();
        workspaceLayout.layoutMode = mode;
        if (mode === 1) workspaceLayout.recalculate(true);
        documentViewportTransition.focusTarget = mode === 2 ? "preview"
            : workspaceLayout.liveEditEnabled ? "live" : "source";
    }
    function setLiveEditing(enabled) {
        if (!enabled) { workspaceLayout.liveEditEnabled = false; return; }
        if (searchOpen) closeSearch(false);
        liveEditorLoader.cursorOnEnter = editor.cursorPosition;
        workspaceLayout.liveEditEnabled = true;
        if (workspaceLayout.layoutMode === 2)
            workspaceLayout.layoutMode = workspaceLayout.lastEditingLayoutMode;
        lastWritingSurface = "live";
        sourceFormattingOwned = false;
        if (liveEditorLoader.item) liveEditorLoader.item.focusLive();
    }
    function setSourceEditing() {
        if (workspaceLayout.liveEditEnabled) {
            // Carry the Live caret back to the Source editor, but only when it
            // moved: an untouched page leaves the Source caret and selection alone.
            var liveCursor = backend.liveCursor();
            var moved = backend.editorBridge.caretMovedByUser();
            workspaceLayout.liveEditEnabled = false;
            if (moved && liveCursor >= 0) editor.cursorPosition = Math.min(liveCursor, editor.length);
        }
        if (workspaceLayout.effectiveLayoutMode !== 2) {
            focusWritingSurface();
            return;
        }
        beginDocumentViewportTransition();
        if (workspaceLayout.layoutMode === 2)
            workspaceLayout.layoutMode = workspaceLayout.lastEditingLayoutMode;
        lastWritingSurface = "source";
        lastZoomPane = "source";
        sourceFormattingOwned = false;
        documentViewportTransition.focusTarget = "source";
    }
    // Kept for callers that still pass the retired Visual Edit flag.
    function setEditingMode(visual) { setSourceEditing(); }
    function selectWritingMode(mode) {
        if (mode === "preview") setDocumentView(2);
        else if (mode === "live") setLiveEditing(true);
        else setSourceEditing();
    }
    function applyStudioWorkspace() {
        backend.themePreset = "studio";
        workspaceSettings.writingAppearance = "editorial";
        workspaceSettings.writingSize = 16;
        paneZoom.resetAll();
        workspaceSettings.toolbarVisibilityMode = 1;
        workspaceLayout.restoreDefaults();
        workspaceLayout.layoutMode = 0;
        workspaceLayout.liveEditEnabled = false;
        libraryPane.showExcerpts = true;
        libraryPane.dateMode = 1;
    }

    function applyReferenceWorkspace() {
        backend.themePreset = "studio";
        workspaceSettings.writingAppearance = "manuscript";
        workspaceSettings.writingSize = 16;
        paneZoom.resetAll();
        workspaceSettings.toolbarVisibilityMode = 1;
        workspaceSettings.toolbarMode = 0;
        workspaceSettings.titleBarMode = 1;
        workspaceSettings.typewriter = false;
        workspaceSettings.showMarkup = true;
        workspaceLayout.restoreState({ version: 1, organizerVisible: true, filesVisible: true,
            layoutMode: 0, liveEditEnabled: false, organizerWidth: 184, fileWidth: 232,
            previewWidth: workspaceLayout.previewWidth });
        libraryPane.showExcerpts = true;
        libraryPane.dateMode = 1;
        libraryPane.showSortBar = true;
        backend.library.sortMode = 1;
        backend.library.ascending = false;
        libraryPane.showFilterBar = false;
        lastWritingSurface = "source";
    }

    Settings {
        id: workspaceSettings
        objectName: "workspaceSettings"
        category: "workspace"
        property bool libraryVisible: true
        property bool organizerVisible: true
        property int layoutMode: 1
        property int writingSize: 16 // Legacy migration only; pane zoom now owns screen size.
        property int sourceZoom: 100
        property int previewZoom: 100
        property bool linkedZoom: false
        property int zoomRevision: 0
        property string writingAppearance: "editorial"
        property int appearanceRevision: 0
        property bool showMarkup: true // Legacy preference; Source now always shows syntax.
        property string reviewWords: ""
        onReviewWordsChanged: if (styleCheckCustom) customReviewRefreshTimer.restart()
        property bool styleCheckCustom: false
        onStyleCheckCustomChanged: {
            if (styleCheckCustom) customReviewRefreshTimer.restart();
            else win.applyStyleReviewNow();
        }
        property bool styleCheckFillers: false
        onStyleCheckFillersChanged: {
            if (styleCheckFillers) customReviewRefreshTimer.restart();
            else win.applyStyleReviewNow();
        }
        property string authorshipProfileName: ""
        property string authorshipProfileIdentifier: ""
        property bool smartQuotes: false
        property bool smartDashes: false
        property bool autosaveEnabled: false
        property bool synchronizedScroll: false
        property bool typewriter: false
        // Live follows the Output Style with its layout filtered out; this
        // switch applies the theme exactly instead (docs/live-appearance-design.md).
        property bool liveThemeExact: false
        property bool sentenceFocus: false
        onSentenceFocusChanged: backend.setFocusPosition(editor.cursorPosition, paragraphFocus, sentenceFocus)
        property bool paragraphFocus: false
        onParagraphFocusChanged: backend.setFocusPosition(editor.cursorPosition, paragraphFocus, sentenceFocus)
        // Retain the legacy setting for migration; Title Bar now uses autoHideChrome.
        property int titleBarMode: 1
        property int toolbarVisibilityMode: 1
        property string publishingFormat: "web"
        property bool autoHideChrome: true
        onAutoHideChromeChanged: if (!autoHideChrome) win.documentChromeHidden = false
        property int toolbarMode: 0
        property bool toolbarCharacters: true
        property bool toolbarCharactersNoSpaces: true
        property bool toolbarWords: true
        property bool toolbarSentences: true
        property bool toolbarReadingTime: true
        property bool toolbarSpeakingTime: true
        property bool toolbarTasks: true
        property bool toolbarHuman: true
        property bool toolbarAI: true
        property bool toolbarReference: true
        property bool automaticVersions: false
        onShowMarkupChanged: backend.setShowMarkup(true)
    }

    PaneZoomState {
        id: paneZoom
        settings: workspaceSettings
        legacyAppearanceOffset: win.writingBasePixelSize - 16
        effectivePane: workspaceLayout.effectiveLayoutMode === 0 ? "source"
            : workspaceLayout.effectiveLayoutMode === 2 ? "preview" : win.lastZoomPane
        onBeforeZoomChanged: win.beginDocumentViewportTransition(true)
    }

    WorkspaceCommands {
        id: workspaceCommands
        zoomController: paneZoom
        settings: workspaceSettings
        layoutState: workspaceLayout
        library: backend.library
        libraryPane: libraryPane
        window: win
        editor: editor
        preview: previewPane
        onOutlineRequested: {
            outlineDrawer.headings = backend.documentOutline(editor.text);
            outlineDrawer.showFor(workspaceLayout.effectiveLayoutMode !== 2 && win.lastWritingSurface === "source" ? editor.cursorPosition
                : workspaceLayout.effectiveLayoutMode !== 2 && workspaceLayout.liveEditEnabled ? backend.liveCursor() : -1,
                win.isInside(win.activeFocusItem, editor) || win.isInside(win.activeFocusItem, previewPane) ? win.activeFocusItem : null);
        }
        onStatisticsRequested: statisticsDialog.open()
        onTypewriterChanged: editorFlick.ensureCursorVisible()
        onCommandRequested: function(id) {
            switch (id) {
            case "new": win.requestNewDocument(); break;
            case "newWindow": backend.newWindow(); break;
            case "open": backend.openDialog(); break;
            case "openPath": openPathDialog.open(); break;
            case "save": backend.save(); break;
            case "saveAs": backend.saveAsDialog(); break;
            case "duplicate": fileNameDialog.showFor(false); break;
            case "rename": fileNameDialog.showFor(true); break;
            case "move": moveFolderDialog.currentFolder=backend.documentBaseUrl; moveFolderDialog.open(); break;
            case "reveal": win.showCurrentFileInLibrary(); break;
            case "quickOpen": win.openQuickSearchState("", false, backend.library.rootFolder, false); break;
            case "refreshTags": backend.library.refreshTags(); break;
            case "exportHtml": win.openExportHub("html"); break;
            case "printPreview": backend.printPreview(); break;
            case "exportPdf": win.openExportHub("pdf"); break;
            case "pageBreak": win.tryInsertSourceSnippet("\n\n<!-- pagebreak -->\n\n"); break;
            case "writingReview": analysisDialog.open(); break;
            case "spelling": spellingDialog.open(); break;
            case "authorship": authorshipDialog.ranges=backend.authorshipRanges(); authorshipDialog.open(); break;
            case "themeSystem": backend.themePreset="system"; break;
            case "themeLight": backend.themePreset="light"; break;
            case "themeDark": backend.themePreset="dark"; break;
            case "themePaper": backend.themePreset="paper"; break;
            case "themeStudio": backend.themePreset="studio"; break;
            }
        }
    }

    // The native menu is created in a Loader component with its own id scope.
    property var nativePublishingMenu: null

    // Menu controls toggle themselves before triggered. Restore every checked
    // binding after validation, including rejected and repeated selections.
    component PublishingMenuItem: CompactMenuItem {
        id: choice
        property string publishingId: ""
        property int basicStyle: -1
        readonly property bool backendSelected: publishingId.length > 0
            ? backend.publishingThemeId === publishingId
            : backend.publishingThemeId.length === 0 && backend.outputStyle === basicStyle
        checkable: true
        checked: backendSelected
        function restoreSelectionCheck() { checked = Qt.binding(function() { return choice.backendSelected; }); }
        onTriggered: win.choosePublishingStyle(publishingId, basicStyle)
    }
    function restorePublishingMenuChecks(menu, nativeMenu) {
        if (!menu) return;
        var count = nativeMenu ? menu.items.length : menu.count;
        for (var i = 0; i < count; ++i) {
            var item = nativeMenu ? menu.items[i] : menu.itemAt(i);
            if (!item) continue;
            if (item.restoreSelectionCheck) item.restoreSelectionCheck();
            if (item.subMenu) restorePublishingMenuChecks(item.subMenu, nativeMenu);
        }
    }
    function restorePublishingChecks() {
        restorePublishingMenuChecks(previewTemplateMenu, false);
        restorePublishingMenuChecks(nativePublishingMenu, true);
        for (var k = 0; k < writingOptions.count; ++k) {
            var writingItem = writingOptions.itemAt(k);
            if (writingItem && writingItem.restoreSelectionCheck) writingItem.restoreSelectionCheck();
        }
    }
    function choosePublishingStyle(id, style) {
        if (id.length > 0) {
            if (!backend.selectPublishingTheme(id)) showPublishingThemeNotice(backend.publishingThemeError || backend.status);
        } else backend.setOutputStyle(style);
        restorePublishingChecks();
    }
    function showPublishingThemeNotice(message) {
        if (!message) return;
        publishingThemeNotice.message = message;
        publishingThemeNotice.open();
    }
    function reloadPublishingThemes() {
        backend.reloadPublishingThemes();
        restorePublishingChecks();
        if (backend.publishingThemeError) showPublishingThemeNotice(backend.publishingThemeError);
    }
    function openPublishingThemesFolder() {
        if (!backend.openPublishingThemesFolder()) showPublishingThemeNotice(backend.publishingThemeError || backend.status);
    }

    function openExportHub(format) {
        exportHub.selectedFormat = format || "pdf";
        exportHub.open();
    }

    WorkspaceHeader {
        id: topChrome
        activityHidden: workspaceSettings.autoHideChrome && win.documentChromeHidden
        menuOpen: win.documentChromeMenuOpen
        documentName: win.appBackend.fileName
        documentModified: win.appBackend.modified
        zoomController: paneZoom
        anchors.top: parent.top
        width: parent.width
        window: win
        settings: workspaceSettings
        layoutState: workspaceLayout
        organizerSlot: organizerSlot
        filesSlot: filesSlot
        editorPane: editorPane
        previewPane: previewPane
        onActionRequested: function(action, anchor) {
            switch (action) {
            case "addLocation": organizerPane.chooseLocation(); break;
            case "hideOrganizer": win.hideWorkspacePane("organizer"); break;
            case "hideFiles": win.hideWorkspacePane("files"); break;
            case "toggleOrganizer": workspaceCommands.run("organizer"); break;
            case "toggleFiles": workspaceCommands.run("library"); break;
            case "chooseFolder": libraryPane.chooseFolder(); break;
            case "newDocument": libraryPane.newDocument(); break;
            case "libraryOptions": win.openAnchoredMenu(libraryActions, anchor); break;
            case "back": win.requestHistory(-1); break;
            case "forward": win.requestHistory(1); break;
            case "find": win.openSearch(false, false); break;
            case "bold": win.tryWrapSelection("**", "**"); break;
            case "italic": win.tryWrapSelection("*", "*"); break;
            case "link": win.openLinkEditor(anchor); break;
            case "format": win.openQuickFormat(anchor); break;
            case "smaller": workspaceCommands.run("smaller"); break;
            case "larger": workspaceCommands.run("larger"); break;
            case "appearance": win.openAnchoredMenu(writingOptions, anchor); break;
            case "workspace": win.openAnchoredMenu(workspaceMenu, anchor); break;
            case "export": win.openExportHub("pdf"); break;
            case "hidePreview": win.setDocumentView(0); break;
            }
        }
    }

    Timer { id: navigationNoticeTimer; interval: 7000; onTriggered: win.navigationNotice = "" }
    Rectangle {
        objectName: "navigationNotice"
        visible: win.navigationNotice !== ""
        z: 20
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: documentFooter.height + 10
        width: Math.min(440, parent.width - 32)
        height: navigationNoticeLabel.implicitHeight + 28
        radius: 8
        color: backend.palette.panel
        border.color: backend.palette.border
        Label {
            id: navigationNoticeLabel
            anchors.left: parent.left; anchors.right: noticeDismiss.left
            anchors.leftMargin: 14; anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: win.navigationNotice
            wrapMode: Text.WordWrap
            color: backend.palette.text
            font.pixelSize: 13
            Accessible.name: text
        }
        ToolbarButton {
            id: noticeDismiss
            anchors.right: parent.right; anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            text: "×"
            hint: "Dismiss message"
            onClicked: win.navigationNotice = ""
        }
    }

    DocumentOutline {
        id: outlineDrawer
        onFocusRestoreRequested: {
            win.focusWritingSurface();
        }
        onJumpRequested: function(position) {
            if (workspaceLayout.liveEditEnabled && liveEditorLoader.item) {
                // Stay in Live: the page places and reveals its own caret.
                backend.editorBridge.placeCursor(position);
                backend.editorBridge.noteCaretMovedByUser(); // the writer asked for this move
                liveEditorLoader.item.focusLive();
                return;
            }
            win.setEditingMode(false);
            win.cancelDocumentViewportTransition();
            win.lastWritingSurface = "source";
            editor.cursorPosition = position;
            editor.forceActiveFocus();
            Qt.callLater(editorFlick.ensureCursorVisible);
        }
    }

    Dialog {
        id: statisticsDialog
        objectName: "statisticsDialog"
        property var statistics: win.documentStatistics
        title: "Document statistics"
        anchors.centerIn: parent
        width: 300
        modal: true
        standardButtons: Dialog.Close
        onOpened: win.refreshDocumentStatistics()
        Label {
            text: (statisticsDialog.statistics.words || 0) + " words\n"
                + (statisticsDialog.statistics.characters || 0) + " characters\n"
                + (statisticsDialog.statistics.charactersWithoutSpaces || 0) + " excluding whitespace\n"
                + (statisticsDialog.statistics.sentences || 0) + " sentences\n"
                + (statisticsDialog.statistics.tasks || 0) + " tasks\n\n"
                + (statisticsDialog.statistics.readingMinutes || 0) + " min estimated reading time\n"
                + (statisticsDialog.statistics.speakingMinutes || 0) + " min estimated speaking time\n\n"
                + (statisticsDialog.statistics.humanWords || 0) + " Human words\n"
                + (statisticsDialog.statistics.aiWords || 0) + " AI words\n"
                + (statisticsDialog.statistics.referenceWords || 0) + " Reference words\n"
                + "Authorship counts are manual assertions."
            lineHeight: 1.6
        }
    }

    Popup {
        id: completionPopup
        objectName: "completionPopup"
        width: 260
        height: Math.min(240, completionList.contentHeight + 12)
        padding: 6
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            color: backend.palette.panel
            border.color: backend.palette.border
            radius: 5
        }
        ListView {
            id: completionList
            objectName: "completionList"
            anchors.fill: parent
            clip: true
            model: win.completionItems
            currentIndex: 0
            keyNavigationWraps: true
            Keys.onReturnPressed: function(event) { event.accepted = win.acceptCompletion(currentIndex); }
            Keys.onEnterPressed: function(event) { event.accepted = win.acceptCompletion(currentIndex); }
            Keys.onEscapePressed: function(event) {
                completionPopup.close();
                editor.forceActiveFocus();
                event.accepted = true;
            }
            delegate: ItemDelegate {
                required property string modelData
                required property int index
                objectName: "completionItem_" + index
                width: completionList.width
                height: 30
                text: modelData
                highlighted: ListView.isCurrentItem
                onClicked: win.acceptCompletion(index)
            }
        }
    }

    LinkEditor {
        id: linkEditor
        hostWindow: win
        sourceEditor: editor
        sourceViewport: editorFlick
        documentBackend: backend
    }

    CompactMenu {
        id: formatQuickMenu
        objectName: "quickFormatMenu"
        width: 230
        CompactMenuItem { enabled: win.canFormatSource; text: "Bold"; onTriggered: win.tryWrapSelection("**", "**") }
        CompactMenuItem { enabled: win.canFormatSource; text: "Italic"; onTriggered: win.tryWrapSelection("*", "*") }
        CompactMenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Link…"; onTriggered: win.tryInsertLink() }
        MenuSeparator {}
        CompactMenuItem { enabled: win.canFormatSource; text: "Body"; onTriggered: win.editMarkdown("body") }
        CompactMenuItem { enabled: win.canFormatSource; text: "Heading 1"; onTriggered: win.editMarkdown("heading1") }
        CompactMenuItem { enabled: win.canFormatSource; text: "Heading 2"; onTriggered: win.editMarkdown("heading2") }
        CompactMenuItem { enabled: win.canFormatSource; text: "Heading 3"; onTriggered: win.editMarkdown("heading3") }
        MenuSeparator {}
        CompactMenuItem { enabled: win.canFormatSource; text: "Blockquote"; onTriggered: win.editMarkdown("quote") }
        CompactMenuItem { enabled: win.canFormatSource; text: "Bullet List"; onTriggered: win.editMarkdown("bullet") }
        CompactMenuItem { enabled: win.canFormatSource; text: "Ordered List"; onTriggered: win.editMarkdown("ordered") }
        CompactMenuItem { enabled: win.canFormatSource; text: "Task List"; onTriggered: win.editMarkdown("task") }
    }

    CompactMenu {
        id: formatPopover
        x: Math.max(0, editorPane.x + 12)
        y: Math.max(0, win.contentItem.height - height - 38)
        CompactMenuItem { objectName: "saveButton"; text: "Save"; onTriggered: backend.save() }
        CompactMenuItem { objectName: "openButton"; text: "Open…"; onTriggered: backend.openDialog() }
        CompactMenuItem { text: "Open by Path…"; onTriggered: openPathDialog.open() }
        MenuSeparator {}
        CompactMenuItem { enabled: win.canFormatSource; text: "Strikethrough"; onTriggered: workspaceCommands.run("strike") }
        CompactMenuItem { enabled: win.canFormatSource; text: "Inline code"; onTriggered: workspaceCommands.run("inlineCode") }
    }
    // Right-click on an underlined word in the Source editor: suggestions, then
    // Learn Spelling and Ignore Spelling. Replacements go through
    // backend.correctWriting, one undoable edit.
    CompactMenu {
        id: spellingMenu
        objectName: "spellingContextMenu"
        property int wordStart: -1
        property int wordEnd: -1
        property string word: ""
        property var guesses: []
        function showFor(point, hit) {
            wordStart = hit.start; wordEnd = hit.end; word = hit.word;
            guesses = backend.spellCheck.suggestions(hit.word).slice(0, 5);
            popup(editor, point.x, point.y);
        }
        function replaceWith(replacement) {
            var ok = false;
            win.performDocumentEdit(function() { ok = backend.correctWriting(wordStart, wordEnd, word, replacement); });
            if (ok) editor.cursorPosition = wordStart + replacement.length;
            editor.forceActiveFocus();
        }
        onClosed: if (win.sourceEditorVisible) editor.forceActiveFocus()
        CompactMenuItem { objectName: "spellingSuggestion0"; visible: spellingMenu.guesses.length > 0; height: visible ? implicitHeight : 0
            text: spellingMenu.guesses.length > 0 ? spellingMenu.guesses[0] : ""; onTriggered: spellingMenu.replaceWith(text) }
        CompactMenuItem { objectName: "spellingSuggestion1"; visible: spellingMenu.guesses.length > 1; height: visible ? implicitHeight : 0
            text: spellingMenu.guesses.length > 1 ? spellingMenu.guesses[1] : ""; onTriggered: spellingMenu.replaceWith(text) }
        CompactMenuItem { objectName: "spellingSuggestion2"; visible: spellingMenu.guesses.length > 2; height: visible ? implicitHeight : 0
            text: spellingMenu.guesses.length > 2 ? spellingMenu.guesses[2] : ""; onTriggered: spellingMenu.replaceWith(text) }
        CompactMenuItem { objectName: "spellingSuggestion3"; visible: spellingMenu.guesses.length > 3; height: visible ? implicitHeight : 0
            text: spellingMenu.guesses.length > 3 ? spellingMenu.guesses[3] : ""; onTriggered: spellingMenu.replaceWith(text) }
        CompactMenuItem { objectName: "spellingSuggestion4"; visible: spellingMenu.guesses.length > 4; height: visible ? implicitHeight : 0
            text: spellingMenu.guesses.length > 4 ? spellingMenu.guesses[4] : ""; onTriggered: spellingMenu.replaceWith(text) }
        CompactMenuItem { objectName: "spellingNoGuesses"; visible: spellingMenu.guesses.length === 0; height: visible ? implicitHeight : 0
            enabled: false; text: "No Guesses Found" }
        MenuSeparator {}
        CompactMenuItem { objectName: "spellingLearn"; text: "Learn Spelling"
            onTriggered: { backend.spellCheck.learnWord(spellingMenu.word); editor.forceActiveFocus(); } }
        CompactMenuItem { objectName: "spellingIgnore"; text: "Ignore Spelling"
            onTriggered: { backend.spellCheck.ignoreWord(spellingMenu.word); editor.forceActiveFocus(); } }
    }
    // Right-click on a blue (grammar) range in the Source editor: the checker's
    // message as a disabled first line, up to five corrections, then Ignore
    // Grammar Issue. There is no Learn: a sentence is not a word. Corrections
    // go through backend.correctWriting, one undoable edit.
    CompactMenu {
        id: grammarMenu
        objectName: "grammarContextMenu"
        width: 320
        property int rangeStart: -1
        property int rangeEnd: -1
        property string original: ""
        property string message: ""
        property var corrections: []
        function showFor(point, hit) {
            rangeStart = hit.start; rangeEnd = hit.end; original = hit.word;
            message = hit.message && hit.message.length > 0 ? hit.message : "Grammar";
            corrections = hit.suggestions.slice(0, 5);
            popup(editor, point.x, point.y);
        }
        function replaceWith(replacement) {
            var ok = false;
            win.performDocumentEdit(function() { ok = backend.correctWriting(rangeStart, rangeEnd, original, replacement); });
            if (ok) editor.cursorPosition = rangeStart + replacement.length;
            editor.forceActiveFocus();
        }
        onClosed: if (win.sourceEditorVisible) editor.forceActiveFocus()
        CompactMenuItem { objectName: "grammarMessage"; enabled: false; text: grammarMenu.message }
        CompactMenuItem { objectName: "grammarSuggestion0"; visible: grammarMenu.corrections.length > 0; height: visible ? implicitHeight : 0
            text: grammarMenu.corrections.length > 0 ? grammarMenu.corrections[0] : ""; onTriggered: grammarMenu.replaceWith(text) }
        CompactMenuItem { objectName: "grammarSuggestion1"; visible: grammarMenu.corrections.length > 1; height: visible ? implicitHeight : 0
            text: grammarMenu.corrections.length > 1 ? grammarMenu.corrections[1] : ""; onTriggered: grammarMenu.replaceWith(text) }
        CompactMenuItem { objectName: "grammarSuggestion2"; visible: grammarMenu.corrections.length > 2; height: visible ? implicitHeight : 0
            text: grammarMenu.corrections.length > 2 ? grammarMenu.corrections[2] : ""; onTriggered: grammarMenu.replaceWith(text) }
        CompactMenuItem { objectName: "grammarSuggestion3"; visible: grammarMenu.corrections.length > 3; height: visible ? implicitHeight : 0
            text: grammarMenu.corrections.length > 3 ? grammarMenu.corrections[3] : ""; onTriggered: grammarMenu.replaceWith(text) }
        CompactMenuItem { objectName: "grammarSuggestion4"; visible: grammarMenu.corrections.length > 4; height: visible ? implicitHeight : 0
            text: grammarMenu.corrections.length > 4 ? grammarMenu.corrections[4] : ""; onTriggered: grammarMenu.replaceWith(text) }
        MenuSeparator {}
        CompactMenuItem { objectName: "grammarIgnore"; text: "Ignore Grammar Issue"
            onTriggered: { backend.ignoreGrammarIssue(grammarMenu.rangeStart, grammarMenu.rangeEnd); editor.forceActiveFocus(); } }
    }
    CompactMenu {
        id: writingOptions
        objectName: "writingOptions"
        width: 270
        CompactMenuItem { text: "Reference writing layout"; onTriggered: win.applyReferenceWorkspace() }
        CompactMenuItem { text: "Studio writing layout"; onTriggered: win.applyStudioWorkspace() }
        MenuSeparator {}
        x: Math.max(0, win.width - width - 160)
        y: 44
        CompactMenu { title: "Theme"
            CompactMenuItem { objectName: "themeSystem"; text: "Follow system (reset)"; checkable: true; checked: backend.themePreset === "system"; onTriggered: backend.themePreset = "system" }
            CompactMenuItem { objectName: "themeLight"; text: "Light"; checkable: true; checked: backend.themePreset === "light"; onTriggered: backend.themePreset = "light" }
            CompactMenuItem { objectName: "themeDark"; text: "Dark"; checkable: true; checked: backend.themePreset === "dark"; onTriggered: backend.themePreset = "dark" }
            CompactMenuItem { objectName: "themePaper"; text: "Warm paper"; checkable: true; checked: backend.themePreset === "paper"; onTriggered: backend.themePreset = "paper" }
            CompactMenuItem { objectName: "themeStudio"; text: "Studio"; checkable: true; checked: backend.themePreset === "studio"; onTriggered: backend.themePreset = "studio" }
        }
        CompactMenu { title: "Writing appearance"
            CompactMenuItem { text: "Manuscript"; checkable: true; checked: win.activeWritingAppearance === "manuscript"; onTriggered: workspaceCommands.run("writingManuscript") }
            CompactMenuItem { text: "Editorial"; checkable: true; checked: win.activeWritingAppearance === "editorial"; onTriggered: workspaceCommands.run("writingEditorial") }
            CompactMenuItem { text: "Book"; checkable: true; checked: win.activeWritingAppearance === "book"; onTriggered: workspaceCommands.run("writingBook") }
            CompactMenuItem { text: "Code"; checkable: true; checked: win.activeWritingAppearance === "code"; onTriggered: workspaceCommands.run("writingCode") }
        }
        MenuSeparator {}
        CompactMenuItem { text: "Paragraph focus"; checkable: true; checked: workspaceSettings.paragraphFocus; onTriggered: workspaceCommands.run("paragraph") }
        CompactMenuItem { text: "Typewriter scrolling (Source)"; checkable: true; checked: workspaceSettings.typewriter; onTriggered: workspaceCommands.run("typewriter") }
        MenuSeparator {}
        CompactMenuItem { text: "Larger text"; enabled: paneZoom.activeZoom < paneZoom.maximumZoom; onTriggered: workspaceCommands.run("larger") }
        CompactMenuItem { text: "Smaller text"; enabled: paneZoom.activeZoom > paneZoom.minimumZoom; onTriggered: workspaceCommands.run("smaller") }
        CompactMenuItem { text: "Reset text size"; onTriggered: workspaceCommands.run("resetSize") }
        MenuSeparator {}
        PublishingMenuItem { text: "Basic: Modern"; basicStyle: 0 }
        PublishingMenuItem { text: "Basic: Classic"; basicStyle: 1 }
        PublishingMenuItem { text: "Basic: Manuscript"; basicStyle: 2 }
    }

    CompactMenu {
        id: sourceAppearanceMenu
        objectName: "sourceAppearanceMenu"
        width: 220
        CompactMenuItem { objectName: "sourceAppearanceManuscript"; text: "Manuscript"; checkable: true; autoExclusive: true; checked: win.activeWritingAppearance === "manuscript"; onTriggered: workspaceCommands.run("writingManuscript") }
        CompactMenuItem { objectName: "sourceAppearanceEditorial"; text: "Editorial"; checkable: true; autoExclusive: true; checked: win.activeWritingAppearance === "editorial"; onTriggered: workspaceCommands.run("writingEditorial") }
        CompactMenuItem { objectName: "sourceAppearanceBook"; text: "Book"; checkable: true; autoExclusive: true; checked: win.activeWritingAppearance === "book"; onTriggered: workspaceCommands.run("writingBook") }
        CompactMenuItem { objectName: "sourceAppearanceCode"; text: "Code"; checkable: true; autoExclusive: true; checked: !workspaceLayout.liveEditEnabled && win.activeWritingAppearance === "code"; onTriggered: workspaceCommands.run("writingCode") }
        MenuSeparator {}
        CompactMenuItem { objectName: "sourceAppearanceLarger"; text: "Larger text"; enabled: paneZoom.sourceZoom < paneZoom.maximumZoom; onTriggered: paneZoom.adjustZoom("source", 10) }
        CompactMenuItem { objectName: "sourceAppearanceSmaller"; text: "Smaller text"; enabled: paneZoom.sourceZoom > paneZoom.minimumZoom; onTriggered: paneZoom.adjustZoom("source", -10) }
        CompactMenuItem { objectName: "sourceAppearanceReset"; text: "Reset text size"; onTriggered: paneZoom.resetZoom("source") }
    }
    CompactMenu {
        id: previewTemplateMenu
        objectName: "previewTemplateMenu"
        width: 260
        cascade: true
        delegate: CompactMenuItem { iconName: "" }
        property real maximumPopupHeight: Math.max(0, win.contentItem.height - 32)
        height: Math.min(implicitHeight, maximumPopupHeight)
        onAboutToShow: win.restorePublishingChecks()
        contentItem: ListView {
            implicitHeight: contentHeight
            model: previewTemplateMenu.contentModel
            currentIndex: previewTemplateMenu.currentIndex
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        }
        CompactMenu {
            id: previewCustomThemesMenu
            objectName: "previewCustomThemesMenu"
            title: "Custom Themes"
            cascade: true
            width: 260
            height: Math.min(implicitHeight, Math.max(0, win.contentItem.height - 24))
            margins: 8
            onAboutToShow: win.restorePublishingChecks()
            contentItem: ListView {
                implicitHeight: contentHeight
                model: previewCustomThemesMenu.contentModel
                currentIndex: previewCustomThemesMenu.currentIndex
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            }
            Instantiator {
                model: backend.publishingThemes
                delegate: PublishingMenuItem {
                    required property var modelData
                    objectName: "previewPublishingTheme_" + modelData.id
                    text: modelData.id === "claude-like" ? "Claude Like" : modelData.name
                    publishingId: modelData.id
                }
                onObjectAdded: function(index, object) { previewCustomThemesMenu.insertItem(index, object); }
                onObjectRemoved: function(index, object) { previewCustomThemesMenu.removeItem(object); }
            }
        }
        MenuSeparator {}
        CompactMenuItem { text: "Basic Font & Page Settings"; enabled: false }
        PublishingMenuItem { objectName: "previewTemplate0"; text: "Modern (Sans)"; basicStyle: 0 }
        PublishingMenuItem { objectName: "previewTemplate1"; text: "Classic (Serif)"; basicStyle: 1 }
        PublishingMenuItem { objectName: "previewTemplate2"; text: "Manuscript (Mono)"; basicStyle: 2 }
        MenuSeparator {}
        PublishingMenuItem { objectName: "previewTemplate4"; text: "GitHub"; basicStyle: 4 }
        PublishingMenuItem { objectName: "previewTemplate5"; text: "Helvetica"; basicStyle: 5 }
        PublishingMenuItem { objectName: "previewTemplate6"; text: "Palatino"; basicStyle: 6 }
        PublishingMenuItem { objectName: "previewTemplate7"; text: "MLA Draft"; basicStyle: 7 }
        MenuSeparator {}
        PublishingMenuItem { objectName: "previewTemplate3"; text: "Custom Settings"; basicStyle: 3 }
        CompactMenuItem { objectName: "previewTemplateCustomEdit"; text: "Edit Basic Settings…"; onTriggered: win.openExportHub("pdf") }
        CompactMenuItem { objectName: "previewTemplateCustomLoad"; text: "Load Custom Settings…"; onTriggered: outputStyleDialog.open() }
        MenuSeparator {}
        CompactMenuItem { objectName: "previewGetMoreThemes"; text: "Get More Themes…"; onTriggered: Qt.openUrlExternally("https://theme.typora.io/") }
        CompactMenuItem { objectName: "previewThemeImport"; text: "Import Theme…"; onTriggered: publishingThemeDialog.open() }
        CompactMenuItem { objectName: "previewThemesFolder"; text: "Open Themes Folder"; onTriggered: win.openPublishingThemesFolder() }
        CompactMenuItem { objectName: "previewThemesReload"; text: "Reload Themes"; onTriggered: win.reloadPublishingThemes() }
    }
    CompactMenu {
        id: workspaceMenu
        objectName: "workspaceMenu"
        width: 258
        function restoreViewChecks() {
            workspaceSourceView.checked = Qt.binding(function() { return workspaceLayout.effectiveLayoutMode !== 2 && !workspaceLayout.liveEditEnabled; });
            workspaceLiveView.checked = Qt.binding(function() { return workspaceLayout.effectiveLayoutMode !== 2 && workspaceLayout.liveEditEnabled; });
            workspaceSingleView.checked = Qt.binding(function() { return workspaceLayout.effectiveLayoutMode === 0; });
            workspaceSplitView.checked = Qt.binding(function() { return workspaceLayout.effectiveLayoutMode === 1; });
            workspacePreviewView.checked = Qt.binding(function() { return workspaceLayout.effectiveLayoutMode === 2; });
        }
        CompactMenuItem { objectName: "workspaceOrganizer"; text: workspaceCommands.label("organizer"); checkable: true; checked: workspaceLayout.effectiveOrganizerVisible; onTriggered: win.toggleWorkspacePane("organizer") }
        CompactMenuItem { objectName: "workspaceFiles"; text: workspaceCommands.label("library"); checkable: true; checked: workspaceLayout.effectiveFilesVisible; onTriggered: win.toggleWorkspacePane("files") }
        MenuSeparator {}
        CompactMenuItem { id: workspaceSourceView; objectName: "workspaceSourceView"; text: "Source"; checkable: true; checked: workspaceLayout.effectiveLayoutMode !== 2 && !workspaceLayout.liveEditEnabled; onTriggered: { win.selectWritingMode("source"); workspaceMenu.restoreViewChecks(); } }
        CompactMenuItem { id: workspaceLiveView; objectName: "workspaceLiveView"; text: "Live"; checkable: true; checked: workspaceLayout.effectiveLayoutMode !== 2 && workspaceLayout.liveEditEnabled; onTriggered: { win.selectWritingMode("live"); workspaceMenu.restoreViewChecks(); } }
        MenuSeparator {}
        CompactMenuItem { id: workspaceSingleView; objectName: "workspaceSingleView"; text: "Single"; checkable: true; checked: workspaceLayout.effectiveLayoutMode === 0; onTriggered: { win.setDocumentView(0); workspaceMenu.restoreViewChecks(); } }
        CompactMenuItem { id: workspaceSplitView; objectName: "workspaceSplitView"; text: "Split"; checkable: true; checked: workspaceLayout.effectiveLayoutMode === 1; enabled: documentFooter.canSplit; onTriggered: { win.setDocumentView(1); workspaceMenu.restoreViewChecks(); } }
        MenuSeparator {}
        CompactMenuItem { id: workspacePreviewView; objectName: "workspacePreviewView"; text: "Preview Only"; checkable: true; checked: workspaceLayout.effectiveLayoutMode === 2; onTriggered: { win.selectWritingMode("preview"); workspaceMenu.restoreViewChecks(); } }
        MenuSeparator {}
        CompactMenuItem { objectName: "workspaceOutlineEntry"; text: "Document outline"; onTriggered: workspaceCommands.run("outline") }
        CompactMenuItem { text: "Document statistics"; onTriggered: workspaceCommands.run("statistics") }
        CompactMenuItem { text: "Export and share…"; onTriggered: win.openExportHub("pdf") }
        MenuSeparator {}
        CompactMenuItem { objectName: "studioWorkspaceAction"; text: "Studio writing layout"; onTriggered: win.applyStudioWorkspace() }
        CompactMenuItem { text: "Restore column widths"; onTriggered: { workspaceLayout.organizerWidth=208; workspaceLayout.fileWidth=288; workspaceLayout.previewWidth=420; } }
        CompactMenuItem { text: "Show toolbar"; checkable: true; checked: workspaceSettings.toolbarVisibilityMode !== 2; onTriggered: workspaceCommands.run(workspaceSettings.toolbarVisibilityMode === 2 ? "toolbarAlways" : "toolbarHide") }
    }
    CompactMenu {
        id: libraryActions
        width: 246
        CompactMenuItem { text: "Search files…"; onTriggered: win.openQuickSearchState("", false, backend.library.rootFolder, false) }
        CompactMenuItem { text: "Choose folder…"; onTriggered: libraryPane.chooseFolder() }
        CompactMenuItem { text: "New document…"; onTriggered: libraryPane.newDocument() }
        CompactMenuItem { text: "New folder…"; onTriggered: libraryPane.newFolder() }
        MenuSeparator {}
        CompactMenuItem { text: "Previous folder"; enabled: backend.library.canGoBack; onTriggered: backend.library.navigateHistory(-1) }
        CompactMenuItem { text: "Next folder"; enabled: backend.library.canGoForward; onTriggered: backend.library.navigateHistory(1) }
        MenuSeparator {}
        CompactMenuItem { text: "Document previews"; checkable: true; checked: libraryPane.showExcerpts; onTriggered: workspaceCommands.run("excerpts") }
        CompactMenuItem { text: "Sort by name"; onTriggered: workspaceCommands.run("sortName") }
        CompactMenuItem { text: "Sort by modified date"; onTriggered: workspaceCommands.run("sortModified") }
        CompactMenuItem { text: "More folder options…"; onTriggered: libraryPane.showOptions(topChrome) }
    }
    Drawer {
        id: navigationDrawer
        objectName: "workspaceNavigationDrawer"
        property string paneName: "files"
        property var previousFocusItem: null
        width: Math.min(340, win.width - 80)
        height: win.contentItem.height
        edge: Qt.LeftEdge
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { color: navigationDrawer.paneName === "organizer" ? backend.palette.organizer : backend.palette.library }
        onAboutToShow: {
            previousFocusItem = win.activeFocusItem;
            var pane = paneName === "organizer" ? organizerPane : libraryPane;
            pane.parent = navigationBody;
        }
        onOpened: navigationCloseButton.forceActiveFocus(Qt.PopupFocusReason)
        onClosed: {
            organizerPane.parent = organizerSlot;
            libraryPane.parent = filesSlot;
            if (previousFocusItem && previousFocusItem.visible && previousFocusItem.enabled)
                previousFocusItem.forceActiveFocus(Qt.PopupFocusReason);
            else win.focusWritingSurface();
            previousFocusItem = null;
        }
        contentItem: ColumnLayout {
            spacing: 0
            RowLayout {
                Layout.fillWidth: true; Layout.preferredHeight: 52; Layout.leftMargin: 12; Layout.rightMargin: 12
                Label { text: navigationDrawer.paneName === "organizer" ? "Organizer" : "Files"; color: backend.palette.text; font.weight: Font.Medium; Layout.fillWidth: true }
                ToolbarButton { id: navigationCloseButton; objectName: "navigationCloseButton"; iconName: "close"; hint: "Close navigation"; onClicked: navigationDrawer.close() }
            }
            Item { id: navigationBody; Layout.fillWidth: true; Layout.fillHeight: true }
        }
    }

    Material.theme: darkMode ? Material.Dark : Material.Light
    Material.accent: backend.themeAccent
    Material.background: backend.palette.panel
    Material.foreground: backend.themeForeground
    color: pageColor

    function prepareQuit() {
        pendingAction = "quit";
        if (backend.modified) unsavedChangesDialog.open();
        else completePendingAction();
    }
    function commitQuit() {
        backend.discardRecovery();
        closeConfirmed = true;
        close();
    }
    onClosing: function(close) {
        if (closeConfirmed || !backend.modified) {
            backend.notifyWindowClosed();
            return;
        }

        close.accepted = false;
        pendingAction = "close";
        if (!unsavedChangesDialog.opened)
            unsavedChangesDialog.open();
    }

    function sourceFormattingAllowed() {
        return canFormatSource;
    }
    function liveFormattingAllowed() {
        return canFormatLive;
    }

    function tryWrapSelection(before, after) {
        if (liveFormattingAllowed()) { backend.liveWrapSelection(before, after); return; }
        if (sourceFormattingAllowed()) editor.wrapSelection(before, after)
    }

    function liveWebAction(name) {
        if (workspaceLayout.liveEditEnabled && liveEditorLoader.item) liveEditorLoader.item.triggerWebAction(name);
    }

    function tryInsertLink() {
        // The Source link editor dialog works on the Source text; the Live page has its own link panel.
        if (liveFormattingAllowed()) { backend.editorBridge.runCommand("link"); return; }
        if (sourceFormattingAllowed()) openLinkEditor(null)
    }

    function tryInsertSourceSnippet(replacement) {
        if (liveFormattingAllowed()) { backend.liveReplaceSelection(replacement); return; }
        if (!sourceFormattingAllowed()) return;
        var start = editor.selectionStart;
        var end = editor.selectionEnd;
        editor.forceActiveFocus();
        editor.replaceAtomic(start, end, replacement);
    }

    function editMarkdown(action) {
        if (liveFormattingAllowed()) { backend.liveEditMarkdown(action); return; }
        if (!sourceFormattingAllowed()) return
        var result = backend.editMarkdown(action, editor.selectionStart, editor.selectionEnd);
        editor.forceActiveFocus();
        if (result.start !== undefined) editor.select(result.start, result.end);
    }

    function decodeSimpleMarkdownLinkPart(value) { return LinkSyntax.decode(value); }
    function simpleInlineLinkAt(selectionStart, selectionEnd) {
        return LinkSyntax.analyze(editor.text, selectionStart, selectionEnd).existing || null;
    }
    function openLinkEditor(button) {
        if (!sourceFormattingAllowed()) return;
        var start = Math.min(editor.selectionStart, editor.selectionEnd);
        var end = Math.max(editor.selectionStart, editor.selectionEnd);
        var context = LinkSyntax.analyze(editor.text, start, end);
        if (context.error) { win.showNavigationNotice(context.error); return; }
        win.cancelDocumentViewportTransition();
        var point = button ? button.mapToItem(win.contentItem, button.width, button.height)
            : editorPane.mapToItem(win.contentItem, Math.min(editorPane.width - 16, 440), 16);
        linkEditor.showFor(start, end, context.existing || null, point);
    }

    function openQuickFormat(button) {
        if (!sourceFormattingAllowed() && !liveFormattingAllowed()) return;
        var point = button.mapToItem(win.contentItem, 0, button.height);
        formatQuickMenu.x = Math.max(8, Math.min(win.contentItem.width - formatQuickMenu.width - 8,
                                                  point.x + button.width - formatQuickMenu.width));
        formatQuickMenu.y = Math.min(win.contentItem.height - formatQuickMenu.height - 8,
                                     Math.max(48, point.y + 6));
        formatQuickMenu.open();
    }

    function showCompletions() {
        completionPopup.close();
        if (!sourceEditorVisible || editor.inputMethodComposing || editor.selectionStart !== editor.selectionEnd) return false;
        var result = backend.wordCompletions(editor.cursorPosition);
        var items = result.items || [];
        if (items.length === 0) return false;
        completionStart = result.start;
        completionEnd = result.end;
        completionPrefix = result.prefix;
        completionItems = items;
        completionList.currentIndex = 0;
        var point = editor.mapToItem(win.contentItem, editor.cursorRectangle.x,
                                     editor.cursorRectangle.y + editor.cursorRectangle.height);
        completionPopup.x = Math.max(8, Math.min(win.contentItem.width - completionPopup.width - 8, point.x));
        completionPopup.y = Math.max(8, Math.min(win.contentItem.height - completionPopup.height - 8, point.y));
        completionPopup.open();
        Qt.callLater(function() { if (win.sourceEditorVisible && completionPopup.visible) completionList.forceActiveFocus(); });
        return true;
    }

    function acceptCompletion(index) {
        if (!sourceEditorVisible) { completionPopup.close(); return false; }
        if (index < 0 || index >= completionItems.length
            || editor.inputMethodComposing
            || editor.selectionStart !== editor.selectionEnd
            || editor.cursorPosition !== completionEnd
            || editor.text.slice(completionStart, completionEnd) !== completionPrefix) {
            completionPopup.close();
            editor.forceActiveFocus();
            return false;
        }
        var completion = completionItems[index];
        completionPopup.close();
        editor.replaceAtomic(completionStart, completionEnd, completion);
        editor.forceActiveFocus();
        return true;
    }

    function insertSmartQuote() {
        if (!workspaceSettings.smartQuotes || editor.inputMethodComposing
            || editor.selectionStart !== editor.selectionEnd)
            return false;
        var position = editor.cursorPosition;
        var replacement = backend.smartQuoteAt(position);
        if (replacement.length === 0) return false;
        // Keep the literal keystroke and the automatic substitution as two
        // undo records, matching the native behavior: first Undo restores the
        // straight quote, and a second Undo removes the typed character.
        editor.replaceAtomic(position, position, "\"");
        editor.replaceAtomic(position, position + 1, replacement);
        editor.forceActiveFocus();
        return true;
    }

    function insertSmartDash() {
        if (!workspaceSettings.smartDashes || editor.inputMethodComposing
            || editor.selectionStart !== editor.selectionEnd)
            return false;
        var position = editor.cursorPosition;
        if (!backend.smartDashAt(position)) return false;
        // Preserve the literal pair as the first Undo state. Only the second
        // directly typed hyphen enters this path; paste and existing text do not.
        editor.replaceAtomic(position, position, "-");
        editor.replaceAtomic(position - 1, position + 1, "\u2014");
        editor.forceActiveFocus();
        return true;
    }

    function requestHistory(direction) {
        backend.rememberCursor(editor.cursorPosition);
        pendingHistoryDirection = direction;
        pendingAction = "history";
        if (backend.modified) unsavedChangesDialog.open();
        else completePendingAction();
    }

    function openSourceLink() {
        var link = backend.sourceLinkAt(editor.cursorPosition);
        if (/^file:.*\.(md|markdown|mdown|txt|text)(#.*)?$/i.test(String(link))) requestOpen(link);
        else backend.openExternalUrl(link);
    }

    function documentLinkParts(url) {
        var value = String(url);
        var hash = value.indexOf("#");
        return { url: hash < 0 ? value : value.slice(0, hash),
                 fragment: hash < 0 ? "" : value.slice(hash + 1) };
    }

    function showNavigationNotice(message) {
        navigationNotice = message;
        navigationNoticeTimer.restart();
    }

    function navigateDocumentFragment(fragment) {
        if (fragment === "") return false;
        var decoded;
        try { decoded = decodeURIComponent(fragment); }
        catch (error) {
            showNavigationNotice("This link contains an invalid heading address.");
            return false;
        }
        var position = backend.markdownAnchorPosition(editor.text, decoded);
        if (position < 0) {
            showNavigationNotice("Heading “" + decoded + "” was not found in this document.");
            return false;
        }
        navigationNotice = "";
        // Anchor navigation never reloads an existing view, including a dirty
        // destination. Resolve against the text that is actually open there.
        Qt.callLater(function() {
            win.cancelDocumentViewportTransition();
            editor.cursorPosition = position;
            if (win.sourceEditorVisible) { editor.forceActiveFocus(); editorFlick.ensureCursorVisible(); }
        });
        previewPane.navigateToAnchor(fragment);
        return true;
    }

    function openDocumentTarget(url, fragment) {
        navigationNotice = "";
        // Keep the fragment until the session manager has had the opportunity
        // to focus an already-open document and navigate in that destination.
        var target = String(url) + (fragment !== "" ? "#" + fragment : "");
        var opened = backend.open(target);
        if (opened) {
            navigationDrawer.close();
            if (fragment !== "") navigateDocumentFragment(fragment);
        }
        return opened;
    }

    function openQuickSearchState(query, contents, requestedRoot, creating) {
        var root = requestedRoot && requestedRoot.toString() !== "" ? requestedRoot : backend.library.rootFolder;
        if (!root || root.toString() === "") return false;
        backend.library.rootFolder = root;
        // FileLibrary rejects missing/unreadable roots and either retains the
        // previous folder or reports an error for a now-missing current root.
        // Do not show a query against the wrong or unavailable library.
        if (backend.library.rootFolder.toString() !== root.toString() || backend.library.error !== "") {
            workspaceLayout.filesVisible = true; // LibraryPane shows the concrete refusal.
            return false;
        }
        quickOpenDialog.creationMode = creating;
        savedSearchChoice.currentIndex = -1;
        quickContents.checked = contents;
        quickQuery.text = query;
        quickOpenDialog.open();
        return true;
    }

    function openSavedSearch(item) {
        return item && openQuickSearchState(item.query, item.contents, item.root, false);
    }

    function openHashtag(tag) {
        return openQuickSearchState("#" + tag, true, backend.library.rootFolder, false);
    }

    function requestOpen(url) {
        var parts = documentLinkParts(url);
        if (parts.fragment !== "" && (parts.url === "" || parts.url === String(backend.fileUrl)))
            return navigateDocumentFragment(parts.fragment);
        backend.rememberCursor(editor.cursorPosition);
        if (!backend.modified) {
            return openDocumentTarget(parts.url, parts.fragment);
        }
        pendingOpenUrl = parts.url;
        pendingOpenFragment = parts.fragment;
        pendingAction = "open";
        unsavedChangesDialog.open();
        return false;
    }

    function requestNewDocument() {
        pendingAction = "new";
        if (backend.modified) unsavedChangesDialog.open();
        else completePendingAction();
    }

    function requestCreateDocument(name, inNewWindow) {
        workspaceLayout.filesVisible = true;
        if (inNewWindow) {
            var file = backend.library.createDocument(name);
            if (file.toString() !== "") backend.openInNewWindow(file);
            return;
        }
        pendingFileName = name;
        pendingAction = "create";
        if (backend.modified) unsavedChangesDialog.open();
        else completePendingAction();
    }

    function showCurrentFileInLibrary() {
        workspaceLayout.filesVisible = true;
        libraryPane.showCurrentFile();
    }

    function completePendingAction() {
        var action = pendingAction;
        pendingAction = "";
        if (action === "quit") {
            backend.notifyQuitReady();
        } else if (action === "close") {
            closeConfirmed = true;
            close();
        } else if (action === "open") {
            var fragment = pendingOpenFragment;
            pendingOpenFragment = "";
            openDocumentTarget(pendingOpenUrl, fragment);
        } else if (action === "history") {
            var position = backend.navigateHistory(pendingHistoryDirection);
            if (position >= 0) Qt.callLater(function() {
                win.cancelDocumentViewportTransition();
                editor.cursorPosition = position;
                if (win.sourceEditorVisible) { editor.forceActiveFocus(); editorFlick.ensureCursorVisible(); }
            });
        } else if (action === "new") {
            backend.newDocument();
        } else if (action === "create") {
            var file = backend.library.createDocument(pendingFileName);
            if (file.toString() !== "") backend.open(file);
        }
    }

    Connections {
        target: backend
        function onStatusChanged() {
            // Pane contraction must not hide command failures or save feedback.
            // Routine dirty-state updates stay quiet while the user is typing.
            var message = backend.status;
            if (!message || message === "Unsaved") return;
            var sourceStatusVisible = documentFooter.statusVisible;
            if (!workspaceLayout.effectiveOrganizerVisible && !sourceStatusVisible)
                win.showNavigationNotice(message);
        }
    }

    FontMetrics {
        id: writerFontMetrics
        font.family: win.editorFontFamily
        font.pixelSize: win.editorFontPixelSize
    }

    // Every hardcoded size in the interface is expressed at text scale 1.
    function scaledSize(pixels) {
        return Math.max(1, Math.round(pixels * win.textScale));
    }

    function toggleFullScreen() {
        win.visibility = win.visibility === Window.FullScreen
            ? Window.Windowed
            : Window.FullScreen;
    }

    readonly property var editTarget: activeFocusItem && typeof activeFocusItem.cut === "function" ? activeFocusItem : editor
    // Clipboard commands must not borrow an invisible Source selection.
    readonly property bool sourceClipboardAllowed: sourceEditorVisible || editTarget !== editor

    function openSearch(withReplace, useSelection) {
        // In Live, Find/Replace is the editor page's own search panel.
        if (workspaceLayout.liveEditEnabled && liveEditorLoader.item) { backend.editorBridge.runCommand(withReplace ? "replace" : "find"); return; }
        cancelDocumentViewportTransition();
        var selected = editor.selectedText;
        // Find/Replace acts on canonical Markdown. Reveal that editor explicitly
        // while preserving Single/Split rather than hiding search behind the Live editor.
        if (!sourceEditorVisible) setEditingMode(false);
        cancelDocumentViewportTransition();
        if (!searchOpen) searchAnchor = editor.selectionStart;
        searchOpen = true;
        replaceOpen = withReplace;
        if (useSelection && selected.length > 0) searchPane.query = selected;
        searchPane.focusQuery();
        updateSearch();
    }

    function updateSearch(position, selectMatch) {
        var anchor = typeof position === "number" ? position : searchAnchor;
        searchMatches = backend.searchPositions(searchPane.query);
        searchMatchIndex = searchMatches.length > 0 ? 0 : -1;
        for (var index = 0; index < searchMatches.length; ++index) {
            if (searchMatches[index] >= anchor) { searchMatchIndex = index; break; }
        }
        if (searchOpen) showSearchMatch(selectMatch !== false);
    }

    function replaceSearch(all) {
        if (searchMatchIndex < 0) return;
        cancelDocumentViewportTransition();
        var start = searchMatches[searchMatchIndex];
        searchUpdating = true;
        backend.replaceMatches(searchPane.query, searchPane.replacement, all ? -1 : start);
        searchUpdating = false;
        backend.editorTextChanged();
        // Continue after the replacement, including when it contains the query.
        updateSearch(all ? 0 : start + searchPane.replacement.replace(/\r\n?/g, "\n").length);
    }

    function showSearchMatch(selectMatch) {
        var start = searchMatchIndex >= 0 ? searchMatches[searchMatchIndex] : -1;
        searchUpdating = true;
        backend.setSearchHighlight(searchPane.query, start);
        if (start >= 0 && selectMatch !== false) {
            editor.select(start, start + searchPane.query.length);
            editorFlick.ensureCursorVisible();
        }
        searchUpdating = false;
    }

    function moveSearch(direction) {
        if (searchMatches.length === 0)
            return;
        searchMatchIndex = (searchMatchIndex + direction + searchMatches.length)
                           % searchMatches.length;
        showSearchMatch();
    }

    function closeSearch(restoreFocus) {
        searchOpen = false;
        searchUpdating = true;
        backend.setSearchHighlight("", -1);
        editor.deselect();
        searchUpdating = false;
        replaceOpen = false;
        if (restoreFocus !== false) focusWritingSurface();
    }

    Shortcut {
        sequence: "Ctrl+S"
        context: Qt.WindowShortcut
        onActivated: backend.save()
    }

    Shortcut {
        sequence: win.isMac ? "Ctrl+Alt+F" : "Ctrl+H"
        context: Qt.WindowShortcut
        onActivated: {
            win.openSearch(true, false);
        }
    }

    Shortcut {
        sequence: "Ctrl+B"
        context: Qt.WindowShortcut
        enabled: win.canFormatSource
        onActivated: win.tryWrapSelection("**", "**")
    }

    Shortcut {
        sequence: "Ctrl+I"
        context: Qt.WindowShortcut
        enabled: win.canFormatSource
        onActivated: win.tryWrapSelection("*", "*")
    }

    Shortcut {
        sequence: "Ctrl+K"
        context: Qt.WindowShortcut
        enabled: win.canFormatSource || win.canFormatLive
        onActivated: win.tryInsertLink()
    }

    Shortcut {
        sequence: "Ctrl+?"
        context: Qt.WindowShortcut
        onActivated: shortcutsDialog.open()
    }

    Shortcut {
        sequence: "Ctrl+O"
        context: Qt.WindowShortcut
        onActivated: backend.openDialog()
    }

    Shortcut {
        sequence: "Ctrl+N"
        context: Qt.WindowShortcut
        onActivated: backend.newWindow()
    }

    Shortcut {
        sequence: "Ctrl+Shift+S"
        context: Qt.WindowShortcut
        onActivated: backend.saveAsDialog()
    }

    Shortcut {
        sequence: "Ctrl+P"
        context: Qt.WindowShortcut
        onActivated: backend.printDocument()
    }

    Shortcut {
        sequences: win.isMac ? ["Ctrl+Meta+F"] : ["Meta+F", "F11"]
        context: Qt.WindowShortcut
        onActivated: toggleFullScreen()
    }

    Shortcut {
        sequence: "Ctrl+Z"
        context: Qt.WindowShortcut
        onActivated: win.performDocumentEdit(function() { win.undoEditing(); })
    }

    Shortcut {
        sequences: ["Ctrl+Shift+Z", "Ctrl+Y"]
        context: Qt.WindowShortcut
        onActivated: win.performDocumentEdit(function() { win.redoEditing(); })
    }

    Shortcut {
        sequence: "Ctrl+F"
        context: Qt.WindowShortcut
        onActivated: {
            win.openSearch(false, false);
        }
    }

    Shortcut {
        sequence: "Ctrl+Shift+G"
        enabled: win.searchOpen
        onActivated: win.moveSearch(-1)
    }

    Shortcut {
        sequence: "Ctrl+G"
        context: Qt.WindowShortcut
        enabled: win.searchOpen
        onActivated: win.moveSearch(1)
    }

    Shortcut {
        sequences: [StandardKey.Close]
        onActivated: win.close()
    }

    Loader {
        active: win.isMac
        sourceComponent: Component {
    NativeMenuBar {}

        }
    }

    Connections {
        target: backend

        function onDocumentLoaded() {
            win.sourceFormattingOwned = false;
            win.cancelDocumentViewportTransition();
            Qt.callLater(function() {
                editor.cursorPosition = 0;
                editorFlick.contentY = 0;
                win.updateSourceFormattingOwner();
            });
        }

        function onOpenDialogRequested() {
            openFileDialog.open();
        }

        function onSaveDialogRequested(suggestedUrl) {
            saveFileDialog.selectedFile = suggestedUrl;
            saveFileDialog.open();
        }

        function onSaveFailed() { win.awaitingPendingSave = false; win.pendingAction = ""; }

        function onSaveSucceeded() {
            win.awaitingPendingSave = false;
            if (win.pendingAction !== "")
                win.completePendingAction();
        }

        function onExternalChangeDetected(deleted, locallyModified) {
            externalChangeDialog.deleted = deleted;
            externalChangeDialog.locallyModified = locallyModified;
            externalChangeDialog.open();
        }
    }

    Timer { interval: 60000; repeat: true; running: workspaceSettings.autosaveEnabled; onTriggered: { if (!unsavedChangesDialog.opened && win.pendingAction === "") backend.autosave(); } }
    Dialog {
        id: authorshipSetupDialog
        objectName: "authorshipSetupDialog"
        title: "Set Up Authorship"
        modal: true
        anchors.centerIn: parent
        width: Math.min(480, win.width - 40)
        closePolicy: Popup.CloseOnEscape
        readonly property bool validProfile: {
            var name = authorshipProfileName.text.trim();
            var identifier = authorshipProfileIdentifier.text.trim();
            var controls = /[\u0000-\u001f\u007f-\u009f]/;
            return name.length > 0 && name.length <= 100
                    && identifier.length <= 200
                    && !controls.test(name) && !controls.test(identifier);
        }
        onOpened: {
            authorshipProfileName.text = workspaceSettings.authorshipProfileName;
            authorshipProfileIdentifier.text = workspaceSettings.authorshipProfileIdentifier;
            authorshipProfileName.forceActiveFocus();
            authorshipProfileName.selectAll();
        }
        function saveProfile() {
            if (!validProfile) return;
            workspaceSettings.authorshipProfileName = authorshipProfileName.text.trim();
            workspaceSettings.authorshipProfileIdentifier = authorshipProfileIdentifier.text.trim();
            close();
        }
        ColumnLayout {
            anchors.fill: parent
            Label { text: "Name" }
            TextField {
                id: authorshipProfileName
                objectName: "authorshipProfileName"
                Layout.fillWidth: true
                placeholderText: "Required"
                onAccepted: if (authorshipSetupDialog.validProfile) authorshipSetupDialog.saveProfile()
            }
            Label { text: "Identifier (optional)" }
            TextField {
                id: authorshipProfileIdentifier
                objectName: "authorshipProfileIdentifier"
                Layout.fillWidth: true
                placeholderText: "Optional"
                onAccepted: if (authorshipSetupDialog.validProfile) authorshipSetupDialog.saveProfile()
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: "This local profile does not label existing or future text automatically. Use Edit → Authorship Annotations for manual labels."
            }
        }
        footer: DialogButtonBox {
            Button {
                objectName: "authorshipProfileCancel"
                text: "Cancel"
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            }
            Button {
                objectName: "authorshipProfileSave"
                text: "Save"
                enabled: authorshipSetupDialog.validProfile
                onClicked: authorshipSetupDialog.saveProfile()
            }
        }
    }
    Dialog {
        id: versionsDialog
        objectName: "versionsDialog"
        title: "Restore a saved version (one-step undo)"
        property var items: []
        modal: true
        anchors.centerIn: parent
        width: Math.min(540, win.width - 40)
        height: Math.min(400, win.height - 60)
        standardButtons: Dialog.Cancel
        ColumnLayout {
            anchors.fill: parent
            Label { text: "Replaces editor text without authorship labels. Autosave pauses until you Save. One Undo restores your previous text and labels."; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { visible: versionsDialog.items.length === 0; text: "No saved versions are available." }
            ListView {
                Layout.fillWidth: true; Layout.fillHeight: true
                model: versionsDialog.items
                delegate: ItemDelegate {
                    required property var modelData
                    width: ListView.view.width
                    text: modelData.date
                    onClicked: { backend.restoreVersion(modelData.url); versionsDialog.close(); }
                }
            }
        }
    }

    Dialog {
        id: authorshipDialog
        property var ranges: []
        property int pendingStart: -1
        property int pendingEnd: -1
        onClosed: if (pendingStart >= 0) { editor.forceActiveFocus(); editor.select(pendingStart, pendingEnd); pendingStart = -1; }
        title: "Authorship annotations"
        modal: true
        anchors.centerIn: parent
        width: Math.min(620, win.width - 40)
        height: Math.min(500, win.height - 60)
        standardButtons: Dialog.Close
        ColumnLayout {
            anchors.fill: parent
            Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "Manual labels, not verified provenance. Edits can inherit nearby labels. Save writes a hidden .omawrite-authors.json sidecar; keep it with the Markdown file. External edits invalidate labels. Copy/paste between Fomawrite windows preserves labels; other apps may remove them. External plain text is unlabelled. Export metadata separately to keep manual assertions with matching Markdown." }
            TextField { id: authorName; Layout.fillWidth: true; placeholderText: "Author or source name (optional)" }
            RowLayout {
                Repeater {
                    model: ["Human", "AI", "Reference", "Unknown"]
                    Button { required property string modelData; text: modelData; enabled: editor.selectionStart !== editor.selectionEnd; onClicked: { backend.markAuthorship(editor.selectionStart, editor.selectionEnd, modelData, authorName.text); authorshipDialog.ranges = backend.authorshipRanges(); } }
                }
            }
            ListView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                model: authorshipDialog.ranges
                delegate: ItemDelegate { required property var modelData; width: ListView.view.width; text: modelData.start + "–" + modelData.end + ": " + modelData.category + " " + modelData.author; onClicked: { authorshipDialog.pendingStart=modelData.start; authorshipDialog.pendingEnd=modelData.end; authorshipDialog.close(); } }
                ScrollBar.vertical: ScrollBar {}
            }
        }
    }

    Dialog {
        id: analysisDialog
        objectName: "writingReviewDialog"
        closePolicy: Popup.CloseOnEscape
        property var results: []
        title: "Writing review — suggestions, not corrections"
        modal: false
        dim: false
        anchors.centerIn: parent
        width: Math.min(580, win.width - 40)
        height: Math.min(460, win.height - 60)
        standardButtons: Dialog.Close
        onOpened: refresh()
        function refresh() { results = backend.writingAnalysis(editor.text, workspaceSettings.reviewWords); }
        Timer { interval: 1000; repeat: true; running: analysisDialog.visible; onTriggered: analysisDialog.refresh() }
        ColumnLayout {
            anchors.fill: parent
            TextField { Layout.fillWidth: true; text: workspaceSettings.reviewWords; placeholderText: "Custom review words, separated by commas"; onEditingFinished: { workspaceSettings.reviewWords = text; analysisDialog.refresh(); } }
            Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "System word classes and review words; code and URL destinations excluded. Refreshes while open. First 50,000 characters / 1,000 results. Language support depends on macOS." }
            ListView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                model: analysisDialog.results
                delegate: Label { required property var modelData; text: modelData.word + " — " + modelData.label; width: ListView.view.width; height: 28 }
                ScrollBar.vertical: ScrollBar {}
            }
        }
    }

    Dialog {
        id: spellingDialog
        objectName: "spellingDialog"
        property var issues: []
        property string snapshot: ""
        title: "Spelling and Grammar"
        modal: true
        anchors.centerIn: parent
        width: Math.min(640, win.width - 40)
        height: Math.min(540, win.height - 60)
        standardButtons: Dialog.Close
        function refresh() { snapshot=editor.text; issues=backend.writingIssues(snapshot, writingLanguage.currentText, grammarReview.checked); }
        onOpened: refresh()
        ColumnLayout {
            anchors.fill: parent
            RowLayout {
                ComboBox { id: writingLanguage; Accessible.name: "Review language"; model: ["System language"].concat(backend.writingLanguages()); onActivated: spellingDialog.refresh() }
                CheckBox { id: grammarReview; text: "Grammar"; onToggled: spellingDialog.refresh() }
                Button { text: "Refresh"; onClicked: spellingDialog.refresh() }
            }
            Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "Suggestions use macOS dictionaries. Review before replacing. Code and URL destinations are excluded. First 50,000 characters / 100 issues; grammar support varies by language." }
            Label { text: spellingDialog.issues.length ? spellingDialog.issues.length + " issues" : "No issues found in the checked text." }
            ListView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                model: spellingDialog.issues
                delegate: ColumnLayout {
                    required property var modelData
                    width: ListView.view.width
                    Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: modelData.word + " — " + modelData.label }
                    RowLayout {
                        ComboBox { id: correction; Layout.fillWidth: true; model: modelData.suggestions; Accessible.name: "Replacement for " + modelData.word }
                        Button { text: "Replace"; enabled: correction.count > 0 && spellingDialog.snapshot === editor.text
                            onClicked: { backend.correctWriting(modelData.start,modelData.end,modelData.word,correction.currentText); spellingDialog.refresh(); } }
                    }
                }
                ScrollBar.vertical: ScrollBar {}
            }
        }
    }

    Dialog {
        id: commandPalette
        objectName: "commandPalette"
        title: "Command Palette"
        modal: true
        anchors.centerIn: parent
        width: Math.min(580, win.width - 40)
        height: Math.min(460, win.height - 60)
        standardButtons: Dialog.Cancel
        property string pendingCommand: ""
        onClosed: if (pendingCommand !== "") { var command=pendingCommand; pendingCommand=""; editor.forceActiveFocus(); workspaceCommands.run(command); }
        property var results: []
        function refresh() {
            results = workspaceCommands.entries.filter(function(item) {
                return workspaceCommands.label(item.id).toLowerCase().indexOf(commandQuery.text.toLowerCase()) >= 0;
            });
            commandList.currentIndex = results.length ? 0 : -1;
        }
        function choose(index) {
            if (index < 0 || index >= results.length) return;
            var id = results[index].id;
            if (!workspaceCommands.isEnabled(id)) return;
            pendingCommand=id;
            close();
        }
        onOpened: { refresh(); commandQuery.forceActiveFocus(); commandQuery.selectAll(); }
        ColumnLayout {
            anchors.fill: parent
            TextField {
                id: commandQuery
                objectName: "commandQuery"
                Layout.fillWidth: true
                placeholderText: "Search workspace commands…"
                onTextChanged: commandPalette.refresh()
                onAccepted: commandPalette.choose(commandList.currentIndex)
                Keys.onDownPressed: commandList.currentIndex = Math.min(commandList.count - 1, commandList.currentIndex + 1)
                Keys.onUpPressed: commandList.currentIndex = Math.max(0, commandList.currentIndex - 1)
            }
            ListView {
                id: commandList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: commandPalette.results
                delegate: ItemDelegate {
                    required property var modelData
                    required property int index
                    width: commandList.width
                    text: workspaceCommands.label(modelData.id) + (workspaceCommands.isChecked(modelData.id) ? " ✓" : "")
                    enabled: workspaceCommands.isEnabled(modelData.id)
                    highlighted: commandList.currentIndex === index
                    onClicked: commandPalette.choose(index)
                }
                ScrollBar.vertical: ScrollBar {}
            }
            Label { visible: commandPalette.results.length === 0; text: "No matching commands" }
        }
    }

    Dialog {
        id: quickOpenDialog
        objectName: "quickOpenDialog"
        property bool creationMode: false
        title: creationMode ? "New Smart Folder — save this query" : "Quick Search — current library"
        modal: true
        anchors.centerIn: parent
        width: Math.min(620, win.width - 40)
        height: Math.min(460, win.height - 60)
        standardButtons: Dialog.Cancel
        onOpened: { quickQuery.forceActiveFocus(); quickQuery.selectAll(); quickSearchTimer.restart(); }
        onClosed: { creationMode = false; quickSearchTimer.stop(); backend.library.cancelQuickSearch(); }
        function choose(index) {
            var results = backend.library.quickResults;
            if (index < 0 || index >= results.length) return;
            var url = results[index].url;
            close();
            win.requestOpen(url);
        }
        Timer { interval: 5000; repeat: true; running: quickOpenDialog.visible && quickContents.checked; onTriggered: quickSearchTimer.restart() }
        Timer { id: quickSearchTimer; interval: 150; onTriggered: backend.library.quickSearch(quickQuery.text, quickContents.checked) }
        ColumnLayout {
            anchors.fill: parent
            TextField {
                id: quickQuery
                objectName: "quickQuery"
                Layout.fillWidth: true
                placeholderText: "Filename, content, or #tag…"
                onTextChanged: { backend.library.cancelQuickSearch(); quickSearchTimer.restart(); }
                onAccepted: quickOpenDialog.choose(quickList.currentIndex)
                Keys.onDownPressed: quickList.currentIndex = Math.min(quickList.count - 1, quickList.currentIndex + 1)
                Keys.onUpPressed: quickList.currentIndex = Math.max(0, quickList.currentIndex - 1)
            }
            RowLayout {
                Layout.fillWidth: true
                ComboBox {
                    id: savedSearchChoice
                    objectName: "savedSearchChoice"
                    Layout.fillWidth: true
                    model: backend.library.savedSearches
                    textRole: "query"
                    displayText: currentIndex < 0 ? "Saved searches…" : currentText
                    onActivated: {
                        var item = backend.library.savedSearches[currentIndex];
                        win.openSavedSearch(item);
                    }
                }
                Button { objectName: "saveSmartFolder"; text: "Save query"; enabled: quickQuery.text.trim().length > 0; onClicked: backend.library.saveSearch(quickQuery.text, quickContents.checked) }
                Button { objectName: "removeSmartFolder"; text: "Remove query"; enabled: savedSearchChoice.currentIndex >= 0; onClicked: backend.library.removeSearch(savedSearchChoice.currentIndex) }
            }
            CheckBox { id: quickContents; objectName: "quickContents"; text: "Search saved file contents too"; onToggled: { backend.library.cancelQuickSearch(); quickSearchTimer.restart(); } }
            Label { Layout.fillWidth: true; text: backend.library.quickStatus; wrapMode: Text.Wrap }
            ListView {
                id: quickList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: backend.library.quickResults
                onCountChanged: currentIndex = count > 0 ? 0 : -1
                delegate: ItemDelegate {
                    required property var modelData
                    required property int index
                    width: quickList.width
                    text: modelData.path
                    highlighted: quickList.currentIndex === index
                    onClicked: quickOpenDialog.choose(index)
                }
                ScrollBar.vertical: ScrollBar {}
            }
            Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "Markdown/text files only. Hidden folders, symlinks and build/dependency folders are excluded. Content search reads saved files up to 256 KiB each; unsaved edits are excluded."; font.pixelSize: 12 }
        }
    }

    Dialogs.FileDialog { id: authorshipExportDialog; title: "Export Authorship Metadata"; fileMode: Dialogs.FileDialog.SaveFile; nameFilters: ["Metadata (*.json)"]; onAccepted: backend.exportAuthorship(selectedFile) }
    Dialogs.FileDialog {
        id: exportDialog
        property string outputFormat: "html"
        title: "Export Document"
        fileMode: Dialogs.FileDialog.SaveFile
        onAccepted: backend.exportDocument(selectedFile, outputFormat)
    }
    ExportHub {
        id: exportHub
        backend: win.appBackend
        renderer: win.appBackend
        markdown: editor.text
        documentBaseUrl: win.appBackend.documentBaseUrl
        darkMode: win.darkMode
        onDestinationRequested: function(format) {
            exportDialog.outputFormat = format;
            exportDialog.nameFilters = format === "pdf" ? ["PDF (*.pdf)"] : ["HTML (*.html)"];
            exportDialog.open();
        }
    }
    Dialog {
        id: publishingThemeNotice
        objectName: "publishingThemeNotice"
        title: "Publishing Theme Notice"
        property string message: ""
        modal: true
        anchors.centerIn: parent
        width: Math.min(560, win.width - 48)
        standardButtons: Dialog.Ok
        Label { width: parent.width; text: publishingThemeNotice.message; wrapMode: Text.Wrap; color: backend.palette.text; Accessible.name: text }
    }
    Connections {
        target: backend
        property string lastIssue: ""
        function onPublishingThemesChanged() {
            var issue = backend.publishingThemeError;
            if (issue && issue !== lastIssue) win.showPublishingThemeNotice(issue);
            lastIssue = issue;
        }
    }

    Dialogs.FileDialog {
        id: publishingThemeDialog
        objectName: "publishingThemeDialog"
        title: "Import Publishing Theme"
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: ["CSS themes (*.css)"]
        onAccepted: { if (!backend.importPublishingTheme(selectedFile)) win.showPublishingThemeNotice(backend.publishingThemeError || backend.status); win.restorePublishingChecks(); }
    }
    Dialogs.FileDialog {
        id: outputStyleDialog
        objectName: "outputStyleDialog"
        title: "Load Custom Font and Page Settings"
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: ["Font and page settings (*.json)"]
        onAccepted: { if (!backend.loadOutputStyle(selectedFile)) win.showPublishingThemeNotice(backend.status); win.restorePublishingChecks(); }
    }

    Dialog {
        id: openPathDialog
        objectName: "openPathDialog"
        title: "Open by Path"
        modal: true
        anchors.centerIn: parent
        width: Math.min(560, win.width - 48)
        property string errorText: ""
        onOpened: { errorText = ""; openPathInput.forceActiveFocus(); openPathInput.selectAll(); }
        function submit() {
            var result = backend.resolveOpenPath(openPathInput.text);
            if (result.error) { errorText = result.error; return; }
            if (result.folder) {
                backend.library.rootFolder = result.url;
                if (String(backend.library.rootFolder) !== String(result.url)) {
                    errorText = backend.library.error;
                    return;
                }
                workspaceLayout.filesVisible = true;
                openPathDialog.close();
            } else {
                openPathDialog.close();
                win.requestOpen(result.url);
            }
        }
        ColumnLayout {
            width: parent.width
            Label { text: "Open a Markdown/text file, or show a folder in the library."; wrapMode: Text.Wrap; Layout.fillWidth: true }
            TextField {
                id: openPathInput
                objectName: "openPathInput"
                Layout.fillWidth: true
                placeholderText: "~/Documents or /full/path/note.md"
                Accessible.name: "File or folder path"
                onTextChanged: openPathDialog.errorText = ""
                onAccepted: openPathDialog.submit()
            }
            Label { text: "Accepts absolute paths, ~/ paths and file:/// URLs. Folder opening keeps your current document."; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { text: openPathDialog.errorText; visible: text !== ""; wrapMode: Text.Wrap; Layout.fillWidth: true; color: win.darkMode ? "#fca5a5" : "#b42318" }
        }
        footer: DialogButtonBox {
            Button { text: "Cancel"; DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            Button { text: "Open"; enabled: openPathInput.text.trim().length > 0; onClicked: openPathDialog.submit() }
            onRejected: openPathDialog.reject()
        }
    }

    Dialogs.FileDialog {
        id: openFileDialog
        title: "Open File"
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: ["Markdown files (*.md *.markdown)", "All files (*)"]
        onAccepted: win.requestOpen(selectedFile)
    }

    Dialogs.FileDialog {
        id: saveFileDialog
        title: "Save File"
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: ["Markdown files (*.md *.markdown)", "All files (*)"]
        onAccepted: backend.saveAs(selectedFile)
        onRejected: {
            backend.fileDialogCanceled();
            backend.cancelQuit();
            win.awaitingPendingSave = false;
            win.pendingAction = "";
        }
    }

    Dialogs.FolderDialog {
        id: moveFolderDialog
        objectName: "moveFolderDialog"
        // The macOS native picker currently leaves its accept button disabled.
        options: win.isMac ? Dialogs.FolderDialog.DontUseNativeDialog : 0
        title: "Move document — keep unsaved edits; relative links use the new folder"
        onAccepted: {
            if (!backend.moveDocument(selectedFolder)) {
                moveError.text = backend.status;
                moveError.open();
            }
        }
    }

    Dialog {
        id: moveError
        objectName: "moveError"
        property string text: ""
        title: "Could not finish moving"
        modal: true
        anchors.centerIn: parent
        width: Math.min(440, win.width - 40)
        standardButtons: Dialog.Ok
        Label { width: parent.width; text: moveError.text; wrapMode: Text.Wrap }
    }

    Dialog {
        id: fileNameDialog
        objectName: "fileNameDialog"
        property bool renaming: false
        property string errorText: ""
        title: renaming ? "Rename document" : "Duplicate document"
        modal: true
        anchors.centerIn: parent
        width: Math.min(440, win.width - 48)
        function showFor(rename) {
            renaming = rename;
            errorText = "";
            var name = backend.fileName;
            var dot = name.lastIndexOf(".");
            fileNameInput.text = rename ? name : (dot > 0 ? name.slice(0, dot) + " copy" + name.slice(dot) : name + " copy.md");
            open();
            fileNameInput.forceActiveFocus();
            fileNameInput.selectAll();
        }
        function submit() {
            var success = renaming ? backend.renameDocument(fileNameInput.text) : backend.duplicateDocument(fileNameInput.text);
            if (success) close();
            else errorText = backend.status;
        }
        ColumnLayout {
            width: parent.width
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: fileNameDialog.renaming ? "Rename in the current folder. Unsaved edits stay in this window."
                    : "Create a copy in the current folder, including unsaved edits. Keep editing the original."
            }
            TextField { id: fileNameInput; objectName: "fileNameInput"; Layout.fillWidth: true; onAccepted: fileNameDialog.submit() }
            Label { Layout.fillWidth: true; wrapMode: Text.Wrap; visible: text !== ""; text: fileNameDialog.errorText; color: win.darkMode ? "#fca5a5" : "#b42318" }
        }
        footer: DialogButtonBox {
            Button { text: "Cancel"; DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            Button { text: fileNameDialog.renaming ? "Rename" : "Duplicate"; enabled: fileNameInput.text.length > 0; onClicked: fileNameDialog.submit() }
            onRejected: fileNameDialog.reject()
        }
    }

    UnsavedChangesDialog {
        id: unsavedChangesDialog
        objectName: "unsavedChangesPrompt"
        pendingAction: win.pendingAction
        fileName: backend.fileName
        darkMode: win.darkMode
        textScale: win.textScale
        textColor: win.textColor
        strongTextColor: win.strongTextColor
        activeButtonColor: backend.themeAccent
        containerWidth: win.width
        containerHeight: win.height

        onDiscardRequested: {
            // A failed open/create must retain recovery for the unchanged buffer.
            if (win.pendingAction === "close") backend.discardRecovery();
            win.completePendingAction();
        }

        onSaveRequested: {
            win.awaitingPendingSave = true;
            backend.save();
        }
        onCancelRequested: { win.pendingAction = ""; win.pendingOpenFragment = ""; backend.cancelQuit(); }
    }

    ExternalChangeDialog {
        id: externalChangeDialog
        darkMode: win.darkMode
        textScale: win.textScale
        textColor: win.textColor
        strongTextColor: win.strongTextColor
        containerWidth: win.width
        containerHeight: win.height

        onKeepRequested: backend.keepExternalVersion()
        onReloadRequested: backend.reloadFromDisk()
        onOverwriteRequested: { backend.keepExternalVersion(); backend.save(); }
    }

    Dialog {
        id: shortcutsDialog
        objectName: "shortcutsDialog"
        modal: true
        title: "Keyboard shortcuts"
        width: Math.min(win.width - 40, win.scaledSize(440))
        standardButtons: Dialog.Close
        anchors.centerIn: parent
        contentItem: Label {
            text: win.isMac ? "⌘S  Save\n⇧⌘S  Save As\n⌘O  Open\n⌘N  New Window\n⌘W  Close Window\n⌘F  Find\n⌥⌘F  Find and Replace\n⌘B  Bold\n⌘I  Italic\n⌘K  Link\n⌘P  Print\n⌃⌘F  Fullscreen\nF6 / ⇧F6  Next / previous workspace pane\n⌘?  Shortcuts" : "Ctrl+S  Save\nCtrl+Shift+S  Save As\nCtrl+O  Open\nCtrl+N  New Window\nCtrl+F  Find\nCtrl+H  Find and Replace\nCtrl+B  Bold\nCtrl+I  Italic\nCtrl+K  Link\nCtrl+P  Print\nF11 / Super+F  Fullscreen\nF6 / Shift+F6  Next / previous workspace pane\nCtrl+?  Shortcuts"
            lineHeight: 1.5
        }
    }

    Dialog {
        id: helpDialog
        objectName: "helpDialog"
        property string markdown: ""
        modal: false
        dim: false
        width: Math.min(win.width - 40, win.scaledSize(720))
        height: Math.min(win.height - 60, win.scaledSize(640))
        anchors.centerIn: parent
        closePolicy: Popup.CloseOnEscape
        standardButtons: Dialog.Close
        function showPage(page, pageTitle) {
            title = pageTitle;
            markdown = backend.bundledHelp(page);
            open();
        }
        contentItem: ScrollView {
            TextArea {
                objectName: "helpDocumentText"
                text: helpDialog.markdown
                textFormat: TextEdit.MarkdownText
                readOnly: true
                selectByMouse: true
                wrapMode: win.codeAppearance ? TextEdit.NoWrap : TextEdit.Wrap
                color: win.textColor
                background: null
                Accessible.name: helpDialog.title
            }
        }
    }

    SplitView {
        id: workspaceSplit
        objectName: "workspaceSplit"
        property var resizeStart: null
        onResizingChanged: {
            if (resizing) {
                resizeStart = { organizer: organizerSlot.width, files: filesSlot.width, preview: previewPane.width };
            } else if (resizeStart) {
                var actual = { organizer: organizerSlot.width, files: filesSlot.width, preview: previewPane.width };
                if (organizerSlot.visible && Math.abs(actual.organizer - resizeStart.organizer) > 1)
                    workspaceLayout.updateWidth("organizer", actual.organizer);
                if (filesSlot.visible && Math.abs(actual.files - resizeStart.files) > 1)
                    workspaceLayout.updateWidth("files", actual.files);
                if (previewPane.visible && editorPane.visible && Math.abs(actual.preview - resizeStart.preview) > 1)
                    workspaceLayout.updateWidth("preview", actual.preview);
                resizeStart = null;
                // SplitView replaces preferred-size bindings during an explicit drag.
                organizerSlot.SplitView.preferredWidth = Qt.binding(function() { return workspaceLayout.effectiveOrganizerWidth; });
                filesSlot.SplitView.preferredWidth = Qt.binding(function() { return workspaceLayout.effectiveFileWidth; });
                previewPane.SplitView.preferredWidth = Qt.binding(function() { return workspaceLayout.effectivePreviewWidth; });
            }
        }
        anchors.top: topChrome.bottom
        anchors.topMargin: win.tabInset
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        orientation: Qt.Horizontal
        handle: Rectangle {
            id: workspaceDivider
            readonly property bool documentDivider: editorPane.visible && previewPane.visible
                && Math.abs(x - (editorPane.x + editorPane.width)) <= 2
            objectName: documentDivider ? "documentSplitDivider" : "workspaceDivider"
            implicitWidth: 1
            color: SplitHandle.hovered || SplitHandle.pressed ? backend.palette.focus : backend.palette.border
            // A fine visual rule still has a generous mouse target for resizing.
            containmentMask: Item { x: -4; width: 9; height: workspaceDivider.height }
            HoverHandler { id: dividerHover; cursorShape: Qt.SplitHCursor; margin: 4 }
        }
        Item {
            id: organizerSlot
            objectName: "organizerSlot"
            visible: workspaceLayout.effectiveOrganizerVisible
            SplitView.preferredWidth: workspaceLayout.effectiveOrganizerWidth
            SplitView.minimumWidth: 184
            SplitView.maximumWidth: 288
            OrganizerPane {
                id: organizerPane
                anchors.fill: parent
                commands: workspaceCommands
                library: backend.library
                currentFile: backend.fileUrl
                darkMode: win.darkMode
                onSearchRequested: function(query, contents, folder) { backend.library.rootFolder=folder; quickQuery.text=query; quickContents.checked=contents; navigationDrawer.close(); quickOpenDialog.open(); }
                onOpenRequested: function(file) { win.requestOpen(file); }
            }
        }
        Item {
            id: filesSlot
            objectName: "filesSlot"
            visible: workspaceLayout.effectiveFilesVisible
            SplitView.preferredWidth: workspaceLayout.effectiveFileWidth
            SplitView.minimumWidth: 232
            SplitView.maximumWidth: 420
            LibraryPane {
                id: libraryPane
                anchors.fill: parent
                commands: workspaceCommands
                library: backend.library
                currentFile: backend.fileUrl
                darkMode: win.darkMode
                onCreateRequested: function(name, inNewWindow) { win.requestCreateDocument(name, inNewWindow); }
                onOpenRequested: function(file) { win.requestOpen(file); }
            }
        }
        Rectangle {
            id: editorPane
            objectName: "editorPane"
            onVisibleChanged: if (!visible && win.searchOpen) win.closeSearch(false)
            color: backend.palette.editor
            visible: workspaceLayout.effectiveLayoutMode !== 2
            SplitView.fillWidth: true
            SplitView.minimumWidth: workspaceLayout.effectiveLayoutMode === 1 ? 480 : 320

        RowLayout {
            id: documentMeta
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            objectName: "documentMeta"
            anchors.topMargin: win.searchOpen ? searchPane.height + 24 : 0
            anchors.leftMargin: 24; anchors.rightMargin: 24; height: 48
            Label {
                objectName: "topChromeTitle"
                text: backend.fileName
                color: backend.palette.muted
                font.pixelSize: 12
                elide: Text.ElideMiddle
                opacity: topChrome.titleContentOpacity
                Layout.fillWidth: true
                Accessible.description: backend.fileUrl.toString()
            }
            ChromeButton {
                id: editorWordCountChip
                objectName: "editorWordCountChip"
                text: win.compactStatistic("words")
                font.pixelSize: 12
                implicitHeight: 26
                background: Rectangle {
                    radius: height / 2
                    color: editorWordCountChip.down ? backend.palette.controlPressed
                        : editorWordCountChip.hovered ? backend.palette.controlHover : backend.palette.control
                    border.width: editorWordCountChip.activeFocus ? 2 : 1
                    border.color: editorWordCountChip.activeFocus ? backend.palette.focus : backend.palette.border
                }
                hint: "Document statistics"
                Accessible.name: "Document word count: " + text
                onClicked: workspaceCommands.run("statistics")
            }
        }

        Flickable {
            id: editorFlick
            property var codeLines: []
            Timer {
                id: codeDecorationsTimer
                interval: 0
                onTriggered: editorFlick.codeLines = backend.sourceLineDecorations(editorFlick.contentY - editor.y, editorFlick.contentY + editorFlick.height - editor.y)
            }
            TextMetrics { id: codeMetrics; font: editor.font; text: " " }
            Repeater {
                model: editorFlick.codeLines
                delegate: Item {
                    required property var modelData
                    x: editor.x; y: editor.y + modelData.y
                    width: Math.max(editor.width, editor.contentWidth); height: modelData.height
                    Rectangle {
                        objectName: "sourceCodeBlockBackground"
                        visible: modelData.fenced
                        x: modelData.margin - 6; y: 0
                        width: parent.width - x + 6; height: parent.height + 0.5
                        color: win.darkMode ? "#252A32" : "#EAECF0"
                    }
                    Text {
                        visible: win.codeAppearance
                        x: -win.scaledSize(56); width: win.scaledSize(44)
                        text: modelData.number; horizontalAlignment: Text.AlignRight
                        font: editor.font; color: win.mutedColor; opacity: 0.75
                        Accessible.ignored: true
                    }
                    Repeater {
                        model: win.codeAppearance ? Math.floor(parent.modelData.indent / 4) : 0
                        Rectangle {
                            required property int index
                            x: index * codeMetrics.advanceWidth * 4
                            width: 1; height: parent.height
                            color: backend.palette.border; opacity: 0.6
                        }
                    }
                }
            }
            visible: !workspaceLayout.liveEditEnabled
            onContentYChanged: {
                codeDecorationsTimer.restart();
                if (!win.sourceEditorVisible || !workspaceSettings.synchronizedScroll || win.synchronizingScroll || win.changingDocumentView || workspaceLayout.effectiveLayoutMode !== 1) return;
                win.synchronizingScroll = true;
                previewPane.scrollToFraction(contentY / Math.max(1, contentHeight - height));
                win.synchronizingScroll = false;
            }
            objectName: "editorScroll"
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            anchors.bottomMargin: documentFooter.height
            anchors.topMargin: documentMeta.y + documentMeta.height
            clip: true
            contentWidth: win.codeAppearance ? Math.max(width, editor.x + editor.contentWidth + 24) : width
            ScrollBar.horizontal: ScrollBar { policy: win.codeAppearance ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }
            contentHeight: Math.max(height, editor.y + editor.implicitHeight + (workspaceSettings.typewriter ? height / 2 : 220))
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                // Wheel scrolling moves contentY directly rather than
                // flicking the Flickable, so the bar has to be told about
                // that activity; linger briefly after the last event.
                active: hovered || pressed || wheelScroll.running || scrollLinger.running
                // Stop above the footer strip so the bar doesn't overlap
                // the word count in the bottom-right corner. Padding and
                // inset, not anchors: the attached-ScrollBar layout overrides
                // anchors. Padding stops the thumb, the inset the track.
                bottomPadding: win.scaledSize(32)
                bottomInset: win.scaledSize(32)
            }

            Timer {
                id: scrollLinger
                interval: 600
            }

            // Flickable turns a wheel notch into a flick sized by the small
            // application font, which crawls next to a browser. Reproduce
            // Chromium's wheel physics instead (cc::ScrollOffsetAnimationCurve):
            // each notch moves 3 lines of 40px towards a running target, the
            // animation gets shorter as the outstanding distance grows, and a
            // notch landing mid-animation carries the current velocity into
            // the new curve, so sustained spinning keeps picking up speed.
            readonly property real wheelStep: win.scaledSize(120)

            FrameAnimation {
                id: wheelScroll
                running: false

                property real startY: 0
                property real targetY: 0
                property real duration: 0.2
                // Cubic bezier easing; ease-in-out (0.42, 0, 0.58, 1) for a
                // fresh scroll, with y1 tilted on retarget so the curve's
                // initial slope matches the velocity it inherits.
                property real cx1: 0.42
                property real cy1: 0
                readonly property real cx2: 0.58
                readonly property real cy2: 1

                onTriggered: {
                    var x = elapsedTime / duration;
                    if (x >= 1) {
                        editorFlick.contentY = editorFlick.snapToPixel(targetY);
                        stop();
                        return;
                    }
                    editorFlick.contentY = editorFlick.snapToPixel(
                        startY + (targetY - startY) * curveY(solveCurve(x)));
                }

                function begin(from, to, dur, slope) {
                    startY = from;
                    targetY = to;
                    duration = dur;
                    cx1 = 0.42;
                    cy1 = 0.42 * Math.max(-1000, Math.min(1000, slope));
                    restart();
                }

                function retarget(newTarget) {
                    var s = solveCurve(Math.min(1, elapsedTime / duration));
                    var pos = startY + (targetY - startY) * curveY(s);
                    var delta = newTarget - pos;
                    if (Math.abs(delta) < 0.5) {
                        editorFlick.contentY = newTarget;
                        stop();
                        return;
                    }

                    var velocity = curveDY(s) / Math.max(1e-6, curveDX(s))
                        * (targetY - startY) / duration;
                    var dur = editorFlick.wheelDuration(delta);
                    // When already moving faster than the eased curve would,
                    // bound the duration by the time to target at the current
                    // velocity; the 2.5x covers the ease-out tail.
                    if (velocity !== 0 && delta / velocity > 0)
                        dur = Math.min(dur, delta / velocity * 2.5);
                    begin(pos, newTarget, dur, velocity * dur / delta);
                }

                // Cubic bezier through (0,0), (cx1,cy1), (cx2,cy2), (1,1),
                // evaluated by Newton-solving the curve parameter from x.
                function curveX(s) { return 3 * s * (1 - s) * ((1 - s) * cx1 + s * cx2) + s * s * s; }
                function curveY(s) { return 3 * s * (1 - s) * ((1 - s) * cy1 + s * cy2) + s * s * s; }
                function curveDX(s) { return 3 * (1 - s) * (1 - s) * cx1 + 6 * (1 - s) * s * (cx2 - cx1) + 3 * s * s * (1 - cx2); }
                function curveDY(s) { return 3 * (1 - s) * (1 - s) * cy1 + 6 * (1 - s) * s * (cy2 - cy1) + 3 * s * s * (1 - cy2); }

                function solveCurve(x) {
                    var s = x;
                    for (var i = 0; i < 8; ++i) {
                        var error = curveX(s) - x;
                        if (Math.abs(error) < 0.001)
                            break;
                        var d = curveDX(s);
                        if (Math.abs(d) < 1e-6)
                            break;
                        s = Math.max(0, Math.min(1, s - error / d));
                    }
                    return s;
                }
            }

            WheelHandler {
                // Wayland compositors route every pointer's scroll through
                // one seat device that Qt classifies as a touchpad, so the
                // device type cannot tell a mouse wheel from two-finger
                // scrolling. Distinguish by event shape instead: discrete
                // wheel notches arrive with only angleDelta set, while
                // finger scrolling carries pixel-precise pixelDelta.
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onWheel: function(wheel) {
                    win.writingActivity();
                    win.cancelDocumentViewportTransition();
                    scrollLinger.restart();
                    if (win.codeAppearance && (wheel.pixelDelta.x || wheel.angleDelta.x))
                        editorFlick.contentX = Math.max(0, Math.min(editorFlick.contentWidth - editorFlick.width, editorFlick.contentX - (wheel.pixelDelta.x || wheel.angleDelta.x)));
                    if (wheel.pixelDelta.y !== 0)
                        editorFlick.scrollTo(editorFlick.clampContentY(editorFlick.contentY - wheel.pixelDelta.y));
                    else
                        editorFlick.scrollByWheel(wheel);
                    wheel.accepted = true;
                }
            }

            onMovementStarted: { win.writingActivity(); win.cancelDocumentViewportTransition(); wheelScroll.stop(); }

            function scrollByWheel(wheel) {
                // High-resolution wheels report fractional notches; feed
                // those through the same animated path, like Chromium does
                // for every wheel-source event.
                var notches = wheel.angleDelta.y / 120;
                if (notches === 0)
                    return;

                if (wheelScroll.running) {
                    wheelScroll.retarget(clampContentY(wheelScroll.targetY - notches * wheelStep));
                    return;
                }

                var target = clampContentY(contentY - notches * wheelStep);
                if (target !== contentY)
                    wheelScroll.begin(contentY, target, wheelDuration(target - contentY), 0);
            }

            // Chromium's inverse-delta duration: 200ms for a single notch,
            // ramping down to 100ms once 480px are outstanding.
            function wheelDuration(delta) {
                var pixels = Math.abs(delta) / win.textScale;
                return Math.max(6, Math.min(12, 14 - pixels / 60)) / 60;
            }

            function clampContentY(y) {
                return Math.max(0, Math.min(Math.max(0, contentHeight - height), y));
            }

            // Whole device pixels keep natively hinted glyphs from
            // re-rasterizing mid-animation, which reads as shimmer.
            function snapToPixel(y) {
                return Math.round(y * Screen.devicePixelRatio) / Screen.devicePixelRatio;
            }

            // Jump to a position, abandoning any wheel animation still running.
            function scrollTo(y) {
                wheelScroll.stop();
                contentY = snapToPixel(y);
            }

            // Keep the editing caret within the viewport so writing past the
            // bottom edge scrolls the page along with the text.
            function ensureCursorVisible() {
                if (win.changingDocumentView) return;
                if (win.codeAppearance) {
                    var caretX = editor.x + editor.cursorRectangle.x;
                    if (caretX < contentX + 64) contentX = Math.max(0, caretX - 64);
                    else if (caretX > contentX + width - 24) contentX = Math.min(contentWidth - width, caretX - width + 24);
                }
                if (workspaceSettings.typewriter && editor.selectionStart === editor.selectionEnd) {
                    scrollTo(clampContentY(editor.y + editor.cursorRectangle.y - height / 2 + editor.cursorRectangle.height / 2));
                    return;
                }
                var margin = win.editorFontPixelSize * 2;
                var cursorTop = editor.y + editor.cursorRectangle.y;
                var cursorBottom = cursorTop + editor.cursorRectangle.height;
                var maxContentY = Math.max(0, contentHeight - height);

                if (cursorBottom + margin > contentY + height)
                    scrollTo(Math.min(maxContentY, cursorBottom + margin - height));
                else if (cursorTop - margin < contentY)
                    scrollTo(Math.max(0, cursorTop - margin));
            }

            TextEdit {
                id: editor
                objectName: "sourceEditor"
            Accessible.name: "Markdown editor"
                x: win.codeAppearance ? win.scaledSize(64) : Math.round((editorFlick.width - width) / 2)
                y: workspaceSettings.typewriter ? editorFlick.height / 2
                    : win.scaledSize(38)
                width: win.editorWidth
                height: Math.max(editorFlick.height - y - 96, implicitHeight + 20)
                text: ""
                textFormat: TextEdit.PlainText
                wrapMode: win.codeAppearance ? TextEdit.NoWrap : TextEdit.Wrap
                selectByMouse: true
                persistentSelection: true
                activeFocusOnPress: true
                color: win.textColor
                selectedTextColor: "#ffffff"
                selectionColor: win.selectionFill
                font.family: win.editorFontFamily
                font.pixelSize: win.editorFontPixelSize
                font.weight: Font.Normal
                onFontChanged: Qt.callLater(function() { backend.setSourceAppearance(win.activeWritingAppearance); codeDecorationsTimer.restart(); })
                onContentHeightChanged: codeDecorationsTimer.restart()
                // Native rendering hints glyphs to the pixel grid, which is
                // crispest at whole scale factors but misplaces and unevenly
                // rasterizes glyphs at fractional ones (and goes stale when
                // the compositor delivers the fractional scale after the
                // first frame). Fall back to Qt's scalable renderer there.
                renderType: Screen.devicePixelRatio % 1 === 0 ? TextEdit.NativeRendering : TextEdit.QtRendering
                cursorDelegate: Rectangle {
                    width: 1
                    color: win.strongTextColor
                }
                // Reflowing an inactive Source pane must not pull the preview
                // back to an old caret through synchronized scrolling.
                onCursorRectangleChanged: if (activeFocus && win.sourceEditorVisible) editorFlick.ensureCursorVisible()
                onActiveFocusChanged: if (activeFocus) {
                    Qt.callLater(win.updateWritingSurfaceFromFocus);
                    Qt.callLater(win.updateSourceFormattingOwner);
                }
                onCursorPositionChanged: backend.setFocusPosition(cursorPosition, workspaceSettings.paragraphFocus, workspaceSettings.sentenceFocus)

                function replaceSelectionWith(replacement) {
                    var start = Math.min(selectionStart, selectionEnd);
                    var end = Math.max(selectionStart, selectionEnd);
                    EditorMutations.replaceRange(editor, start, end, replacement);
                }

                function wrapSelection(before, after) {
                    forceActiveFocus();
                    var start = Math.min(selectionStart, selectionEnd);
                    var end = Math.max(selectionStart, selectionEnd);
                    var selection = backend.wrapSelection(start, end, before, after);
                    if (selection.start !== undefined)
                        select(selection.start, selection.end);
                }

                function replaceAtomic(start, end, text, selectionStartOffset, selectionEndOffset) {
                    var result;
                    win.performDocumentEdit(function() { result = backend.replaceText(start, end, text); });
                    if (result.start === undefined) return;
                    if (selectionStartOffset !== undefined)
                        select(result.start + selectionStartOffset, result.start + selectionEndOffset);
                    else cursorPosition = result.end;
                }

                function insertLink() {
                    var start = Math.min(selectionStart, selectionEnd);
                    var end = Math.max(selectionStart, selectionEnd);
                    var selected = text.slice(start, end);
                    var url = backend.clipboardUrl();
                    var label = selected.length > 0 ? selected : "link text";
                    var destination = url.length > 0 ? url : "https://";
                    var escapedLabel = escapeMarkdownLinkText(label);
                    var markdown = "[" + escapedLabel + "](" + escapeMarkdownLinkDestination(destination) + ")";
                    if (selected.length === 0) {
                        replaceAtomic(start, end, markdown,
                                                     1, 1 + escapedLabel.length);
                    } else if (url.length === 0) {
                        replaceAtomic(start, end, markdown,
                                                     escapedLabel.length + 3,
                                                     markdown.length - 1);
                    } else {
                        replaceAtomic(start, end, markdown);
                    }
                }

                function applyCodeEdit(action) {
                    var collapsed = selectionStart === selectionEnd;
                    var result;
                    win.performDocumentEdit(function() { result = backend.editCode(action, editor.selectionStart, editor.selectionEnd); });
                    if (result.start === undefined) return;
                    if (collapsed || action === "newline") cursorPosition = result.end;
                    else select(result.start, result.end);
                    codeDecorationsTimer.restart();
                }

                function smartReturn(softBreak) {
                    if (win.codeAppearance && !softBreak) { applyCodeEdit("newline"); return; }
                    if (softBreak) {
                        replaceSelectionWith("\n");
                        return;
                    }
                    var lineStart = text.lastIndexOf("\n", cursorPosition - 1) + 1;
                    var line = text.slice(lineStart, cursorPosition);
                    var before = text.slice(0, cursorPosition);
                    var fences = (before.match(/^\s*```/gm) || []).length;
                    if ((fences % 2) === 1) {
                        replaceSelectionWith("\n");
                        return;
                    }
                    var match = line.match(/^(\s*)([-+*]|\d+[.)]|>+)\s+(.*)$/);
                    if (match) {
                        if (match[3].length === 0) {
                            EditorMutations.replaceRange(editor, lineStart,
                                                         cursorPosition, "\n");
                        } else {
                            var marker = match[2];
                            if (/^\d/.test(marker))
                                marker = (parseInt(marker) + 1) + marker.slice(-1);
                            replaceSelectionWith("\n" + match[1] + marker + " ");
                        }
                        return;
                    }
                    replaceSelectionWith("\n\n");
                }

                function escapeMarkdownLinkText(linkText) {
                    return linkText.replace(/\\/g, "\\\\")
                                   .replace(/\[/g, "\\[")
                                   .replace(/\]/g, "\\]");
                }

                function escapeMarkdownLinkDestination(linkUrl) {
                    return linkUrl.replace(/\\/g, "\\\\")
                                  .replace(/\(/g, "\\(")
                                  .replace(/\)/g, "\\)");
                }

                function pasteClipboardUrlAsMarkdownLink() {
                    var start = Math.min(selectionStart, selectionEnd);
                    var end = Math.max(selectionStart, selectionEnd);
                    if (start === end)
                        return false;

                    var url = backend.clipboardUrl();
                    if (url === "")
                        return false;

                    var selected = text.slice(start, end);
                    var leading = selected.match(/^\s*/)[0];
                    var trailing = selected.match(/\s*$/)[0];
                    var linkText = selected.slice(leading.length,
                                                  selected.length - trailing.length);
                    if (linkText === "")
                        return false;

                    replaceSelectionWith(leading + "[" + escapeMarkdownLinkText(linkText) + "]("
                                         + escapeMarkdownLinkDestination(url) + ")" + trailing);
                    return true;
                }

                function pasteClipboardAsPlainText() {
                    var end;
                    win.performDocumentEdit(function() { end = backend.pasteWithAuthorship(selectionStart, selectionEnd); });
                    if (end >= 0) cursorPosition = end;
                }

                function skipHiddenForward(position) {
                    var pos = position;
                    var ranges = backend.hiddenRangesAt(pos);
                    for (var i = 0; i < ranges.length; i++) {
                        if (pos >= ranges[i].start && pos < ranges[i].end) {
                            pos = ranges[i].end;
                            i = -1;
                        }
                    }
                    return pos;
                }

                function skipHiddenBackward(position) {
                    var pos = position;
                    var ranges = backend.hiddenRangesAt(pos);
                    for (var i = ranges.length - 1; i >= 0; i--) {
                        if (pos > ranges[i].start && pos <= ranges[i].end) {
                            pos = ranges[i].start;
                            i = ranges.length;
                        }
                    }
                    return pos;
                }

                function moveCursorVisibly(direction) {
                    if (selectionStart !== selectionEnd) {
                        cursorPosition = direction > 0
                            ? Math.max(selectionStart, selectionEnd)
                            : Math.min(selectionStart, selectionEnd);
                        return;
                    }

                    var pos = Math.max(0, Math.min(text.length, cursorPosition + direction));
                    cursorPosition = direction > 0
                        ? skipHiddenForward(pos)
                        : skipHiddenBackward(pos);
                }

                function movePage(direction, extendSelection) {
                    var pageStep = Math.max(win.editorFontPixelSize,
                                            editorFlick.height - win.editorFontPixelSize * 2);
                    var rect = cursorRectangle;
                    var targetY = rect.y + rect.height / 2 + direction * pageStep;
                    var target = positionAt(rect.x, Math.max(0, targetY));
                    if (extendSelection)
                        moveCursorSelection(target, TextEdit.SelectCharacters);
                    else
                        cursorPosition = target;
                }

                function deleteParagraphBreakBehindCursor() {
                    if (selectionStart !== selectionEnd || cursorPosition < 2)
                        return false;

                    if (text.slice(cursorPosition - 2, cursorPosition) !== "\n\n")
                        return false;

                    var start = cursorPosition - 2;
                    remove(start, cursorPosition);
                    cursorPosition = start;
                    return true;
                }

                Keys.priority: Keys.BeforeItem
                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Alt) win.revealDocumentChrome();
                    else if ((event.text.length > 0 && !(event.modifiers & (Qt.ControlModifier | Qt.MetaModifier)))
                        || event.key === Qt.Key_Backspace || event.key === Qt.Key_Delete
                        || event.key === Qt.Key_PageUp || event.key === Qt.Key_PageDown
                        || event.key === Qt.Key_Up || event.key === Qt.Key_Down) win.writingActivity();
                    win.cancelDocumentViewportTransition();
                    if ((event.key === Qt.Key_C || event.key === Qt.Key_X) && (event.modifiers & Qt.ControlModifier)
                        && !(event.modifiers & (Qt.AltModifier | Qt.MetaModifier | Qt.ShiftModifier))) {
                        if (backend.copySelection(selectionStart, selectionEnd, "markdown") && event.key === Qt.Key_X)
                            win.performDocumentEdit(function() { editor.remove(editor.selectionStart, editor.selectionEnd); });
                        event.accepted = true; return;
                    }
                    var pasteKey = (event.key === Qt.Key_V)
                        && (event.modifiers & Qt.ControlModifier)
                        && !(event.modifiers & (Qt.AltModifier | Qt.MetaModifier | Qt.ShiftModifier));
                    var shiftInsert = (event.key === Qt.Key_Insert)
                        && (event.modifiers & Qt.ShiftModifier)
                        && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier));
                    if (pasteKey || shiftInsert) {
                        if (win.codeAppearance || !pasteClipboardUrlAsMarkdownLink())
                            pasteClipboardAsPlainText();
                        event.accepted = true;
                        return;
                    }

                    if (!(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))
                        && !win.codeAppearance && event.text === "\"" && win.insertSmartQuote()) {
                        event.accepted = true;
                        return;
                    }
                    if (!(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))
                        && !win.codeAppearance && event.text === "-" && win.insertSmartDash()) {
                        event.accepted = true;
                        return;
                    }

                    var returnKey = event.key === Qt.Key_Return || event.key === Qt.Key_Enter;
                    var commandModifier = event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier);
                    if (win.codeAppearance && !commandModifier && (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab)) {
                        applyCodeEdit((event.key === Qt.Key_Backtab || (event.modifiers & Qt.ShiftModifier)) ? "outdent" : "indent");
                        event.accepted = true;
                    } else if (returnKey && !commandModifier) {
                        smartReturn(event.modifiers & Qt.ShiftModifier);
                        event.accepted = true;
                    } else if (!commandModifier && event.key === Qt.Key_Backspace
                               && !win.codeAppearance && deleteParagraphBreakBehindCursor()) {
                        event.accepted = true;
                    } else if (!commandModifier && !(event.modifiers & Qt.ShiftModifier)
                               && event.key === Qt.Key_Right) {
                        moveCursorVisibly(1);
                        event.accepted = true;
                    } else if (!commandModifier && !(event.modifiers & Qt.ShiftModifier)
                               && event.key === Qt.Key_Left) {
                        moveCursorVisibly(-1);
                        event.accepted = true;
                    } else if (!commandModifier
                               && (event.key === Qt.Key_PageDown || event.key === Qt.Key_PageUp)) {
                        movePage(event.key === Qt.Key_PageDown ? 1 : -1,
                                 event.modifiers & Qt.ShiftModifier);
                        event.accepted = true;
                    }
                }

                onPreeditTextChanged: if (activeFocus && preeditText.length) win.writingActivity()
                onTextChanged: {
                    if (win.searchUpdating) return;
                    var contentChanged = backend.editorTextChanged();
                    if (!contentChanged) return;
                    codeDecorationsTimer.restart();
                    // Highlight/typography notifications must not cancel a view
                    // transaction; actual writing always takes precedence.
                    win.cancelDocumentViewportTransition();
                    if (workspaceSettings.styleCheckCustom || workspaceSettings.styleCheckFillers)
                        customReviewRefreshTimer.restart();
                    if (win.searchOpen)
                        win.updateSearch(editor.cursorPosition, !editor.activeFocus);
                }

                // A right-click on an underlined word offers corrections. Anywhere
                // else the press is declined, so default behaviour is untouched.
                MouseArea {
                    objectName: "sourceSpellingArea"
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onPressed: function(mouse) {
                        var position = editor.positionAt(mouse.x, mouse.y);
                        var hit = backend.spellCheck.enabled && !editor.inputMethodComposing
                            ? backend.misspelledWordAt(position) : ({});
                        if (hit.word !== undefined) { spellingMenu.showFor(Qt.point(mouse.x, mouse.y), hit); return; }
                        // Spelling first; then a blue grammar range.
                        var grammar = backend.spellCheck.enabled && backend.spellCheck.grammarEnabled && !editor.inputMethodComposing
                            ? backend.grammarIssueAt(position) : ({});
                        if (grammar.word === undefined) { mouse.accepted = false; return; }
                        grammarMenu.showFor(Qt.point(mouse.x, mouse.y), grammar);
                    }
                }

                Text {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    text: "# Start writing"
                    visible: editor.text.length === 0 && !editor.activeFocus
                    color: win.mutedColor
                    font.family: editor.font.family
                    font.pixelSize: editor.font.pixelSize
                    font.weight: editor.font.weight
                }

                Component.onCompleted: {
                    backend.setShowMarkup(true);
                    backend.attachDocument(textDocument);
                    backend.setSourceAppearance(win.activeWritingAppearance);
                    backend.setFocusPosition(cursorPosition, workspaceSettings.paragraphFocus, workspaceSettings.sentenceFocus);
                    if (workspaceSettings.styleCheckCustom || workspaceSettings.styleCheckFillers)
                        customReviewRefreshTimer.restart();
                    forceActiveFocus();
                }
            }
        }

        // The Live editor page (and its Chromium renderer) exists only while
        // Live editing is on; every other mode pays nothing for it.
        Loader {
            id: liveEditorLoader
            anchors.fill: parent
            anchors.topMargin: documentMeta.y + documentMeta.height
            // Created on first use and then kept (hidden) so switching modes does
            // not reload the editor page; documents that never use Live pay nothing.
            property bool used: false
            property int cursorOnEnter: -1
            onActiveChanged: if (active) used = true
            active: workspaceLayout.liveEditEnabled || used
            visible: workspaceLayout.liveEditEnabled
            sourceComponent: LiveEditorPane {
                cursorOnEnter: liveEditorLoader.cursorOnEnter
                bottomInset: documentFooter.height
                bridge: backend.editorBridge
                appearance: workspaceSettings.writingAppearance
                themeFilter: !workspaceSettings.liveThemeExact
                paletteBackground: backend.palette.editor
                paletteText: backend.palette.text
                fontFamily: win.editorFontFamily
                fontSize: win.editorFontPixelSize
                typewriter: workspaceSettings.typewriter
                focusMode: workspaceSettings.paragraphFocus || workspaceSettings.sentenceFocus
                dark: win.darkMode
                onWritingActivity: win.writingActivity()
                onNoticeRequested: function(message) { win.showNavigationNotice(message); }
                onScrollFractionChanged: function(fraction) {
                    if (!workspaceSettings.synchronizedScroll || win.synchronizingScroll || win.changingDocumentView || workspaceLayout.effectiveLayoutMode !== 1) return;
                    win.synchronizingScroll = true;
                    previewPane.scrollToFraction(fraction);
                    win.synchronizingScroll = false;
                }
            }
        }

        DocumentFindBar {
            id: searchPane
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 12
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            visible: win.searchOpen
            replaceVisible: win.replaceOpen
            matchCount: win.searchMatches.length
            matchIndex: win.searchMatchIndex
            z: 10
            onQueryEdited: if (win.searchOpen) win.updateSearch()
            onMoveRequested: function(direction) { win.moveSearch(direction); }
            onReplaceRequested: function(all) { win.replaceSearch(all); }
            onReplaceToggled: {
                win.replaceOpen = !win.replaceOpen;
                if (win.replaceOpen) focusReplacement();
                else focusQuery();
            }
            onCloseRequested: win.closeSearch()
        }
    }

        PublishingPreview {
            id: previewPane
            publishingMode: workspaceSettings.publishingFormat
            zoom: paneZoom.previewZoom / 100
            showFooter: false
            bottomInset: documentFooter.height
            suspendViewportUpdates: win.changingDocumentView
            onViewportInteraction: win.cancelDocumentViewportTransition()
            onWritingActivity: win.writingActivity()
            renderer: win.appBackend
            onTemplateMenuRequested: function(anchor) { win.openAnchoredMenu(previewTemplateMenu, anchor); }
            onScrollFractionChanged: function(fraction) {
                if (!workspaceSettings.synchronizedScroll || win.synchronizingScroll || win.changingDocumentView || workspaceLayout.effectiveLayoutMode !== 1) return;
                win.synchronizingScroll = true;
                if (workspaceLayout.liveEditEnabled) { if (liveEditorLoader.item) liveEditorLoader.item.scrollToFraction(fraction); }
                else editorFlick.contentY = Math.max(0, editorFlick.contentHeight - editorFlick.height) * fraction;
                win.synchronizingScroll = false;
            }
            markdown: editor.text
            documentBaseUrl: backend.documentBaseUrl
            darkMode: win.darkMode
            visible: workspaceLayout.effectiveLayoutMode !== 0
            SplitView.fillWidth: workspaceLayout.effectiveLayoutMode === 2
            SplitView.preferredWidth: workspaceLayout.effectivePreviewWidth
            SplitView.minimumWidth: 320
            typeface: backend.outputFont
            // Output zoom stays independent from either editing representation.
            textSize: Math.round(Math.max(17, backend.outputPointSize * 4 / 3) * paneZoom.previewZoom / 100)
            layoutMode: workspaceLayout.effectiveLayoutMode
            onLayoutRequested: function(mode) { win.setDocumentView(mode); }
            onAnchorNavigationFailed: function(anchor) {
                win.showNavigationNotice("Heading or anchor “" + anchor + "” was not found in this document.");
            }
            onLinkRequested: function(link) {
                var resolved = backend.resolveDocumentLink(link);
                if (/^file:.*\.(md|markdown|mdown|txt|text)(#.*)?$/i.test(String(resolved)))
                    win.requestOpen(resolved);
                else
                    backend.openExternalUrl(resolved);
            }
        }
    }

    // SplitView consumes pointer events on its delegates. Own both gestures in
    // this narrow document gutter; navigation dividers retain native resizing.
    MouseArea {
        id: documentDividerGestures
        objectName: "documentDividerGestures"
        visible: editorPane.visible && previewPane.visible
        x: workspaceSplit.x + editorPane.x + editorPane.width - 4
        y: workspaceSplit.y
        width: 9
        height: Math.max(0, workspaceSplit.height - documentFooter.height)
        z: 1
        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        preventStealing: true
        cursorShape: Qt.SplitHCursor
        property real pressSceneX: 0
        property real pressPreviewWidth: 0
        property var readingAnchors: null
        property bool balanceOnRelease: false
        function preserveReadingPosition() {
            if (!readingAnchors) return;
            win.beginDocumentViewportTransition(true);
            win.documentViewportTransition.sourceAnchor = readingAnchors.source;
            win.documentViewportTransition.previewAnchor = readingAnchors.preview;
        }
        onPressed: function(mouse) {
            pressSceneX = mapToItem(win.contentItem, mouse.x, mouse.y).x;
            pressPreviewWidth = previewPane.width;
            balanceOnRelease = false;
            win.beginDocumentViewportTransition(true);
            readingAnchors = { source: win.documentViewportTransition.sourceAnchor,
                preview: win.documentViewportTransition.previewAnchor };
        }
        onPositionChanged: function(mouse) {
            if (!pressed || balanceOnRelease || !readingAnchors) return;
            var delta = mapToItem(win.contentItem, mouse.x, mouse.y).x - pressSceneX;
            var total = editorPane.width + previewPane.width;
            var next = Math.max(320, Math.min(total - 480, pressPreviewWidth - delta));
            if (Math.abs(next - previewPane.width) < 0.5) return;
            preserveReadingPosition();
            workspaceLayout.updateWidth("preview", next);
        }
        onDoubleClicked: balanceOnRelease = true
        onReleased: {
            preserveReadingPosition();
            readingAnchors = null;
            if (balanceOnRelease) Qt.callLater(win.equalizeDocumentPanes);
            balanceOnRelease = false;
        }
        onCanceled: {
            preserveReadingPosition();
            readingAnchors = null;
            balanceOnRelease = false;
        }
        Rectangle {
            x: 4; width: 1; height: parent.height
            visible: documentDividerGestures.containsMouse || documentDividerGestures.pressed
            color: backend.palette.focus
        }
        Rectangle {
            anchors.centerIn: parent
            width: 3; height: 32; radius: 1.5
            visible: documentDividerGestures.containsMouse || documentDividerGestures.pressed
            color: backend.palette.focus
        }
        ToolTip.visible: containsMouse && !pressed
        ToolTip.delay: 700
        ToolTip.text: "Drag to resize panes · Double-click to balance"
    }

    DocumentFooter {
        id: documentFooter
        activityHidden: workspaceSettings.autoHideChrome && win.documentChromeHidden
        menuOpen: win.documentChromeMenuOpen
        objectName: "documentFooter"
        x: editorPane.visible ? editorPane.x : previewPane.x
        anchors.bottom: parent.bottom
        width: parent.width - x
        z: 2
        layoutMode: workspaceLayout.effectiveLayoutMode
        liveEditing: workspaceLayout.liveEditEnabled
        canSplit: workspaceLayout.availableWidth >= 800
        sourcePaneWidth: editorPane.visible ? editorPane.width : 0
        previewPaneStart: previewPane.visible ? previewPane.x - x : width
        writingAppearance: workspaceLayout.liveEditEnabled ? backend.publishingThemeName : win.codeAppearance ? "Code" : win.activeWritingAppearance === "editorial" ? "Editorial" : win.activeWritingAppearance === "book" ? "Book" : "Manuscript"
        previewTemplate: backend.publishingThemeName
        statusText: backend.status
        showStatus: !workspaceLayout.effectiveOrganizerVisible
        showStatistics: workspaceSettings.toolbarMode === 1
        statisticsText: win.compactToolbarStatistics()
        publishingFormat: workspaceSettings.publishingFormat
        onPublishingFormatRequested: function(format) { workspaceSettings.publishingFormat = format; }
        onLayoutRequested: function(mode) { win.setDocumentView(mode); }
        onSourceEditingRequested: win.setSourceEditing()
        onLiveEditingRequested: win.setLiveEditing(true)
        // In Live the left control names the Output Style, so it opens that menu (the same one as the publishing pane).
        onAppearanceMenuRequested: function(anchor) { win.openAnchoredMenu(workspaceLayout.liveEditEnabled ? previewTemplateMenu : sourceAppearanceMenu, anchor); }
        onTemplateMenuRequested: function(anchor) { win.openAnchoredMenu(previewTemplateMenu, anchor); }
        onStatisticsRequested: workspaceCommands.run("statistics")
    }

    Component.onCompleted: {
        Qt.callLater(function() { if (backend.publishingThemeError) win.showPublishingThemeNotice(backend.publishingThemeError); });
        workspaceLayout.filesVisible = workspaceSettings.libraryVisible;
        workspaceLayout.organizerVisible = workspaceSettings.organizerVisible;
        workspaceLayout.layoutMode = workspaceSettings.layoutMode;
        if (workspaceSettings.appearanceRevision < 1) {
            if (workspaceSettings.writingSize === 20) workspaceSettings.writingSize = 16;
            workspaceSettings.appearanceRevision = 1;
        }
        if (workspaceSettings.toolbarMode !== 0 && workspaceSettings.toolbarMode !== 1)
            workspaceSettings.toolbarMode = 0;
        if (workspaceSettings.titleBarMode !== 0 && workspaceSettings.titleBarMode !== 1)
            workspaceSettings.titleBarMode = 1;
        if (workspaceSettings.toolbarVisibilityMode < 0 || workspaceSettings.toolbarVisibilityMode > 2)
            workspaceSettings.toolbarVisibilityMode = 1;
        if (adoptReferenceWorkspace) applyReferenceWorkspace();
        paneZoom.initialize();
        Qt.callLater(win.refreshDocumentStatistics);
        var geometry = backend.windowGeometry();
        if (geometry.x >= 0) x = geometry.x;
        if (geometry.y >= 0) y = geometry.y;
        width = geometry.width;
        height = geometry.height;
        if (geometry.maximized) showMaximized();
    }

    Component.onDestruction: backend.saveWindowGeometry(x, y, width, height, visibility === Window.Maximized)

}
