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
    signal writingActivity()

    function pushAppearance() {
        if (!root.bridge) return;
        root.bridge.applyAppearance(JSON.stringify({ appearance: root.appearance, fontFamily: root.fontFamily,
            fontSize: root.fontSize, typewriter: root.typewriter, focus: root.focusMode, dark: root.dark }));
    }
    onAppearanceChanged: pushAppearance()
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
        web.url = "qrc:/editor/index.html";
        // Created lazily when Live is switched on: push the document straight
        // away (the bridge replays it once the page connects).
        if (visible) backend.syncLiveEditor();
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
        Keys.onPressed: root.writingActivity()
        Accessible.name: "Live Markdown editor"
    }

    onVisibleChanged: if (visible) backend.syncLiveEditor()
    Connections {
        target: backend
        function onDocumentLoaded() { if (root.visible) backend.syncLiveEditor(); }
    }
}
