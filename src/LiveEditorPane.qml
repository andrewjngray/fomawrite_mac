import QtQuick
import QtWebEngine
import QtWebChannel

// The Live editing surface: a CodeMirror 6 Markdown editor (src/editor) hosted
// in Chromium and connected to the C++ document through backend.editorBridge.
// While this pane is active the editor's document is canonical; every change
// is mirrored into the QTextDocument so saving, recovery, publishing and the
// other panes see the same text.
Rectangle {
    id: root
    objectName: "liveEditorPane"
    required property var bridge
    property real bottomInset: 0
    // Presentation follows the same preferences as the Source editor.
    property string appearance: "manuscript"
    property string fontFamily: ""
    property int fontSize: 17
    property bool typewriter: false
    property bool focusMode: false
    property bool dark: false
    // Live follows the publishing theme with its layout filtered out unless the
    // writer asks for the exact theme (View > Live Follows Theme Exactly).
    property bool themeFilter: true
    property string paletteBackground: ""
    property string paletteText: ""
    // Caret to place when the pane becomes visible (carried over from Source).
    property int cursorOnEnter: -1
    signal writingActivity()
    signal scrollFractionChanged(real fraction)
    // A page-side refusal the writer should see (e.g. an image that could not be saved).
    signal noticeRequested(string message)
    // The app's Format commands apply to the page while it has keyboard focus.
    readonly property bool hasLiveFocus: web.activeFocus

    function scrollToFraction(fraction) { if (root.bridge) root.bridge.scrollTo(fraction); }

    function pushAppearance() {
        if (!root.bridge) return;
        root.bridge.applyAppearance(JSON.stringify({ appearance: root.appearance, fontFamily: root.fontFamily,
            fontSize: root.fontSize, typewriter: root.typewriter, focus: root.focusMode, dark: root.dark,
            liveThemeFilter: root.themeFilter, palette: { background: root.paletteBackground, text: root.paletteText } }));
    }
    onAppearanceChanged: pushAppearance()
    onThemeFilterChanged: pushAppearance()
    onPaletteBackgroundChanged: pushAppearance()
    onPaletteTextChanged: pushAppearance()
    onFontFamilyChanged: pushAppearance()
    onFontSizeChanged: pushAppearance()
    onTypewriterChanged: pushAppearance()
    onFocusModeChanged: pushAppearance()
    onDarkChanged: pushAppearance()

    color: backend.palette.editor

    WebChannel { id: channel }
    // Register the bridge before the page starts loading: QWebChannel announces
    // its objects when the page initialises the channel.
    Component.onCompleted: {
        channel.registerObject("bridge", root.bridge);
        pushAppearance();
        backend.installHarperScheme(); // fomawrite://harper/* for the engine the page loads on demand
        web.url = "qrc:/editor/index.html";
        // Created lazily when Live is switched on: push the document straight
        // away (the bridge replays it once the page connects).
        if (visible) { backend.syncLiveEditor(root.cursorOnEnter); root.cursorOnEnter = -1; }
    }

    // Edit-menu clipboard actions while the page has focus (Chromium owns the clipboard there).
    function triggerWebAction(name) {
        var action = { "Cut": WebEngineView.Cut, "Copy": WebEngineView.Copy, "Paste": WebEngineView.Paste, "SelectAll": WebEngineView.SelectAll }[name];
        if (action === undefined) return;
        web.triggerWebAction(action);
        web.forceActiveFocus();
    }
    function focusLive() {
        web.forceActiveFocus();
        if (root.bridge) root.bridge.focus();
    }

    WebEngineView {
        id: web
        objectName: "liveEditorView"
        anchors.fill: parent
        anchors.bottomMargin: root.bottomInset
        webChannel: channel
        backgroundColor: root.color
        settings.javascriptEnabled: true
        settings.localContentCanAccessFileUrls: false
        settings.localContentCanAccessRemoteUrls: false
        settings.pluginsEnabled: false
        onNavigationRequested: function(request) {
            // The page is the editor; links inside the document are text, not navigation.
            if (request.navigationType === WebEngineView.LinkClickedNavigation
                    || String(request.url).indexOf("qrc:/editor/") !== 0)
                request.action = WebEngineView.IgnoreRequest;
        }
        onContextMenuRequested: function(request) { request.accepted = true; }
        onJavaScriptConsoleMessage: function(level, message, lineNumber, sourceID) {
            console.warn("Live editor page: " + message + " (" + sourceID + ":" + lineNumber + ")");
        }
        onLoadingChanged: function(info) {
            if (info.status === WebEngineView.LoadFailedStatus) console.warn("Live editor page failed to load: " + info.errorString);
        }
        onRenderProcessTerminated: function(terminationStatus, exitCode) { web.reload(); }
        Keys.onPressed: root.writingActivity()
        Accessible.name: "Live Markdown editor"
    }

    onVisibleChanged: if (visible) { backend.syncLiveEditor(root.cursorOnEnter); root.cursorOnEnter = -1; }
    Connections {
        target: backend
        function onDocumentLoaded() { if (root.visible) backend.syncLiveEditor(); }
        function onPublishingCssChanged() { if (root.visible) backend.pushLiveTheme(); }
        function onOutputStyleChanged() { if (root.visible) backend.pushLiveTheme(); }
    }
    Connections {
        target: root.bridge
        function onScrollFractionChanged(fraction) { root.scrollFractionChanged(fraction); }
        function onMessageLogged(message) {
            if (message.indexOf("image not saved: ") === 0) root.noticeRequested(message.substring(17));
            else if (message.indexOf("image ignored") === 0) root.noticeRequested(message);
        }
    }
}
