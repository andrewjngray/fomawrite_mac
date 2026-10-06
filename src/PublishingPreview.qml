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
    // A document lifetime is distinct from its asset directory and source text.
    property string documentIdentity: renderer ? String(renderer.publishingDocumentIdentity || renderer.fileUrl || documentBaseUrl) : ""
    property string displayedDocumentIdentity: ""
    property string appliedDocumentIdentity: ""
    property string requestToken: ""
    readonly property url actualPdfSource: pdf.source
    readonly property bool pdfVisiblePagesPainted: {
        if (!pdfPagesReady || pages.count === 0) return false;
        for (var i = 0; i < pages.count; ++i) {
            var page = pages.itemAt(i);
            if (page && !page.painted) return false;
        }
        return true;
    }
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
    readonly property bool holdingPreviousFrame: false
    property url outputUrl
    property url webOutputUrl
    property bool pdfPagesReady: false
    property var pdfAnchors: ({})
    property real retainedFraction: 0
    property var webAnchor: null
    property bool updatingWebScroll: false
    property real webContentHeight: 0
    property int webViewportRequests: 0
    property int webCommandEpoch: 0
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
        var commandEpoch = webCommandEpoch;
        ++webViewportRequests;
        var metrics = "var a=document.body.querySelectorAll('p,h1,h2,h3,h4,h5,h6,pre,table,li,img'),anchor=null;for(var i=0;i<a.length;i++){var r=a[i].getBoundingClientRect();if(r.bottom>0){anchor={index:i,offset:r.top};break;}}return {y:window.scrollY,height:Math.max(document.documentElement.scrollHeight,document.body.scrollHeight),anchor:anchor};";
        web.runJavaScript("(function(){if(!document.body)return null;" + command + metrics + "})()", 1, function(state) {
            if (commandEpoch !== root.webCommandEpoch) return;
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
    function clearOutput() {
        appliedRevision = -1;
        appliedDocumentIdentity = "";
        displayedDocumentIdentity = "";
        displayedFormat = "";
        pdfPagesReady = false;
        outputUrl = "";
        webOutputUrl = "";
        pdfAnchors = ({});
        webAnchor = null;
        retainedFraction = 0;
        previewScroll.contentY = 0;
        errorText = "";
        warningText = "";
        pendingAnchor = "";
    }
    function cancelRequest() {
        pendingRequest = -1;
        requestToken = "";
        if (renderer && renderer.cancelPublishingPreview) renderer.cancelPublishingPreview(root);
        refreshDeadline.stop();
        previewRefreshInFlight = false;
        ++webCommandEpoch;
        webViewportRequests = 0;
    }
    function failRefresh(message) {
        cancelRequest();
        // Keep the last readable output that belongs to this document behind
        // the notice: a timeout or transient failure must not blank the pane.
        // Output from another document is never kept.
        if (appliedDocumentIdentity !== documentIdentity || String(outputUrl) === "") clearOutput();
        errorText = message;
    }
    function finishRefresh() {
        if (appliedRevision !== refreshRevision || appliedDocumentIdentity !== documentIdentity) return;
        previewRefreshInFlight = false;
        refreshDeadline.stop();
        displayedFormat = publishingMode;
        displayedDocumentIdentity = appliedDocumentIdentity;
        scrollToFraction(retainedFraction);
        if (pendingAnchor !== "") { var anchor = pendingAnchor; pendingAnchor = ""; jumpToAnchor(anchor); }
    }
    function refresh() {
        if (!visible) return;
        retainedFraction = appliedDocumentIdentity === documentIdentity ? viewportFraction() : 0;
        ++refreshRevision;
        webRestorationKey = "";
        requestToken = documentIdentity + ":" + refreshRevision;
        previewRefreshInFlight = true;
        refreshDeadline.restart();
        var result = renderer.requestPublishingPreview(publishingMode === "pdf" ? "pdf" : "html", root, requestToken);
        pendingRequest = result.pending ? result.requestId : -1;
        if (result.pending) return;
        acceptOutput(result);
    }
    function acceptOutput(result) {
        if (result.requestIdentity !== undefined && result.requestIdentity !== requestToken) return;
        if (result.documentIdentity !== undefined && String(result.documentIdentity) !== documentIdentity) return;
        if (result.format !== undefined && result.format !== (publishingMode === "pdf" ? "pdf" : "html")) return;
        if (!result.ok) { failRefresh(result.error || "Could not load the publishing preview."); return; }
        errorText = "";
        warningText = result.warning || "";
        // Cache hits still have to prove the actual displayed source is current.
        if (String(result.url) === String(outputUrl)
                && (publishingMode === "web" ? String(web.url) === String(result.url)
                    : String(pdf.source) === String(result.url))) {
            appliedRevision = refreshRevision;
            appliedDocumentIdentity = documentIdentity;
            if (publishingMode === "web" && !web.loading) finishRefresh();
            else if (publishingMode === "pdf" && pdf.status === PdfDocument.Ready) {
                pdfPagesReady = true;
                finishRefresh();
            }
            return;
        }
        // Loading must never wait for a screenshot or compositor callback.
        applyOutput(result);
    }
    function applyOutput(result) {
        appliedRevision = refreshRevision;
        appliedDocumentIdentity = documentIdentity;
        pdfAnchors = result.anchors || ({});
        outputUrl = result.url;
        if (publishingMode === "web") {
            var sameUrl = String(webOutputUrl) === String(result.url);
            webOutputUrl = result.url;
            // Re-assigning an unchanged URL navigates nowhere, so no load
            // callback would ever complete this refresh. Force the load.
            if (sameUrl) {
                if (String(web.url) === String(result.url)) web.reload();
                else web.url = result.url;
            }
        } else {
            pdfPagesReady = false;
            // Destroy old page delegates before changing their document.
            var revision = refreshRevision, identity = documentIdentity;
            Qt.callLater(function() {
                if (revision !== root.refreshRevision || identity !== root.documentIdentity || root.publishingMode !== "pdf") return;
                if (String(pdf.source) === String(result.url) && pdf.status === PdfDocument.Ready) {
                    // Mode changes clear the view, but may reuse an already-loaded
                    // PDF. Assigning the same source emits no status transition.
                    root.pdfPagesReady = true;
                    root.finishRefresh();
                } else pdf.source = result.url;
            });
        }
    }
    function reload() {
        ++refreshRevision;
        cancelRequest();
        if (appliedDocumentIdentity !== documentIdentity) clearOutput();
        refreshTimer.restart();
    }
    onWebOutputUrlChanged: web.url = webOutputUrl
    onDocumentIdentityChanged: { clearOutput(); reload(); }
    onMarkdownChanged: reload()
    onDocumentBaseUrlChanged: reload()
    onPublishingModeChanged: { clearOutput(); reload(); }
    onVisibleChanged: {
        if (visible) reload();
        else { ++refreshRevision; refreshTimer.stop(); cancelRequest(); }
    }
    Component.onCompleted: reload()
    Timer { id: userScrollExpiry; interval: 350; onTriggered: root.webUserScrolling = false }
    Timer { id: refreshTimer; interval: 220; onTriggered: root.refresh() }
    // Longer than the renderer's own 30 s limit, so its precise error wins.
    Timer { id: refreshDeadline; objectName: "publishingRefreshDeadline"; interval: 35000; onTriggered: root.failRefresh("Preview loading timed out. Use Reload Preview to try again.") }
    Connections {
        target: root.renderer
        function onOutputStyleChanged() { root.reload(); }
        function onOutputPageLayoutChanged() { root.reload(); }
        function onOutputCssChanged() { root.reload(); }
        // Catalog changes only update menus; CSS that reaches the output re-renders.
        function onPublishingCssChanged() { root.reload(); }
        function onPublishingPreviewReady(requestId, result) {
            if (requestId !== root.pendingRequest || root.publishingMode !== "pdf") return;
            root.pendingRequest = -1;
            root.acceptOutput(result);
        }
    }
    PdfDocument {
        id: pdf
        objectName: "actualPdfDocument"
        onStatusChanged: function(status) {
            if (root.appliedRevision !== root.refreshRevision || root.publishingMode !== "pdf" || String(source) !== String(root.outputUrl)) return;
            if (status === PdfDocument.Ready) {
                root.pdfPagesReady = true;
                var revision = root.refreshRevision, loadedUrl = String(source);
                Qt.callLater(function() {
                    if (revision === root.refreshRevision && loadedUrl === String(root.outputUrl)) root.finishRefresh();
                });
            }
            else if (status === PdfDocument.Error) root.failRefresh("Could not read the publishing PDF.");
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
                        objectName: "publishingPdfPage_" + paper.index
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
        visible: root.publishingMode === "web" && String(root.webOutputUrl) !== ""
        zoomFactor: root.zoom
        backgroundColor: "white"
        settings.javascriptEnabled: false
        // Every asset is embedded as a data: URL; the page needs no file access.
        settings.localContentCanAccessFileUrls: false
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
            if (info.status === WebEngineView.LoadSucceededStatus) {
                // Complete the refresh directly; the viewport script only
                // restores metrics and may legitimately return nothing.
                root.finishRefresh();
                root.webViewportCommand("", false);
            }
            else if (info.status === WebEngineView.LoadFailedStatus) root.failRefresh(info.errorString);
        }
        onRenderProcessTerminated: function(terminationStatus, exitCode) {
            root.failRefresh("The web preview stopped unexpectedly. Reloading…");
            root.reload();
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
    Label {
        objectName: "publishingLoadingNotice"
        anchors.centerIn: parent
        visible: root.viewportRefreshPending && String(root.outputUrl) === "" && root.errorText === ""
        text: "Loading preview…"
        color: root.renderer.palette.text
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
