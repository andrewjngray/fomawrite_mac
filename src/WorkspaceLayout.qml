import QtQuick

// User intent is per window. Responsive contraction changes only effective state.
QtObject {
    id: root
    objectName: "workspaceLayout"

    property bool organizerVisible: true
    property bool filesVisible: true
    property bool reviewVisible: false // The Spelling and Grammar pane, right of the document.
    property int layoutMode: 1 // 0 Single editor, 1 Editor + Preview, 2 Preview only
    property int lastEditingLayoutMode: 1 // Restore this arrangement when leaving Preview only.
    property bool liveEditEnabled: false // The CodeMirror Live editor replaces the Source/Visual surface.
    property real organizerWidth: 208
    property real fileWidth: 288
    property real previewWidth: 420
    property real reviewWidth: 320
    property real availableWidth: 1100 // Content width, excluding splitter handles.
    property string activeSurface: "source"

    readonly property bool effectiveOrganizerVisible: _effective.organizerVisible
    readonly property bool effectiveFilesVisible: _effective.filesVisible
    readonly property bool effectiveReviewVisible: _effective.reviewVisible
    readonly property int effectiveLayoutMode: _effective.layoutMode
    readonly property real effectiveOrganizerWidth: _effective.organizerWidth
    readonly property real effectiveFileWidth: _effective.fileWidth
    readonly property real effectivePreviewWidth: _effective.previewWidth
    readonly property real effectiveReviewWidth: _effective.reviewWidth
    readonly property real effectiveSourceWidth: _effective.sourceWidth
    readonly property real restorationMargin: 48

    property var _effective: ({ organizerVisible: false, filesVisible: false, reviewVisible: false,
                                 layoutMode: 0, organizerWidth: 0, fileWidth: 0, reviewWidth: 0,
                                 previewWidth: 0, sourceWidth: 0 })
    property int _collapseLevel: 0
    property bool _batching: false

    function boundedWidth(value, minimum, maximum, fallback) {
        return typeof value === "number" && isFinite(value)
                ? Math.max(minimum, Math.min(maximum, value)) : fallback
    }

    function desiredMode() {
        return layoutMode >= 0 && layoutMode <= 2 ? layoutMode : 0
    }

    function modeAt(level) {
        if (desiredMode() !== 1 || level < 3)
            return desiredMode()
        // Contraction keeps the chosen editor; focus never changes editing mode.
        return 0
    }

    function minimumAt(level) {
        var mode = modeAt(level)
        return (organizerVisible && level < 1 ? 184 : 0)
                + (filesVisible && level < 2 ? 232 : 0)
                + (reviewVisible ? 240 : 0)
                + (mode === 1 ? 800 : 320)
    }

    function recalculate(resetContraction) {
        if (_batching)
            return
        var width = typeof availableWidth === "number" && isFinite(availableWidth)
                ? Math.max(0, availableWidth) : 0
        var level = resetContraction ? 0 : _collapseLevel
        while (level < 3 && width < minimumAt(level))
            ++level
        // Only growing past a genuine pane-restoration boundary needs the margin.
        // Skip levels that represent a manually hidden pane.
        while (level > 0) {
            var next = level - 1
            var changesPresentation = (next === 0 && organizerVisible)
                    || (next === 1 && filesVisible)
                    || (next === 2 && desiredMode() === 1)
            if (changesPresentation && width < minimumAt(next) + restorationMargin)
                break
            --level
        }
        _collapseLevel = level
        var showOrganizer = organizerVisible && level < 1
        var showFiles = filesVisible && level < 2
        var mode = modeAt(level)
        // The review pane is the last to give way: it is dropped only when even
        // the narrowest arrangement leaves no room for it beside the document.
        var showReview = reviewVisible && width >= minimumAt(level)
        var organizer = showOrganizer ? boundedWidth(organizerWidth, 184, 288, 208) : 0
        var files = showFiles ? boundedWidth(fileWidth, 232, 420, 288) : 0
        var preview = mode === 1 ? boundedWidth(previewWidth, 320, 2400, 420) : 0
        var review = showReview ? boundedWidth(reviewWidth, 240, 640, 320) : 0
        var documentMinimum = mode === 1 ? 480 : 320
        var excess = Math.max(0, organizer + files + preview + review + documentMinimum - width)
        var reduction = showOrganizer ? Math.min(excess, organizer - 184) : 0
        organizer -= reduction
        excess -= reduction
        reduction = showFiles ? Math.min(excess, files - 232) : 0
        files -= reduction
        excess -= reduction
        reduction = mode === 1 ? Math.min(excess, preview - 320) : 0
        preview -= reduction
        excess -= reduction
        reduction = showReview ? Math.min(excess, review - 240) : 0
        review -= reduction
        var documentWidth = Math.max(0, width - organizer - files - review)
        _effective = { organizerVisible: showOrganizer, filesVisible: showFiles, reviewVisible: showReview,
            layoutMode: mode, organizerWidth: organizer, fileWidth: files, reviewWidth: review,
            previewWidth: mode === 2 ? documentWidth : preview,
            sourceWidth: mode === 2 ? 0 : Math.max(0, documentWidth - preview) }
    }

    // Call only for a user resize; never pass an automatically contracted width.
    function updateWidth(pane, width) {
        if (pane === "organizer")
            organizerWidth = boundedWidth(width, 184, 288, organizerWidth)
        else if (pane === "files")
            fileWidth = boundedWidth(width, 232, 420, fileWidth)
        else if (pane === "preview")
            previewWidth = boundedWidth(width, 320, 2400, previewWidth)
        else if (pane === "review")
            reviewWidth = boundedWidth(width, 240, 640, reviewWidth)
    }

    function saveState() {
        return { version: 2, organizerVisible: organizerVisible, filesVisible: filesVisible,
            reviewVisible: reviewVisible,
            layoutMode: desiredMode(), liveEditEnabled: liveEditEnabled,
            lastEditingLayoutMode: lastEditingLayoutMode === 1 ? 1 : 0,
            organizerWidth: boundedWidth(organizerWidth, 184, 288, 208),
            fileWidth: boundedWidth(fileWidth, 232, 420, 288),
            previewWidth: boundedWidth(previewWidth, 320, 2400, 420),
            reviewWidth: boundedWidth(reviewWidth, 240, 640, 320) }
    }

    function restoreState(state) {
        if (!state || typeof state !== "object" || (state.version !== 1 && state.version !== 2))
            return false
        var restoredMode = typeof state.layoutMode === "number" && state.layoutMode % 1 === 0
                && state.layoutMode >= 0 && state.layoutMode <= 2 ? state.layoutMode : 1
        // Visual Edit (retired in Cycle 135) is carried over as Live editing.
        // Version 1 combined editing mode and arrangement: Full Visual Edit
        // was mode 2, while mode 0 always selected Source.
        var restoredVisual = typeof state.visualEditEnabled === "boolean" ? state.visualEditEnabled : false
        if (state.version === 1) {
            if (restoredMode === 0) restoredVisual = false
            else if (restoredMode === 2 && restoredVisual) restoredMode = 0
        }
        _batching = true
        organizerVisible = typeof state.organizerVisible === "boolean" ? state.organizerVisible : true
        filesVisible = typeof state.filesVisible === "boolean" ? state.filesVisible : true
        reviewVisible = typeof state.reviewVisible === "boolean" ? state.reviewVisible : false
        reviewWidth = boundedWidth(state.reviewWidth, 240, 640, 320)
        layoutMode = restoredMode
        liveEditEnabled = restoredVisual || (typeof state.liveEditEnabled === "boolean" ? state.liveEditEnabled : false)
        lastEditingLayoutMode = restoredMode !== 2 ? restoredMode
                : state.version === 2 && state.lastEditingLayoutMode === 1 ? 1 : 0
        organizerWidth = boundedWidth(state.organizerWidth, 184, 288, 208)
        fileWidth = boundedWidth(state.fileWidth, 232, 420, 288)
        previewWidth = boundedWidth(state.previewWidth, 320, 2400, 420)
        _batching = false
        recalculate(true)
        return true
    }

    function restoreDefaults() {
        restoreState({ version: 2, organizerVisible: true, filesVisible: true,
            layoutMode: 1, liveEditEnabled: false, lastEditingLayoutMode: 1,
            organizerWidth: 208, fileWidth: 288, previewWidth: 420 })
    }

    onOrganizerVisibleChanged: recalculate(true)
    onFilesVisibleChanged: recalculate(true)
    onReviewVisibleChanged: recalculate(true)
    onLayoutModeChanged: {
        if (!_batching && (layoutMode === 0 || layoutMode === 1))
            lastEditingLayoutMode = layoutMode
        recalculate(true)
    }
    onOrganizerWidthChanged: recalculate(false)
    onFileWidthChanged: recalculate(false)
    onPreviewWidthChanged: recalculate(false)
    onReviewWidthChanged: recalculate(false)
    onAvailableWidthChanged: recalculate(false)
    onActiveSurfaceChanged: recalculate(false)
    Component.onCompleted: recalculate(true)
}
