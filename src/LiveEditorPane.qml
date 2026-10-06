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
    signal writingActivity()

    color: backend.palette.editor

    WebChannel { id: channel }
    Component.onCompleted: channel.registerObject("bridge", root.bridge)

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
        url: "qrc:/editor/index.html"
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
        Keys.onPressed: root.writingActivity()
        Accessible.name: "Live Markdown editor"
    }

    onVisibleChanged: if (visible) backend.syncLiveEditor()
    Connections {
        target: backend
        function onDocumentLoaded() { if (root.visible) backend.syncLiveEditor(); }
    }
}
