pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Pdf
import QtWebEngine

Rectangle {
    id: root
    objectName: "previewPane"
    required property var renderer
    property string markdown: ""
    property url documentBaseUrl
    property string publishingMode: "web"
    property real zoom: 1
    // Compatible assignments from the document workspace; this surface is read-only.
    property bool allowVisualEdit: false
    property bool visualEditEnabled: false
    property string visualEditorObjectName: "outputPreviewVisualEditor"
    property bool showFooter: false
    property real bottomInset: 0
    property bool suspendViewportUpdates: false
    property bool darkMode: false
    property string typeface: "Helvetica Neue"
    property int textSize: 17
    property int layoutMode: 1
    property string renderedMarkdown: markdown
    property string pendingAnchor: ""
    property double pendingRequest: -1
    property bool previewRefreshInFlight: false
    readonly property bool viewportRefreshPending: refreshTimer.running || previewRefreshInFlight || webViewportRequests > 0
    property string errorText: ""
    property string warningText: ""
    property string displayedFormat: ""
    property int appliedRevision: -1
    property var retainedFrame: null
    property int paintedFrames: 0
    readonly property bool holdingPreviousFrame: retainedFrame !== null
    property url outputUrl
    property url webOutputUrl
    property bool pdfPagesReady: false
    property var pdfAnchors: ({})
    property real retainedFraction: 0
    property var webAnchor: null
    property bool updatingWebScroll: false
    property real webContentHeight: 0
    property int webViewportRequests: 0
    property string webRestorationKey: ""
    property bool webUserScrolling: false
    property int refreshRevision: 0
    property string scrollObjectName: "previewScroll"
    property string renderedObjectName: "renderedPreview"
    signal scrollFractionChanged(real fraction)
    signal viewportInteraction()
    signal writingActivity()
    function userInteraction(hideChrome = true) {
        webRestorationKey = "";
        viewportInteraction();
        if (hideChrome) writingActivity();
        if (publishingMode === "web") { webUserScrolling = true; userScrollExpiry.restart(); }
    }
    function scrollKey(event) {
        return event.key === Qt.Key_Up || event.key === Qt.Key_Down
            || event.key === Qt.Key_Left || event.key === Qt.Key_Right
            || event.key === Qt.Key_PageUp || event.key === Qt.Key_PageDown
            || event.key === Qt.Key_Home || event.key === Qt.Key_End || event.key === Qt.Key_Space;
    }
    signal anchorNavigationFailed(string anchor)
    signal linkRequested(url link)
    signal layoutRequested(int mode)
    signal templateMenuRequested(var anchor)
    color: renderer.palette.page

    function focusRenderedSurface() {
        if (publishingMode === "pdf") previewScroll.forceActiveFocus();
        else web.forceActiveFocus();
    }
    function viewportFraction() {
        return Math.max(0, Math.min(1, previewScroll.contentY / Math.max(1, previewScroll.contentHeight - previewScroll.height)));
    }
    function captureReadingAnchor() {
        return { publishingMode: publishingMode, fraction: viewportFraction(), dom: webAnchor };
    }
    function webViewportCommand(command, notify, completion) {
        if (publishingMode !== "web" || !webOutputUrl || String(web.url) !== String(webOutputUrl)) return;
        var revision = refreshRevision;
        ++webViewportRequests;
        var metrics = "var a=document.body.querySelectorAll('p,h1,h2,h3,h4,h5,h6,pre,table,li,img'),anchor=null;for(var i=0;i<a.length;i++){var r=a[i].getBoundingClientRect();if(r.bottom>0){anchor={index:i,offset:r.top};break;}}return {y:window.scrollY,height:Math.max(document.documentElement.scrollHeight,document.body.scrollHeight),anchor:anchor};";
        web.runJavaScript("(function(){if(!document.body)return null;" + command + metrics + "})()", 1, function(state) {
            --root.webViewportRequests;
            if (revision !== root.refreshRevision || root.publishingMode !== "web" || !state) return;
            root.updatingWebScroll = true;
            root.webContentHeight = state.height * root.zoom;
            previewScroll.contentY = state.y * root.zoom;
            root.webAnchor = state.anchor;
            root.updatingWebScroll = false;
            // Browser callbacks arrive after Main's synchronous scroll guard has
            // ended. Only physical scrolling may synchronize the other editor.
            if (notify && root.webUserScrolling && !root.suspendViewportUpdates && !root.previewRefreshInFlight)
                root.scrollFractionChanged(root.viewportFraction());
            if (completion) completion();
        });
    }
    function restoreReadingAnchor(anchor) {
        if (!anchor || viewportRefreshPending) return;
        if (publishingMode === "web" && anchor.publishingMode === "web" && anchor.dom) {
            var dom = anchor.dom;
            var targetKey = [viewportGeometry(), zoom, dom.index, dom.offset].join(":");
            if (targetKey === webRestorationKey) return;
            webViewportCommand("var e=document.body.querySelectorAll('p,h1,h2,h3,h4,h5,h6,pre,table,li,img')[" + Number(dom.index) + "];if(e)window.scrollTo(0,e.getBoundingClientRect().top+window.scrollY-(" + Number(dom.offset) + "));", false, function() { root.webRestorationKey = [root.viewportGeometry(), root.zoom, dom.index, dom.offset].join(":"); });
        } else scrollToFraction(anchor.fraction || 0);
    }
    function readingViewportKey() { return [viewportGeometry(), previewScroll.contentY, publishingMode].join(":"); }
    function viewportGeometry() { return [previewScroll.width, previewScroll.height, previewScroll.contentHeight].join(":"); }
    function stopViewportMotion() { previewScroll.cancelFlick(); }
    function scrollToFraction(fraction) {
        fraction = Math.max(0, Math.min(1, fraction));
        if (publishingMode === "web") {
            var targetKey = [viewportGeometry(), zoom, "fraction", fraction].join(":");
            if (targetKey === webRestorationKey) return;
            webViewportCommand("window.scrollTo(0,Math.max(0,document.documentElement.scrollHeight-window.innerHeight)*(" + fraction + "));", false, function() { root.webRestorationKey = [root.viewportGeometry(), root.zoom, "fraction", fraction].join(":"); });
        } else previewScroll.contentY = Math.max(0, previewScroll.contentHeight - previewScroll.height) * fraction;
    }
    function navigateToSourcePosition(position) {
        scrollToFraction(markdown.length ? position / markdown.length : 0);
    }
    function jumpToAnchor(anchor) {
        var decoded;
        try { decoded = decodeURIComponent(anchor); } catch (error) { anchorNavigationFailed(anchor); return false; }
        if (publishingMode === "pdf") {
            if (pdfAnchors[decoded] === undefined) { anchorNavigationFailed(decoded); return false; }
            var page = pages.itemAt(pdfAnchors[decoded]);
            if (page) previewScroll.contentY = page.y;
        } else {
            var revision = refreshRevision;
            web.runJavaScript("(function(){var n=" + JSON.stringify(decoded) + ";var e=document.getElementById(n)||document.getElementsByName(n)[0];if(!e)return false;e.scrollIntoView();return true;})()", 1,
                function(found) { if (revision === root.refreshRevision && !found) root.anchorNavigationFailed(decoded); });
        }
        return true;
    }
    function navigateToAnchor(anchor) {
        if (!viewportRefreshPending && outputUrl && errorText === "") jumpToAnchor(anchor);
        else { pendingAnchor = anchor; if (!viewportRefreshPending) reload(); }
    }
    function finishRefresh() {
        previewRefreshInFlight = false;
        displayedFormat = publishingMode;
        paintedFrames = 0;
        scrollToFraction(retainedFraction);
        if (pendingAnchor !== "") { var anchor = pendingAnchor; pendingAnchor = ""; jumpToAnchor(anchor); }
    }
    function refresh() {
        if (!visible) return;
        retainedFraction = viewportFraction();
        ++refreshRevision;
        webRestorationKey = "";
        previewRefreshInFlight = true;
        var result = renderer.requestPublishingPreview(publishingMode === "pdf" ? "pdf" : "html", root);
        pendingRequest = result.pending ? result.requestId : -1;
        if (result.pending) return;
        acceptOutput(result);
    }
    function acceptOutput(result) {
        errorText = result.ok ? "" : result.error;
        warningText = result.warning || "";
        if (!result.ok) { previewRefreshInFlight = false; retainedFrame = null; return; }
        // An unchanged refresh must not navigate Chromium or destroy PDF pages.
        if (String(result.url) === String(outputUrl)
                && (publishingMode === "web" ? String(web.url) === String(result.url)
                    : String(pdf.source) === String(result.url))) {
            appliedRevision = refreshRevision;
            if (publishingMode === "web" && !web.loading) finishRefresh();
            else if (publishingMode === "pdf" && pdf.status === PdfDocument.Ready) {
                pdfPagesReady = true;
                finishRefresh();
            }
            return;
        }
        // Keep the painted document visible while its replacement loads. A grab
        // is asynchronous, so discard it if newer typing superseded this result.
        if (displayedFormat === publishingMode && outputUrl && !holdingPreviousFrame) {
            var revision = refreshRevision;
            var surface = publishingMode === "pdf" ? previewScroll : web;
            var captured = surface.grabToImage(function(frame) {
                if (revision !== root.refreshRevision) return;
                root.retainedFrame = frame;
                root.applyOutput(result);
            });
            if (captured) return;
        }
        applyOutput(result);
    }
    function applyOutput(result) {
        appliedRevision = refreshRevision;
        paintedFrames = 0;
        pdfAnchors = result.anchors || ({});
        outputUrl = result.url;
        if (publishingMode === "web") webOutputUrl = result.url;
        else {
            // Destroy previous image delegates before their document changes
            // source; PdfPageImage caches its previous URL internally.
            pdfPagesReady = false;
            var revision = refreshRevision;
            Qt.callLater(function() {
                if (revision === root.refreshRevision && root.publishingMode === "pdf") pdf.source = result.url;
            });
        }
    }
    function reload() { ++refreshRevision; pendingRequest = -1; refreshTimer.restart(); }
    onMarkdownChanged: reload()
    onDocumentBaseUrlChanged: reload()
    onPublishingModeChanged: { webAnchor = null; reload(); }
    onVisibleChanged: if (visible) reload()
    Component.onCompleted: reload()
    Timer { id: userScrollExpiry; interval: 350; onTriggered: root.webUserScrolling = false }
    Timer { id: refreshTimer; interval: 220; onTriggered: root.refresh() }
    Connections {
        target: root.renderer
        function onOutputStyleChanged() { root.reload(); }
        function onOutputPageLayoutChanged() { root.reload(); }
        function onOutputCssChanged() { root.reload(); }
        function onPublishingThemesChanged() { root.reload(); }
        function onPublishingPreviewReady(requestId, result) {
            if (requestId !== root.pendingRequest || root.publishingMode !== "pdf") return;
            root.pendingRequest = -1;
            root.acceptOutput(result);
        }
    }
    PdfDocument {
        id: pdf
        onStatusChanged: function(status) {
            if (root.appliedRevision !== root.refreshRevision || root.publishingMode !== "pdf" || String(source) !== String(root.outputUrl)) return;
            if (status === PdfDocument.Ready) {
                root.pdfPagesReady = true;
                var revision = root.refreshRevision, loadedUrl = String(source);
                Qt.callLater(function() {
                    if (revision === root.refreshRevision && loadedUrl === String(root.outputUrl)) root.finishRefresh();
                });
            }
            else if (status === PdfDocument.Error) { root.errorText = "Could not read the publishing PDF."; root.previewRefreshInFlight = false; }
        }
    }
    Flickable {
        id: previewScroll
        objectName: root.scrollObjectName
        anchors.fill: parent
        anchors.bottomMargin: root.bottomInset
        visible: root.publishingMode === "pdf"
        clip: true
        contentWidth: Math.max(width, pdfPages.width + 32)
        contentHeight: root.publishingMode === "pdf" ? Math.max(height, pdfPages.height + 32) : Math.max(height, root.webContentHeight)
        boundsBehavior: Flickable.StopAtBounds
        onMovementStarted: root.userInteraction()
        onContentYChanged: {
            if (root.updatingWebScroll) return;
            if (root.publishingMode === "web") {
                root.webViewportCommand("window.scrollTo(0,(" + contentY + ")/(" + root.zoom + "));", false);
            } else if (root.visible && !root.suspendViewportUpdates && !root.previewRefreshInFlight)
                root.scrollFractionChanged(root.viewportFraction());
        }
        Keys.onPressed: function(event) { root.userInteraction(root.scrollKey(event)); }
        WheelHandler { target: null; blocking: false; onWheel: root.userInteraction() }
        TapHandler { onPressedChanged: if (pressed) root.userInteraction(false) }
        ScrollBar.vertical: ScrollBar {}
        ScrollBar.horizontal: ScrollBar {}
        Column {
            id: pdfPages
            x: Math.max(16, (previewScroll.width - width) / 2)
            y: 16
            spacing: 16
            Repeater {
                id: pages
                model: root.publishingMode === "pdf" && root.pdfPagesReady ? pdf.pageCount : 0
                Rectangle {
                    id: paper
                    required property int index
                    readonly property size points: pdf.pagePointSize(index)
                    readonly property real pageScale: Math.max(0.1, (previewScroll.width - 32) / Math.max(1, points.width)) * root.zoom
                    width: points.width * pageScale
                    height: points.height * pageScale
                    color: "white"
                    readonly property bool painted: !pageLoader.active || (pageLoader.item !== null && pageLoader.item.status === Image.Ready)
                    Loader {
                        id: pageLoader
                        anchors.fill: parent
                        active: root.visible && paper.y + paper.height >= previewScroll.contentY - previewScroll.height
                            && paper.y <= previewScroll.contentY + 2 * previewScroll.height
                        sourceComponent: PdfPageImage {
                        id: pageImage
                        anchors.fill: parent
                        document: pdf
                        currentFrame: paper.index
                        sourceSize.width: Math.ceil(width * Screen.devicePixelRatio)
                        sourceSize.height: Math.ceil(height * Screen.devicePixelRatio)
                        asynchronous: true
                        Accessible.name: "Publishing PDF page " + (paper.index + 1)
                        }
                    }
                    Repeater {
                        model: PdfLinkModel { document: root.visible && paper.y + paper.height >= previewScroll.contentY - previewScroll.height
                            && paper.y <= previewScroll.contentY + 2 * previewScroll.height ? pdf : null; page: paper.index }
                        delegate: MouseArea {
                            required property rect rectangle
                            required property url url
                            required property int page
                            required property point location
                            x: rectangle.x * paper.pageScale
                            y: rectangle.y * paper.pageScale
                            width: rectangle.width * paper.pageScale
                            height: rectangle.height * paper.pageScale
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.viewportInteraction();
                                if (page >= 0) {
                                    var destination = pages.itemAt(page);
                                    if (destination) previewScroll.contentY = destination.y + location.y * Math.max(0.1, (previewScroll.width - 32) / Math.max(1, pdf.pagePointSize(page).width)) * root.zoom;
                                } else root.linkRequested(url);
                            }
                        }
                    }
                }
            }
        }
    }
    WebEngineView {
        id: web
        objectName: root.renderedObjectName
        readonly property bool readOnly: true
        anchors.fill: parent
        anchors.bottomMargin: root.bottomInset
        visible: root.publishingMode === "web"
        url: root.webOutputUrl
        zoomFactor: root.zoom
        backgroundColor: "white"
        settings.javascriptEnabled: false
        settings.localContentCanAccessFileUrls: true
        settings.localContentCanAccessRemoteUrls: false
        settings.pluginsEnabled: false
        onNavigationRequested: function(request) {
            if (request.navigationType !== WebEngineView.LinkClickedNavigation
                    && String(request.url).split("#")[0] !== String(root.webOutputUrl).split("#")[0]) {
                request.action = WebEngineView.IgnoreRequest;
                return;
            }
            if (request.navigationType === WebEngineView.LinkClickedNavigation) {
                request.action = WebEngineView.IgnoreRequest;
                root.viewportInteraction();
                var link = String(request.url);
                if (link.charAt(0) === "#") root.jumpToAnchor(link.slice(1));
                else if (String(request.url).split("#")[0] === String(root.documentBaseUrl).split("#")[0] && link.indexOf("#") >= 0)
                    root.jumpToAnchor(link.slice(link.indexOf("#") + 1));
                else root.linkRequested(request.url);
            }
        }
        onNewWindowRequested: function(request) { root.linkRequested(request.requestedUrl); }
        onContextMenuRequested: function(request) { request.accepted = true; }
        onLoadingChanged: function(info) {
            if (root.appliedRevision !== root.refreshRevision || root.publishingMode !== "web" || String(info.url) !== String(root.outputUrl)) return;
            if (info.status === WebEngineView.LoadSucceededStatus) root.webViewportCommand("", false, root.finishRefresh);
            else if (info.status === WebEngineView.LoadFailedStatus) { root.errorText = info.errorString; root.previewRefreshInFlight = false; }
        }
        onScrollPositionChanged: if (!root.previewRefreshInFlight && root.publishingMode === "web") root.webViewportCommand("", true)
        onContentsSizeChanged: if (!root.previewRefreshInFlight && root.publishingMode === "web") root.webViewportCommand("", false)
        TapHandler { onPressedChanged: if (pressed) root.userInteraction(false) }
        Keys.onPressed: function(event) { root.userInteraction(root.scrollKey(event)); }
        Accessible.name: "Web publishing preview, read-only"
    }
    // Chromium owns an internal focus/input item, so QML pointer handlers on
    // WebEngineView do not reliably see its wheels. Observe physical wheel
    // input above it and scroll the real browser DOM; clicks pass through.
    MouseArea {
        anchors.fill: web
        visible: web.visible
        acceptedButtons: Qt.NoButton
        onWheel: function(wheel) {
            root.userInteraction();
            var dx = wheel.pixelDelta.x !== 0 ? wheel.pixelDelta.x : wheel.angleDelta.x;
            var dy = wheel.pixelDelta.y !== 0 ? wheel.pixelDelta.y : wheel.angleDelta.y;
            root.webViewportCommand("window.scrollBy(" + (-dx / root.zoom) + "," + (-dy / root.zoom) + ");", true);
            wheel.accepted = true;
        }
    }
    Image {
        anchors.fill: web
        source: root.retainedFrame ? root.retainedFrame.url : ""
        visible: root.holdingPreviousFrame
        fillMode: Image.Stretch
        z: 1
        // This overlay does not accept input or change document geometry.
    }
    FrameAnimation {
        running: root.holdingPreviousFrame && root.visible && !root.viewportRefreshPending
        onTriggered: {
            if (root.publishingMode === "pdf") {
                if (!root.pdfPagesReady) return;
                for (var i = 0; i < pages.count; ++i) {
                    var page = pages.itemAt(i);
                    if (page && !page.painted) { root.paintedFrames = 0; return; }
                }
            }
            // Load completion precedes the compositor. Keep the old frame for
            // three rendered frames, including async PDF image readiness.
            if (++root.paintedFrames >= 3) root.retainedFrame = null;
        }
    }
    Label {
        objectName: "publishingImageNotice"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        anchors.bottomMargin: root.bottomInset + 12
        visible: root.warningText !== "" && root.errorText === ""
        text: root.warningText
        wrapMode: Text.WordWrap
        color: root.renderer.palette.text
        padding: 10
        z: 2
        background: Rectangle { color: root.renderer.palette.panel; radius: 6 }
        Accessible.name: text
    }
    Label {
        z: 2
        anchors.centerIn: parent
        width: Math.max(100, parent.width - 48)
        visible: root.errorText !== ""
        text: root.errorText
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        color: root.renderer.palette.text
        padding: 16
        background: Rectangle { color: root.renderer.palette.panel; radius: 8 }
    }
}
