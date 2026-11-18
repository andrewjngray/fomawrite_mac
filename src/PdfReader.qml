pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import Fomawrite 1.0

// The PDF in the preview pane's slot (Cycles 160–161): a toolbar, the pages in
// one continuous scroll, thumbnails, find, the Inspector, text selection and
// links, and the password and error states. Geometry from the session is in
// page view points; a page is drawn at pageScale logical pixels per point.
Rectangle {
    id: root
    objectName: "pdfReader"
    property var session: null
    property int bottomInset: 0
    property bool darkMode: false
    readonly property int pageGap: 12
    readonly property int sideMargin: 16
    readonly property string pdfState: session ? session.state : "none"
    readonly property int pageCount: session && pdfState === "ready" ? session.pageCount : 0
    readonly property bool ready: pageCount > 0
    readonly property int revision: session ? session.revision : 0
    readonly property size largestPage: session && revision >= 0 && session.largestPageSize.width > 0 ? session.largestPageSize : Qt.size(612, 792)
    signal openUrlRequested(url link)
    color: darkMode ? "#2a2c30" : "#e4e4e2"

    // Zoom. 100% ("actual size") draws one PDF point as 4/3 logical pixels (96 dpi).
    readonly property real pointScale: 4 / 3
    readonly property real minimumZoom: 0.25
    readonly property real maximumZoom: 5
    readonly property var zoomStops: [0.25, 0.33, 0.5, 0.67, 0.75, 0.9, 1, 1.1, 1.25, 1.5, 1.75, 2, 2.5, 3, 4, 5]
    property string zoomMode: "fit" // "fit" (the widest page fills the pane) or "fixed"
    property real fixedZoom: 1
    readonly property real fitZoom: Math.max(minimumZoom, Math.min(maximumZoom,
        (pages.width - 2 * sideMargin) / Math.max(1, largestPage.width * pointScale)))
    readonly property real zoom: zoomMode === "fit" ? fitZoom : fixedZoom
    readonly property real pageScale: zoom * pointScale
    readonly property int zoomPercent: Math.round(zoom * 100)

    // The page with most of its area in view (0-based).
    property int currentPage: 0
    readonly property string currentLabel: session && ready && revision >= 0 ? session.pageLabel(currentPage) : ""

    // The current text selection (one page); Phase 2 marks it.
    property int selectionPage: -1
    property int selectionStart: -1
    property int selectionLength: 0
    property string selectionText: ""
    property var selectionRects: []

    // Find.
    property bool findOpen: false
    property int findIndex: -1
    property int findAnchorPage: 0
    readonly property var findResults: session ? session.searchResults : []
    readonly property bool findSearching: session ? session.searching : false
    readonly property var findByPage: {
        var byPage = ({});
        var results = findResults;
        for (var i = 0; i < results.length; ++i) {
            var result = results[i];
            var list = byPage[result.page] || (byPage[result.page] = []);
            for (var r = 0; r < result.rects.length; ++r) list.push({ rect: result.rects[r], result: i });
        }
        return byPage;
    }
    readonly property bool thumbnailsShown: session ? session.thumbnailsVisible : false

    function clampY(y) { return Math.max(pages.originY, Math.min(pages.originY + Math.max(0, pages.contentHeight - pages.height), y)); }
    function clampX(x) { return Math.max(0, Math.min(Math.max(0, pages.contentWidth - pages.width), x)); }

    // Where a point of the viewport lies in the document: page and fractions of it.
    function pageAtContentY(y) {
        var index = pages.indexAt(1, y);
        if (index < 0) index = pages.indexAt(1, y + pageGap + 1);
        if (index < 0) index = pages.indexAt(1, y - pageGap - 1);
        if (index < 0) index = y <= pages.originY + pageGap ? 0 : pageCount - 1;
        return index;
    }
    function anchorAt(viewX, viewY) {
        if (!ready) return null;
        var page = pageAtContentY(pages.contentY + viewY);
        var item = pages.itemAtIndex(page);
        if (!item) return null;
        return { page: page, viewX: viewX, viewY: viewY,
                 fy: (pages.contentY + viewY - item.y) / Math.max(1, item.height),
                 fx: (pages.contentX + viewX - item.pageX) / Math.max(1, item.pageWidth) };
    }
    property var viewAnchor: null
    property bool restoringView: false
    function restoreAnchor(anchor) {
        restoringView = false;
        if (!anchor || !ready) return;
        restoringView = true;
        pages.forceLayout();
        pages.positionViewAtIndex(anchor.page, ListView.Beginning);
        var item = pages.itemAtIndex(anchor.page);
        if (item) {
            pages.contentY = clampY(item.y + anchor.fy * item.height - anchor.viewY);
            pages.contentX = clampX(item.pageX + anchor.fx * item.pageWidth - anchor.viewX);
        }
        restoringView = false;
        noteViewChanged();
        rememberSoon();
    }
    function noteViewChanged() {
        if (restoringView || !ready) return;
        viewAnchor = anchorAt(0, 0);
        updateCurrentPage();
    }
    onPageScaleChanged: {
        // Whatever changed the scale, the same place in the document stays in
        // view. Changes before the restore runs keep the first place.
        var anchor = pendingAnchor || queuedAnchor || viewAnchor;
        pendingAnchor = null;
        if (!anchor || !ready) return;
        queuedAnchor = anchor;
        restoringView = true;
        restoreTimer.restart();
    }
    property var pendingAnchor: null
    property var queuedAnchor: null
    function restoreQueuedAnchor() {
        var anchor = queuedAnchor;
        queuedAnchor = null;
        restoreAnchor(anchor);
    }
    function setZoom(mode, value, viewX, viewY) {
        var bounded = Math.max(minimumZoom, Math.min(maximumZoom, value));
        if (mode === zoomMode && (mode === "fit" || Math.abs(bounded - fixedZoom) < 0.0001)) return;
        pendingAnchor = queuedAnchor || anchorAt(viewX === undefined ? pages.width / 2 : viewX, viewY === undefined ? pages.height / 2 : viewY);
        if (mode === "fixed") fixedZoom = bounded;
        zoomMode = mode;
        if (!restoringView) pendingAnchor = null; // the scale did not change
        rememberSoon();
    }
    function zoomIn() {
        for (var i = 0; i < zoomStops.length; ++i) if (zoomStops[i] > zoom + 0.001) { setZoom("fixed", zoomStops[i]); return; }
    }
    function zoomOut() {
        for (var i = zoomStops.length - 1; i >= 0; --i) if (zoomStops[i] < zoom - 0.001) { setZoom("fixed", zoomStops[i]); return; }
    }
    function actualSize() { setZoom("fixed", 1); }
    function fitWidth() { setZoom("fit", fitZoom); }
    function zoomAround(value, viewX, viewY) { setZoom("fixed", value, viewX, viewY); }

    function updateCurrentPage() {
        if (!ready) return;
        var top = pages.contentY, bottom = pages.contentY + pages.height;
        var best = -1, bestArea = -1;
        for (var i = pageAtContentY(top); i >= 0 && i < pageCount; ++i) {
            var item = pages.itemAtIndex(i);
            if (!item || item.y > bottom) break;
            var area = Math.min(bottom, item.y + item.height) - Math.max(top, item.y);
            if (area > bestArea + 0.5) { best = i; bestArea = area; }
        }
        if (best >= 0 && best !== currentPage) { currentPage = best; rememberSoon(); }
    }
    function goToPage(page) {
        if (!ready) return;
        page = Math.max(0, Math.min(pageCount - 1, page));
        pages.positionViewAtIndex(page, ListView.Beginning);
        var item = pages.itemAtIndex(page);
        if (item) pages.contentY = clampY(item.y - pageGap);
        currentPage = page;
        noteViewChanged();
    }
    // A page number ("3") or a page label ("iii"); labels win.
    function goToPageText(text) {
        var wanted = String(text).trim();
        if (!ready || wanted === "") return false;
        for (var i = 0; i < pageCount; ++i)
            if (session.pageLabel(i).toLowerCase() === wanted.toLowerCase()) { goToPage(i); return true; }
        if (/^\d+$/.test(wanted) && Number(wanted) >= 1 && Number(wanted) <= pageCount) { goToPage(Number(wanted) - 1); return true; }
        return false;
    }
    function showRequestedPage() {
        if (!session || !ready) return;
        goToPage(session.requestedPage);
    }
    function scrollBy(dx, dy) {
        pages.cancelFlick();
        pages.contentX = clampX(pages.contentX + dx);
        pages.contentY = clampY(pages.contentY + dy);
    }
    function focusPages() { pages.forceActiveFocus(Qt.MouseFocusReason); }
    function focusPageField() { toolbar.focusPageField(); }

    // Selection.
    function setSelection(page, selection) {
        if (!selection || selection.text === undefined || selection.length <= 0) { clearSelection(); return; }
        selectionPage = page;
        selectionStart = selection.start;
        selectionLength = selection.length;
        selectionText = selection.text;
        selectionRects = selection.rects;
    }
    function clearSelection() {
        selectionPage = -1;
        selectionStart = -1;
        selectionLength = 0;
        selectionText = "";
        selectionRects = [];
    }
    function copySelection() {
        if (selectionText === "" || !session) return false;
        session.copyText(selectionText);
        return true;
    }

    // Links: rects in points; page -1 for a web address.
    function linkAt(links, point) {
        for (var i = 0; i < links.length; ++i) {
            var rects = links[i].rects;
            for (var r = 0; r < rects.length; ++r) {
                var rect = rects[r];
                if (point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y && point.y <= rect.y + rect.height)
                    return links[i];
            }
        }
        return null;
    }
    function followLink(link) {
        if (link.page >= 0) goToPage(link.page);
        else if (String(link.url) !== "") linkConfirm.ask(link.url);
    }

    // Find.
    function openFind(useSelection) {
        findOpen = true;
        if (useSelection && selectionText !== "") findBar.query = selectionText;
        findBar.focusQuery();
        if (findBar.query !== "" && session && session.searchQuery !== findBar.query) runFind();
    }
    function closeFind() {
        if (!findOpen) return;
        findOpen = false;
        findIndex = -1;
        if (session) session.clearSearch();
        focusPages();
    }
    function runFind() {
        findIndex = -1;
        findAnchorPage = currentPage;
        if (session) session.startSearch(findBar.query);
    }
    function chooseFindIndex() {
        if (findIndex >= 0 || findResults.length === 0) return;
        for (var i = 0; i < findResults.length; ++i)
            if (findResults[i].page >= findAnchorPage) { findIndex = i; showFindResult(); return; }
        if (!findSearching) { findIndex = 0; showFindResult(); }
    }
    onFindResultsChanged: { if (findIndex >= findResults.length) findIndex = -1; chooseFindIndex(); }
    onFindSearchingChanged: chooseFindIndex()
    function moveFind(direction) {
        if (findResults.length === 0) return;
        findIndex = findIndex < 0 ? (direction > 0 ? 0 : findResults.length - 1)
            : (findIndex + direction + findResults.length) % findResults.length;
        showFindResult();
    }
    function showFindResult() {
        var result = findResults[findIndex];
        if (!result) return;
        var bounds = null;
        for (var r = 0; r < result.rects.length; ++r) {
            var rect = result.rects[r];
            bounds = bounds ? Qt.rect(Math.min(bounds.x, rect.x), Math.min(bounds.y, rect.y),
                                      Math.max(bounds.x + bounds.width, rect.x + rect.width) - Math.min(bounds.x, rect.x),
                                      Math.max(bounds.y + bounds.height, rect.y + rect.height) - Math.min(bounds.y, rect.y))
                            : Qt.rect(rect.x, rect.y, rect.width, rect.height);
        }
        pages.positionViewAtIndex(result.page, ListView.Contain);
        var item = pages.itemAtIndex(result.page);
        if (item && bounds) {
            var top = item.y + bounds.y * pageScale, bottom = item.y + (bounds.y + bounds.height) * pageScale;
            if (top < pages.contentY || bottom > pages.contentY + pages.height)
                pages.contentY = clampY((top + bottom) / 2 - pages.height / 2);
            var left = item.pageX + bounds.x * pageScale, right = item.pageX + (bounds.x + bounds.width) * pageScale;
            if (left < pages.contentX || right > pages.contentX + pages.width)
                pages.contentX = clampX((left + right) / 2 - pages.width / 2);
        }
        noteViewChanged();
    }

    function toggleThumbnails() { if (session) session.thumbnailsVisible = !session.thumbnailsVisible; }
    function showInspector() { if (ready) inspector.showFrom(toolbar.inspectorAnchor); }

    // Last page and zoom per PDF (PdfSession::rememberView), written shortly
    // after they change and at once when another PDF replaces this one.
    property url shownUrl
    property var lastView: null
    function rememberSoon() {
        if (!ready || restoringView || !session || String(shownUrl) === "") return;
        lastView = { url: shownUrl, page: currentPage, mode: zoomMode, zoom: fixedZoom };
        rememberTimer.restart();
    }
    function rememberNow() {
        rememberTimer.stop();
        if (lastView && session) session.rememberView(lastView.url, lastView.page, lastView.mode, lastView.zoom);
    }
    Timer { id: rememberTimer; interval: 400; onTriggered: root.rememberNow() }
    // Deferred to the next turn of the event loop; timers die with the pane
    // (Qt.callLater could outlive it when PDF mode ends).
    Timer { id: restoreTimer; interval: 0; onTriggered: root.restoreQueuedAnchor() }
    Timer { id: adoptTimer; interval: 0; onTriggered: root.adoptDocument() }
    Timer { id: requestTimer; interval: 0; onTriggered: root.showRequestedPage() }
    Timer { id: findTimer; interval: 0; onTriggered: root.runFind() }
    function adoptDocument() {
        if (!session || !ready || String(session.url) === String(shownUrl)) return;
        shownUrl = session.url;
        lastView = null;
        clearSelection();
        viewAnchor = null;
        pendingAnchor = null;
        queuedAnchor = null;
        var view = session.rememberedView(shownUrl);
        if (view && view.mode === "fixed") { fixedZoom = Math.max(minimumZoom, Math.min(maximumZoom, Number(view.zoom) || 1)); zoomMode = "fixed"; }
        else zoomMode = "fit";
        requestTimer.restart();
    }
    Connections {
        target: root.session
        function onUrlChanged() { root.rememberNow(); }
        function onRequestedPageChanged() { root.showRequestedPage(); }
        function onRevisionChanged() {
            root.clearSelection();
            adoptTimer.restart();
            if (root.findOpen && root.findBar.query !== "") findTimer.restart();
        }
    }
    readonly property alias findBar: findBar
    Component.onDestruction: rememberNow()

    Keys.onPressed: function(event) {
        var command = (event.modifiers & Qt.ControlModifier) !== 0;
        var shift = (event.modifiers & Qt.ShiftModifier) !== 0;
        if (event.matches(StandardKey.Copy)) { if (copySelection()) event.accepted = true; return; }
        if (command && (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal)) { zoomIn(); event.accepted = true; return; }
        if (command && event.key === Qt.Key_Minus) { zoomOut(); event.accepted = true; return; }
        if (command && event.key === Qt.Key_0) { actualSize(); event.accepted = true; return; }
        if (command || !ready) return;
        var pageStep = Math.max(40, pages.height - 40);
        switch (event.key) {
        case Qt.Key_PageDown: scrollBy(0, pageStep); break;
        case Qt.Key_PageUp: scrollBy(0, -pageStep); break;
        case Qt.Key_Space: scrollBy(0, shift ? -pageStep : pageStep); break;
        case Qt.Key_Down: scrollBy(0, 40); break;
        case Qt.Key_Up: scrollBy(0, -40); break;
        case Qt.Key_Right: scrollBy(40, 0); break;
        case Qt.Key_Left: scrollBy(-40, 0); break;
        case Qt.Key_Home: pages.contentY = clampY(pages.originY); break;
        case Qt.Key_End: pages.contentY = clampY(pages.originY + pages.contentHeight); break;
        case Qt.Key_Escape: if (selectionText === "") return; clearSelection(); break;
        default: return;
        }
        event.accepted = true;
    }

    PdfToolbar {
        id: toolbar
        reader: root
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }

    PdfThumbnails {
        id: thumbnails
        reader: root
        visible: root.thumbnailsShown && root.ready
        anchors.top: toolbar.bottom
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.bottomInset
        anchors.left: parent.left
        width: visible ? 140 : 0
    }

    ListView {
        id: pages
        objectName: "pdfPageList"
        anchors.top: toolbar.bottom
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.bottomInset
        anchors.left: thumbnails.right
        anchors.right: parent.right
        clip: true
        visible: root.ready
        activeFocusOnTab: true
        keyNavigationEnabled: false
        model: root.pageCount
        spacing: root.pageGap
        topMargin: root.pageGap
        bottomMargin: root.pageGap
        cacheBuffer: Math.max(0, height)
        contentWidth: Math.max(width, root.largestPage.width * root.pageScale + 2 * root.sideMargin)
        flickableDirection: contentWidth > width + 0.5 ? Flickable.HorizontalAndVerticalFlick : Flickable.VerticalFlick
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}
        ScrollBar.horizontal: ScrollBar { policy: pages.contentWidth > pages.width + 0.5 ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff }
        onContentYChanged: root.noteViewChanged()
        onContentXChanged: root.noteViewChanged()
        onHeightChanged: root.noteViewChanged()
        onCountChanged: if (count > 0) adoptTimer.restart()
        onActiveFocusChanged: if (!activeFocus) root.rememberSoon()

        // Cmd/Ctrl + wheel and trackpad pinch zoom around the pointer.
        WheelHandler {
            id: zoomWheel
            objectName: "pdfZoomWheel"
            target: null
            acceptedModifiers: Qt.ControlModifier
            onWheel: function(event) {
                var steps = (event.angleDelta.y || event.pixelDelta.y / 8) / 120;
                var at = zoomWheel.parent.mapToItem(pages, zoomWheel.point.position.x, zoomWheel.point.position.y);
                if (steps !== 0) root.zoomAround(root.zoom * Math.pow(1.15, steps), at.x, at.y);
            }
        }
        PinchHandler {
            id: pinch
            target: null
            property real startZoom: 1
            onActiveChanged: if (active) startZoom = root.zoom
            onActiveScaleChanged: {
                if (!active) return;
                var at = pinch.parent.mapToItem(pages, centroid.position.x, centroid.position.y);
                root.zoomAround(startZoom * activeScale, at.x, at.y);
            }
        }

        delegate: Item {
            id: pageSlot
            required property int index
            // Re-read after a reload: the revision changes when the file does.
            readonly property size points: root.session && root.revision >= 0 ? root.session.pageSize(index) : Qt.size(612, 792)
            readonly property var links: root.session && root.revision >= 0 ? root.session.links(index) : []
            readonly property real pageWidth: Math.max(1, points.width * root.pageScale)
            readonly property real pageX: Math.max(root.sideMargin, (width - pageWidth) / 2)
            width: pages.contentWidth
            height: Math.max(1, points.height * root.pageScale)

            Rectangle {
                x: pageSlot.pageX - 1; y: -1
                width: pageSlot.pageWidth + 2; height: pageSlot.height + 2
                color: root.darkMode ? "#18191b" : "#c6c6c3"
            }
            PdfPage {
                id: page
                objectName: "pdfPage"
                session: root.session
                page: pageSlot.index
                x: pageSlot.pageX
                width: pageSlot.pageWidth
                height: pageSlot.height
                wanted: pageSlot.y + pageSlot.height > pages.contentY - pages.height / 4
                    && pageSlot.y < pages.contentY + pages.height * 1.25
            }
            Item {
                x: page.x; width: page.width; height: page.height
                Repeater {
                    model: root.findByPage[pageSlot.index] || []
                    Rectangle {
                        required property var modelData
                        readonly property bool current: modelData.result === root.findIndex
                        objectName: current ? "pdfFindCurrent" : "pdfFindTint"
                        x: modelData.rect.x * root.pageScale - 1
                        y: modelData.rect.y * root.pageScale - 2
                        width: modelData.rect.width * root.pageScale + 2
                        height: modelData.rect.height * root.pageScale + 4
                        radius: 2
                        color: current ? "#80ff9f0a" : "#59ffd84d"
                        border.width: current ? 1 : 0
                        border.color: "#e08600"
                    }
                }
                Repeater {
                    model: root.selectionPage === pageSlot.index ? root.selectionRects : []
                    Rectangle {
                        required property var modelData
                        objectName: "pdfSelectionRect"
                        x: modelData.x * root.pageScale
                        y: modelData.y * root.pageScale - 2
                        width: modelData.width * root.pageScale
                        height: modelData.height * root.pageScale + 4
                        color: backend.palette.focus
                        opacity: 0.38
                    }
                }
            }
            MouseArea {
                id: pageMouse
                objectName: "pdfPageMouse"
                x: page.x; width: page.width; height: page.height
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true
                preventStealing: true
                property point pressPoint
                property bool dragging: false
                property bool wordClick: false
                property var hoverLink: null
                cursorShape: hoverLink ? Qt.PointingHandCursor : Qt.IBeamCursor
                function toPoints(mouse) { return Qt.point(mouse.x / root.pageScale, mouse.y / root.pageScale); }
                onPressed: function(mouse) {
                    root.focusPages();
                    pressPoint = toPoints(mouse);
                    dragging = false;
                    wordClick = false;
                }
                onPositionChanged: function(mouse) {
                    var point = toPoints(mouse);
                    if (!pressed) { hoverLink = root.linkAt(pageSlot.links, point); return; }
                    if (!dragging && Math.hypot(point.x - pressPoint.x, point.y - pressPoint.y) * root.pageScale < 3) return;
                    dragging = true;
                    root.setSelection(pageSlot.index, root.session.selectionBetween(pageSlot.index, pressPoint, point));
                }
                onReleased: function(mouse) {
                    if (dragging || wordClick) return;
                    var link = root.linkAt(pageSlot.links, toPoints(mouse));
                    if (link) { root.clearSelection(); root.followLink(link); }
                    else root.clearSelection();
                }
                onDoubleClicked: function(mouse) {
                    wordClick = true;
                    root.setSelection(pageSlot.index, root.session.wordAt(pageSlot.index, toPoints(mouse)));
                }
                onExited: hoverLink = null
            }
        }
    }

    PdfFindBar {
        id: findBar
        reader: root
        visible: root.findOpen
        anchors.top: toolbar.bottom
        anchors.topMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 16
        width: Math.min(420, root.width - 32)
        z: 5
    }

    // Password and error states replace the pages.
    Column {
        objectName: "pdfPasswordPrompt"
        visible: root.pdfState === "password"
        anchors.centerIn: parent
        width: Math.min(root.width - 48, 320)
        spacing: 10
        property bool failed: false
        id: passwordPrompt
        onVisibleChanged: { failed = false; passwordField.text = ""; if (visible) passwordField.forceActiveFocus(); }
        function unlock() {
            if (!root.session || passwordField.text === "") return;
            failed = !root.session.unlock(passwordField.text);
            if (!failed) passwordField.text = "";
            else passwordField.selectAll();
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            color: backend.palette.text
            font.pixelSize: 13
            text: root.session ? root.session.fileName + " is protected by a password." : ""
        }
        TextField {
            id: passwordField
            objectName: "pdfPasswordField"
            width: parent.width
            echoMode: TextInput.Password
            placeholderText: ""
            font.pixelSize: 13
            color: backend.palette.text
            leftPadding: 12; rightPadding: 12
            Accessible.name: "PDF password"
            background: Rectangle {
                radius: 8
                color: backend.palette.field
                border.width: passwordField.activeFocus ? 2 : 1
                border.color: passwordField.activeFocus ? backend.palette.focus : backend.palette.border
            }
            onAccepted: passwordPrompt.unlock()
        }
        ToolbarButton {
            objectName: "pdfUnlockButton"
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Unlock"
            hint: "Unlock the PDF"
            onClicked: passwordPrompt.unlock()
        }
        Label {
            objectName: "pdfPasswordError"
            width: parent.width
            visible: passwordPrompt.failed
            horizontalAlignment: Text.AlignHCenter
            color: root.darkMode ? "#ff8a80" : "#c5221f"
            font.pixelSize: 12
            text: "The password is not correct"
        }
    }
    Label {
        objectName: "pdfErrorText"
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 420)
        visible: root.pdfState === "error"
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        textFormat: Text.PlainText
        color: root.darkMode ? "#d8d8d8" : "#444444"
        text: root.session ? root.session.errorText : ""
    }

    PdfInspector {
        id: inspector
        reader: root
    }

    // A web address in the PDF opens only after the reader agrees.
    Popup {
        id: linkConfirm
        objectName: "pdfLinkConfirm"
        property url link
        function ask(url) {
            link = url;
            x = Math.max(8, (root.width - width) / 2);
            y = Math.max(8, (root.height - height) / 3);
            open();
            openButton.forceActiveFocus();
        }
        width: Math.min(380, root.width - 16)
        modal: true
        padding: 16
        popupType: Popup.Item
        background: Rectangle {
            radius: 10
            color: backend.palette.popover || backend.palette.panel
            border.color: backend.palette.border
        }
        onClosed: root.focusPages()
        contentItem: Column {
            spacing: 10
            Label {
                width: parent.width
                text: "Open this link in your browser?"
                font.pixelSize: 13
                font.weight: Font.Medium
                color: backend.palette.text
            }
            Label {
                objectName: "pdfLinkUrl"
                width: parent.width
                text: String(linkConfirm.link)
                textFormat: Text.PlainText
                wrapMode: Text.WrapAnywhere
                maximumLineCount: 4
                elide: Text.ElideRight
                font.pixelSize: 12
                color: backend.palette.muted
            }
            Row {
                anchors.right: parent.right
                spacing: 8
                ToolbarButton { objectName: "pdfLinkCancel"; text: "Cancel"; onClicked: linkConfirm.close() }
                ToolbarButton {
                    id: openButton
                    objectName: "pdfLinkOpen"
                    text: "Open"
                    checked: true
                    Keys.onReturnPressed: clicked()
                    onClicked: { var link = linkConfirm.link; linkConfirm.close(); root.openUrlRequested(link); }
                }
            }
        }
    }
}
