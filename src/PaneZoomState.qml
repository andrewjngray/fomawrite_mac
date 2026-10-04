import QtQuick

// Screen magnification is independent of Markdown and output typography.
QtObject {
    id: root
    objectName: "paneZoomState"
    required property var settings
    property string effectivePane: "source"
    property int legacyAppearanceOffset: 1
    readonly property int sourceZoom: settings.sourceZoom
    readonly property int previewZoom: settings.previewZoom
    readonly property bool linked: settings.linkedZoom
    readonly property int minimumZoom: 75
    readonly property int maximumZoom: 200
    readonly property int activeZoom: zoomFor(effectivePane)
    signal beforeZoomChanged()

    function bounded(value) {
        return isFinite(value) ? Math.max(minimumZoom, Math.min(maximumZoom, Math.round(value))) : 100
    }
    function zoomFor(pane) { return pane === "preview" ? previewZoom : sourceZoom }
    function initialize() {
        if (settings.zoomRevision < 1) {
            // Retain the existing Source size as closely as whole pixels allow.
            // Preview adopts the new readable baseline and independent setting.
            settings.sourceZoom = bounded((settings.writingSize + legacyAppearanceOffset)
                / (16 + legacyAppearanceOffset) * 100)
            settings.previewZoom = 100
            settings.linkedZoom = false
            settings.zoomRevision = 1
        }
        settings.sourceZoom = bounded(settings.sourceZoom)
        settings.previewZoom = bounded(settings.previewZoom)
        if (settings.linkedZoom) settings.previewZoom = settings.sourceZoom
    }
    function setZoom(pane, percent) {
        var target = bounded(percent)
        var source = linked || pane !== "preview" ? target : sourceZoom
        var preview = linked || pane === "preview" ? target : previewZoom
        if (source === sourceZoom && preview === previewZoom) return
        beforeZoomChanged()
        settings.sourceZoom = source
        settings.previewZoom = preview
    }
    function adjustZoom(pane, delta) { setZoom(pane, zoomFor(pane) + delta) }
    function resetZoom(pane) { setZoom(pane, 100) }
    function setLinked(enabled, pane) {
        var target = zoomFor(pane === undefined ? effectivePane : pane)
        settings.linkedZoom = enabled
        if (enabled) setZoom(pane === undefined ? effectivePane : pane, target)
    }
    function resetAll() {
        beforeZoomChanged()
        settings.linkedZoom = false
        settings.sourceZoom = 100
        settings.previewZoom = 100
        settings.zoomRevision = 1
    }
}
