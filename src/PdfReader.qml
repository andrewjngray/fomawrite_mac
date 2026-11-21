pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs
import Fomawrite 1.0

// The PDF in the preview pane's slot (Cycles 160–161): a toolbar, the pages in
// one continuous scroll, thumbnails, find, the Inspector, text selection and
// links, and the password and error states. Geometry from the session is in
// page view points; a page is drawn at pageScale logical pixels per point.
Rectangle {
    id: root
    objectName: "pdfReader"
    property var session: null
    // backend.pdfMarks: the marks in the notes and the highlighter's settings (Cycle 163).
    property var marks: null
    property string noteAuthor: ""
    property int bottomInset: 0
    property bool darkMode: false
    readonly property int pageGap: 12
    readonly property int sideMargin: 16
    readonly property string pdfState: session ? session.state : "none"
    readonly property int pageCount: session && pdfState === "ready" ? session.pageCount : 0
    readonly property bool ready: pageCount > 0
    readonly property bool enhancedPdfToolbar: (Qt.platform.os === "osx" || Qt.platform.os === "windows")
    readonly property int revision: session ? session.revision : 0
    readonly property size largestPage: session && revision >= 0 && session.largestPageSize.width > 0 ? session.largestPageSize : Qt.size(612, 792)
    signal openUrlRequested(url link)
    // Note pins and navigation (Cycle 164). The window shows the notes before a
    // note is placed (editorNeeded), then puts the caret where the note's text
    // goes (noteEditRequested); a click on a mark or pin asks the window to
    // show its item in the notes (revealInNotesRequested, an items index).
    signal editorNeeded()
    signal noteEditRequested(int caret)
    signal revealInNotesRequested(int index)
    color: darkMode ? "#2a2c30" : "#e4e4e2"

    // Zoom. 100% ("actual size") draws one PDF point as 4/3 logical pixels (96 dpi).
    readonly property real pointScale: 4 / 3
    readonly property real minimumZoom: 0.25
    readonly property real maximumZoom: 5
    readonly property var zoomStops: [0.25, 0.33, 0.5, 0.67, 0.75, 0.9, 1, 1.1, 1.25, 1.5, 1.75, 2, 2.5, 3, 4, 5]
    property string zoomMode: "fit" // "fit" (width), "fitPage", or "fixed"
    property real fixedZoom: 1
    readonly property real fitZoom: Math.max(minimumZoom, Math.min(maximumZoom,
        (pages.width - 2 * sideMargin) / Math.max(1, largestPage.width * pointScale)))
    // One scale fits all page sizes without zoom jumps while scrolling.
    readonly property real wholePageZoom: Math.max(0.001, Math.min(maximumZoom,
        (pages.width - 2 * sideMargin) / Math.max(1, largestPage.width * pointScale),
        (pages.height - 2 * pageGap) / Math.max(1, largestPage.height * pointScale)))
    readonly property real zoom: zoomMode === "fit" ? fitZoom : zoomMode === "fitPage" ? wholePageZoom : fixedZoom
    readonly property real pageScale: zoom * pointScale
    readonly property int zoomPercent: Math.round(zoom * 100)

    // The page with most of its area in view (0-based).
    property int currentPage: 0
    readonly property string currentLabel: session && ready && revision >= 0 ? session.pageLabel(currentPage) : ""

    // The thumbnail selection is independent of the page visible in the reader.
    property var selectedPages: []
    property int selectionAnchorPage: -1
    readonly property var pageTargets: selectedPages.length ? selectedPages : (ready ? [Math.min(currentPage, pageCount - 1)] : [])
    readonly property bool canRemoveSelectedPages: ready && pageTargets.length < pageCount
    property var signedEdit: null

    function selectThumbnail(page, modifiers) {
        if ((modifiers & Qt.ShiftModifier) && selectionAnchorPage >= 0) {
            var range = [];
            for (var i = Math.min(page, selectionAnchorPage); i <= Math.max(page, selectionAnchorPage); ++i)
                range.push(i);
            selectedPages = range;
        } else if (modifiers & (Qt.ControlModifier | Qt.MetaModifier)) {
            var selected = selectedPages.length ? selectedPages.slice() : [currentPage];
            var index = selected.indexOf(page);
            if (index < 0) selected.push(page);
            else selected.splice(index, 1);
            selectedPages = selected.sort(function(a, b) { return a - b; });
            selectionAnchorPage = page;
        } else {
            selectedPages = [page];
            selectionAnchorPage = page;
        }
        goToPage(page);
    }

    function selectPageRange(first, count) {
        if (!ready || count <= 0) return;
        var selection = [];
        for (var i = first; i < first + count && i < pageCount; ++i) selection.push(i);
        selectedPages = selection;
        selectionAnchorPage = first;
        goToPage(first);
    }

    function editPages(operation) {
        if (!ready) return;
        if (operation()) return;
        if (session.pageEditNeedsSignatureConsent()) {
            signedEdit = operation;
            signedPdfDialog.open();
        } else showPageEditError();
    }

    function showPageEditError() {
        if (!session || !session.pageEditError) return;
        pdfPageNotice.text = session.pageEditError;
        pdfPageNotice.open();
    }

    function rotateSelectedPages(turn) {
        var targets = pageTargets.slice();
        editPages(function() {
            if (!session.rotatePages(targets, turn)) return false;
            selectedPages = targets;
            selectionAnchorPage = targets[0];
            goToPage(targets[0]);
            return true;
        });
    }

    function removeSelectedPages() {
        if (!canRemoveSelectedPages) return;
        var targets = pageTargets.slice();
        editPages(function() {
            if (!session.removePages(targets)) return false;
            selectPageRange(Math.min(targets[0], pageCount - 1), 1);
            return true;
        });
    }

    function moveSelectedPages(before) {
        var targets = pageTargets.slice();
        var start = before - targets.filter(function(page) { return page < before; }).length;
        editPages(function() {
            if (!session.movePages(targets, before)) return false;
            selectPageRange(start, targets.length);
            return true;
        });
    }

    function insertPagesAt(url, before) {
        editPages(function() {
            var count = pageCount;
            if (!session.insertPdf(url, before)) return false;
            selectPageRange(before, pageCount - count);
            return true;
        });
    }

    function choosePagesToInsert() {
        insertPdfDialog.beforePage = pageTargets[0];
        insertPdfDialog.open();
    }

    function choosePageExport() {
        exportPdfDialog.pages = pageTargets.slice();
        exportPdfDialog.open();
    }

    function dropPdfOnThumbnails(url, point) {
        var boundary = thumbnails.dropBoundary(root, point);
        if (boundary < 0) return false;
        insertPagesAt(url, boundary);
        return true;
    }

    function undoPages() {
        if (session.canUndoPageEdit && !session.undoPageEdit()) showPageEditError();
    }

    function redoPages() {
        if (session.canRedoPageEdit && !session.redoPageEdit()) showPageEditError();
    }

    Dialogs.FileDialog {
        id: insertPdfDialog
        objectName: "insertPdfDialog"
        property int beforePage: 0
        title: "Insert Pages from File"
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: ["PDF files (*.pdf)"]
        onAccepted: root.insertPagesAt(selectedFile, beforePage)
    }

    Dialogs.FileDialog {
        id: exportPdfDialog
        objectName: "exportPdfPagesDialog"
        property var pages: []
        title: "Export Selected Pages"
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: ["PDF files (*.pdf)"]
        defaultSuffix: "pdf"
        onAccepted: {
            if (!backend.exportPdfPages(selectedFile, pages)) {
                pdfPageNotice.text = backend.pdfSaveError;
                pdfPageNotice.open();
            }
        }
    }

    Dialog {
        id: signedPdfDialog
        objectName: "signedPdfPageEditDialog"
        title: "Digitally signed PDF"
        modal: true
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(400, Math.max(240, parent.width - 24))
        implicitHeight: header.implicitHeight + confirmation.implicitHeight + footer.implicitHeight
                        + topPadding + bottomPadding + 2 * spacing
        contentItem: Label {
            id: confirmation
            wrapMode: Text.WordWrap
            text: "Changing these pages will invalidate the PDF’s signature. Continue editing?"
        }
        footer: DialogButtonBox {
            Button { text: "Cancel"; DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            Button { text: "Continue"; DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole }
        }
        onAccepted: {
            var operation = root.signedEdit;
            root.signedEdit = null;
            if (!operation) return;
            root.session.signedEditsAllowed = true;
            if (!operation()) root.showPageEditError();
        }
        onRejected: root.signedEdit = null
    }

    Dialog {
        id: pdfPageNotice
        property alias text: message.text
        title: "PDF Page Changes"
        modal: true
        anchors.centerIn: parent
        width: Math.min(400, root.width - 24)
        standardButtons: Dialog.Ok
        contentItem: Label { id: message; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
    }

    // The current text selection (one page); Phase 2 marks it.
    property int selectionPage: -1
    property int selectionStart: -1
    property int selectionLength: 0
    property string selectionText: ""
    property var selectionRects: []

    // Marks (Cycle 163). The highlighter's tool, Preview's colours (the page
    // is always white, so they read the same in light and dark windows), and
    // the placed text marks by page.
    readonly property string markTool: marks ? marks.tool : "highlight"
    readonly property string markColour: marks ? marks.highlightColour : "yellow"
    readonly property var markColours: ({ yellow: "#ffd60a", green: "#34c759", blue: "#3d9df5", pink: "#ff5c9d", purple: "#a970ff", red: "#ff3b30" })
    readonly property real highlightOpacity: 0.5
    // While on, every selection made by dragging or double-click is marked at once.
    property bool markingMode: false
    readonly property var marksByPage: {
        var byPage = ({});
        var items = marks ? marks.items : [];
        for (var i = 0; i < items.length; ++i) {
            var item = items[i];
            if (!item.placed || (item.kind !== "highlight" && item.kind !== "underline" && item.kind !== "strike")) continue;
            var list = byPage[item.page] || (byPage[item.page] = []);
            for (var r = 0; r < item.rects.length; ++r)
                list.push({ index: item.index, kind: item.kind, colour: item.colour, rect: item.rects[r], direction: item.direction });
        }
        return byPage;
    }
    // Note pins: placed notes by page, drawn at a fixed size at any zoom.
    readonly property real pinSize: 22
    property bool inPageNoteEditing: (Qt.platform.os === "osx" || Qt.platform.os === "windows")
    readonly property bool noteDraftOpen: noteEditor.visible
    property var hoveredNote: null
    property point hoverAnchor
    property bool hoverReady: false
    function clearNoteHover() {
        if (!hoveredNote) return;
        hoveredNote = null;
        hoverReady = false;
        noteHoverTimer.stop();
    }
    function hoverNote(pin, anchor) {
        if (!inPageNoteEditing || !pin || noteTool || noteDraftOpen || pageMenu.visible) {
            clearNoteHover();
            return;
        }
        if (hoveredNote && hoveredNote.index === pin.index) return;
        hoveredNote = pin;
        hoverAnchor = anchor;
        hoverReady = false;
        noteHoverTimer.restart();
    }
    Timer { id: noteHoverTimer; interval: 400; onTriggered: root.hoverReady = true }
    property bool noteTool: false          // armed: the next click on a page places a note
    property string selectedNoteMarker: "" // the selected pin's marker line
    property var dragNote: null            // {index, page, x, y} while a pin is dragged
    property int flashIndex: -1            // the item flashed after the notes asked to show it
    readonly property var notesByPage: {
        var byPage = ({});
        var items = marks ? marks.items : [];
        for (var i = 0; i < items.length; ++i) {
            var item = items[i];
            if (item.kind !== "note" || !item.placed) continue;
            (byPage[item.page] || (byPage[item.page] = [])).push(item);
        }
        return byPage;
    }
    readonly property var selectedPin: {
        var items = marks ? marks.items : [];
        if (selectedNoteMarker === "") return null;
        for (var i = 0; i < items.length; ++i)
            if (items[i].kind === "note" && items[i].marker === selectedNoteMarker) return items[i];
        return null;
    }
    function armNote() {
        if (!ready || !marks || noteDraftOpen) return;
        noteTool = !noteTool;
        if (noteTool) {
            clearNoteHover();
            markingMode = false;
            clearSelection();
            selectedNoteMarker = "";
            if (enhancedPdfToolbar) focusPages();
        }
    }
    // The pin under a point of a page (points), or null.
    function pinAt(page, point) {
        var list = notesByPage[page] || [];
        var half = pinSize / 2 / Math.max(0.01, pageScale);
        for (var i = list.length - 1; i >= 0; --i)
            if (Math.abs(point.x - list[i].x) <= half && Math.abs(point.y - list[i].y) <= half) return list[i];
        return null;
    }
    function placeNoteAt(page, point, anchor) {
        noteTool = false;
        if (!marks) return false;
        if (inPageNoteEditing) {
            openNoteDraft(page, point, anchor, -1);
            return true;
        }
        editorNeeded();
        var caret = marks.placeNote(page, point.x, point.y);
        if (caret < 0) return false;
        noteEditRequested(caret);
        return true;
    }
    function openNoteDraft(page, point, anchor, index) {
        var existing = index < 0 ? null : marks.noteDraft(index);
        if (index >= 0 && (!existing || !existing.marker)) return;
        clearNoteHover();
        noteEditor.page = page;
        noteEditor.dragOffset = Qt.point(0, 0);
        noteEditor.author = existing ? existing.author : "";
        noteEditor.created = existing ? existing.created : "";
        noteEditor.point = point;
        noteEditor.anchor = anchor;
        noteEditor.noteIndex = index;
        noteEditor.marker = existing ? existing.marker : "";
        noteEditor.original = existing ? existing.body : "";
        noteEditor.errorText = "";
        noteInput.text = noteEditor.original;
        noteEditor.open();
    }

    function finishNote() {
        if (!noteDraftOpen || !marks || noteInput.text.trim() === "") return false;
        if (!marks.validNoteDraft(noteInput.text)) {
            noteEditor.errorText = "Use paragraphs here. Edit headings and code fences in Markdown.";
            return false;
        }
        var saved = noteEditor.noteIndex < 0
            ? marks.placeNote(noteEditor.page, noteEditor.point.x, noteEditor.point.y, noteInput.text, noteAuthor) >= 0
            : marks.editNote(noteEditor.noteIndex, noteEditor.marker, noteEditor.original, noteInput.text);
        if (!saved) {
            noteEditor.errorText = "This note could not be applied. Copy your text before closing.";
            return false;
        }
        noteEditor.close();
        return true;
    }

    function deleteDraftNote() {
        if (!noteDraftOpen || !marks || noteEditor.noteIndex < 0) return;
        if (!marks.deleteNote(noteEditor.noteIndex, noteEditor.marker, noteEditor.original)) {
            noteEditor.errorText = "The note changed. Close and reopen it before deleting.";
            return;
        }
        selectedNoteMarker = "";
        noteEditor.close();
    }

    function undoNoteDraft() { noteInput.undo(); }
    function redoNoteDraft() { noteInput.redo(); }
    function selectPin(pin) { selectedNoteMarker = pin ? pin.marker : ""; }
    function removeSelectedPin() {
        var pin = selectedPin;
        if (!pin || !marks) return false;
        selectedNoteMarker = "";
        return marks.removeMark(pin.index);
    }
    // Items that cannot be drawn: their quote is nowhere near its page, or the
    // page or position of a note does not exist (Cycle 164). They exist only in the notes.
    readonly property var unplacedItems: {
        var found = [];
        var items = marks ? marks.items : [];
        for (var i = 0; i < items.length; ++i) {
            var item = items[i];
            if (item.placed) continue;
            if (item.kind === "note" || item.kind === "highlight" || item.kind === "underline" || item.kind === "strike") found.push(item);
        }
        return found;
    }
    // A row of the unplaced list: show the item in the notes (the PDF alone switches to Split first).
    function revealUnplaced(index) {
        editorNeeded();
        // After the layout has settled, so the notes can scroll to the item.
        Qt.callLater(function() { root.revealInNotesRequested(index); });
    }
    function itemForMarker(marker) {
        var items = marks ? marks.items : [];
        for (var i = 0; i < items.length; ++i) if (items[i].marker === marker) return items[i];
        return null;
    }
    // The part of the page an item occupies, in points.
    function itemArea(item) {
        if (item.kind === "note") {
            var half = pinSize / 2 / Math.max(0.01, pageScale);
            return Qt.rect(item.x - half, item.y - half, 2 * half, 2 * half);
        }
        var left = Infinity, top = Infinity, right = -Infinity, bottom = -Infinity;
        for (var r = 0; r < item.rects.length; ++r) {
            var shape = markShape("highlight", item.rects[r], item.direction);
            left = Math.min(left, shape.x); top = Math.min(top, shape.y);
            right = Math.max(right, shape.x + shape.width); bottom = Math.max(bottom, shape.y + shape.height);
        }
        return right < left ? Qt.rect(0, 0, 0, 0) : Qt.rect(left, top, right - left, bottom - top);
    }
    // Scrolls just enough to show `area` (points) of a page; nothing when it is already fully in view.
    function showArea(page, area) {
        var delegate = pages.itemAtIndex(page);
        if (!delegate) {
            pages.positionViewAtIndex(page, ListView.Beginning);
            delegate = pages.itemAtIndex(page);
        }
        if (!delegate) return;
        var margin = 24;
        var top = delegate.y + area.y * pageScale, bottom = top + area.height * pageScale;
        var left = delegate.pageX + area.x * pageScale, right = left + area.width * pageScale;
        var y = pages.contentY, x = pages.contentX;
        if (top < y || bottom > y + pages.height)
            y = (top < y || bottom - top + 2 * margin > pages.height) ? top - margin : bottom + margin - pages.height;
        if (left < x || right > x + pages.width)
            x = (left < x || right - left + 2 * margin > pages.width) ? left - margin : right + margin - pages.width;
        y = clampY(y); x = clampX(x);
        if (y === pages.contentY && x === pages.contentX) return;
        pages.cancelFlick();
        pages.contentX = x;
        pages.contentY = y;
    }
    // The notes moved into the item with this marker line: show it and flash it.
    function revealItem(marker) {
        if (!ready || !marks) return false;
        var item = itemForMarker(marker);
        if (!item) { marks.refresh(); item = itemForMarker(marker); }
        if (!item || !item.placed || item.page < 0 || item.page >= pageCount) return false;
        showArea(item.page, itemArea(item));
        flashIndex = item.index;
        flashTimer.restart();
        return true;
    }
    Timer { id: flashTimer; interval: 1100; onTriggered: root.flashIndex = -1 }
    function colourValue(name) { return markColours[name] || markColours.yellow; }
    function markVerb(kind) { return kind === "note" ? "Note" : kind === "underline" ? "Underline" : kind === "strike" ? "Strikethrough" : "Highlight"; }
    // Where a mark is drawn, in page points, around one line's glyph-tight
    // box (baseline to cap height) and along the text's direction: a
    // highlight padded to the line's height as Preview draws it, an
    // underline just below the baseline, a strikethrough through the middle.
    function markShape(kind, rect, direction) {
        var vertical = direction === 90 || direction === 270;
        var h = vertical ? rect.width : rect.height;
        if (kind === "highlight") {
            var below = 0.32 * h, above = 0.22 * h, along = 0.5;
            switch (direction) {
            case 90: return Qt.rect(rect.x - below, rect.y - along, rect.width + below + above, rect.height + 2 * along);
            case 180: return Qt.rect(rect.x - along, rect.y - below, rect.width + 2 * along, rect.height + below + above);
            case 270: return Qt.rect(rect.x - above, rect.y - along, rect.width + below + above, rect.height + 2 * along);
            default: return Qt.rect(rect.x - along, rect.y - above, rect.width + 2 * along, rect.height + below + above);
            }
        }
        var thickness = Math.max(1.5 / Math.max(0.01, pageScale), 0.08 * h);
        var rise = kind === "underline" ? -0.12 * h : 0.42 * h; // from the baseline toward the cap height
        switch (direction) {
        case 90: return Qt.rect(rect.x + rise - thickness / 2, rect.y, thickness, rect.height);
        case 180: return Qt.rect(rect.x, rect.y + rise - thickness / 2, rect.width, thickness);
        case 270: return Qt.rect(rect.x + rect.width - rise - thickness / 2, rect.y, thickness, rect.height);
        default: return Qt.rect(rect.x, rect.y + rect.height - rise - thickness / 2, rect.width, thickness);
        }
    }
    // The topmost mark under a point of a page (points), or null.
    function markAt(page, point) {
        var list = marksByPage[page] || [];
        for (var i = list.length - 1; i >= 0; --i) {
            var area = markShape("highlight", list[i].rect, list[i].direction);
            if (point.x >= area.x && point.x <= area.x + area.width && point.y >= area.y && point.y <= area.y + area.height)
                return list[i];
        }
        return null;
    }
    // Marks the selection (then clears it); "added", "removed" or "".
    function markSelection(kind, colour) {
        if (!marks || selectionText === "" || selectionPage < 0) return "";
        var result = marks.mark(kind, kind === "highlight" ? (colour || markColour) : "red", selectionPage, selectionStart, selectionLength);
        clearSelection();
        return result;
    }
    function markWithTool() { return markSelection(markTool, markColour); }
    // The toolbar's pen: marks a selection, or turns marking mode on or off.
    function usePen() {
        noteTool = false;
        if (selectionText !== "") { markingMode = false; markWithTool(); return; }
        markingMode = !markingMode;
        if (enhancedPdfToolbar && markingMode) focusPages();
    }
    // The pen's menu: a colour (a highlight) or "underline" / "strike".
    function chooseTool(choice) {
        if (!marks) return;
        if (choice === "underline" || choice === "strike") marks.tool = choice;
        else { marks.highlightColour = choice; marks.tool = "highlight"; }
        if (selectionText !== "") markWithTool();
    }
    onReadyChanged: if (!ready) { markingMode = false; noteTool = false; }

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

    // The table of contents (the PDF's outline): [{title, level, page}].
    readonly property var outlineEntries: session && ready && revision >= 0 ? session.outline() : []
    readonly property bool hasOutline: outlineEntries.length > 0
    // The sidebar shows Contents only when there is an outline to show.
    readonly property string sidebarView: hasOutline && session && session.sidebarView === "contents" ? "contents" : "thumbnails"
    function showSidebarView(view) { if (session) session.sidebarView = view; }
    // The entry of the section the current page is in: the one starting
    // nearest before (or on) it, the later entry when several start there.
    readonly property int currentOutlineIndex: {
        var best = -1, bestPage = -1;
        for (var i = 0; i < outlineEntries.length; ++i) {
            var page = outlineEntries[i].page;
            if (page >= 0 && page <= currentPage && page >= bestPage) { best = i; bestPage = page; }
        }
        return best;
    }

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
            if (anchor.wholePage) {
                pages.contentY = clampY(item.y + (item.height - pages.height) / 2);
                pages.contentX = 0;
            } else {
                pages.contentY = clampY(item.y + anchor.fy * item.height - anchor.viewY);
                pages.contentX = clampX(item.pageX + anchor.fx * item.pageWidth - anchor.viewX);
            }
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
        var anchor = pendingAnchor || queuedAnchor ||
            (zoomMode === "fitPage" ? { page: currentPage, wholePage: true } : viewAnchor);
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
    function fitPage() {
        if (!ready || !enhancedPdfToolbar) return;
        queuedAnchor = { page: currentPage, wholePage: true };
        restoringView = true;
        zoomMode = "fitPage";
        restoreTimer.restart();
    }
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
    // "> text", a blank line and [Page N](File.pdf#page=N) (PdfSession::markdownQuote).
    function copyMarkdownQuote() {
        if (selectionText === "" || !session) return false;
        return session.copyMarkdownQuote(selectionPage, selectionText);
    }
    function copyLinkAddress(link) {
        if (!link || !session) return false;
        var address = link.page >= 0 ? session.pageLinkAddress(link.page) : String(link.url);
        if (address === "") return false;
        session.copyText(address);
        return true;
    }
    // The page's context menu: at `point` (in `item`), over `link` when there is one.
    // Opened after the click's own events (the menu is anchored to the reader,
    // which outlives the page delegate).
    function showPageMenu(item, point, link, mark) {
        var at = item.mapToItem(root, point.x, point.y);
        clearNoteHover();
        menuTimer.link = link;
        menuTimer.mark = mark || null;
        menuTimer.at = at;
        menuTimer.restart();
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
    Timer {
        id: menuTimer
        property var link: null
        property var mark: null
        property point at
        interval: 0
        onTriggered: {
            pageMenu.link = link;
            pageMenu.mark = mark;
            pageMenu.note = root.inPageNoteEditing && root.marks && mark && mark.kind === "note"
                ? root.marks.noteDraft(mark.index) : null;
            pageMenu.popup(root, at.x, at.y);
        }
    }
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
        else zoomMode = enhancedPdfToolbar && view && view.mode === "fitPage" ? "fitPage" : "fit";
        requestTimer.restart();
    }
    Connections {
        target: root.session
        // Renamed or moved with its notes: the same document, so the view stays where it is.
        function onRelocated(from) {
            if (String(from) !== String(root.shownUrl)) return;
            root.shownUrl = root.session.url;
            if (root.lastView) root.lastView.url = root.shownUrl;
        }
        function onUrlChanged() { root.rememberNow(); }
        function onRequestedPageChanged() { root.showRequestedPage(); }
        function onRevisionChanged() {
            root.clearNoteHover();
            root.clearSelection();
            root.selectedPages = [];
            root.selectionAnchorPage = -1;
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
        case Qt.Key_Delete:
        case Qt.Key_Backspace:
            if (!removeSelectedPin()) return;
            break;
        case Qt.Key_Escape:
            if (noteTool) noteTool = false;
            else if (enhancedPdfToolbar && markingMode) markingMode = false;
            else if (selectedNoteMarker !== "") selectedNoteMarker = "";
            else if (selectionText !== "") clearSelection();
            else if (markingMode) markingMode = false;
            else return;
            break;
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
        width: visible ? (root.hasOutline ? 180 : 140) : 0
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
        onContentYChanged: { root.clearNoteHover(); root.noteViewChanged(); }
        onContentXChanged: { root.clearNoteHover(); root.noteViewChanged(); }
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
                    model: root.marksByPage[pageSlot.index] || []
                    Rectangle {
                        required property var modelData
                        readonly property rect shape: root.markShape(modelData.kind, modelData.rect, modelData.direction)
                        objectName: "pdfMark_" + modelData.kind
                        x: shape.x * root.pageScale
                        y: shape.y * root.pageScale
                        width: shape.width * root.pageScale
                        height: shape.height * root.pageScale
                        radius: modelData.kind === "highlight" ? 2 : 0
                        color: root.colourValue(modelData.colour)
                        opacity: modelData.kind === "highlight" ? root.highlightOpacity : 1
                        Rectangle {
                            objectName: "pdfMarkFlash"
                            visible: root.flashIndex === modelData.index
                            anchors.fill: parent
                            anchors.margins: -3
                            radius: 4
                            color: "transparent"
                            border.width: 2
                            border.color: backend.palette.focus
                        }
                    }
                }
                Repeater {
                    model: root.notesByPage[pageSlot.index] || []
                    Item {
                        id: pin
                        required property var modelData
                        readonly property bool dragged: !!root.dragNote && root.dragNote.index === modelData.index
                        readonly property bool selected: !!root.selectedPin && root.selectedPin.index === modelData.index
                        readonly property real pinX: (dragged ? root.dragNote.x : modelData.x) * root.pageScale
                        readonly property real pinY: (dragged ? root.dragNote.y : modelData.y) * root.pageScale
                        objectName: "pdfNotePin"
                        x: pinX - root.pinSize / 2
                        y: pinY - root.pinSize / 2
                        width: root.pinSize
                        height: root.pinSize
                        // The page is always white, so the pin keeps one colour set in light and dark windows.
                        Rectangle {
                            anchors.fill: parent
                            radius: 5
                            color: "#ffd60a"
                            border.width: pin.selected ? 2 : 1
                            border.color: pin.selected ? backend.palette.focus : "#b58900"
                        }
                        LineIcon { anchors.centerIn: parent; width: 14; height: 14; name: root.inPageNoteEditing ? "note-bubble" : "note"; ink: "#5c4a00"; strokeWidth: 2 }
                        Rectangle {
                            objectName: "pdfNoteFlash"
                            visible: root.flashIndex === pin.modelData.index
                            anchors.fill: parent
                            anchors.margins: -4
                            radius: 8
                            color: "transparent"
                            border.width: 2
                            border.color: backend.palette.focus
                        }
                        Rectangle {
                            id: popover
                            objectName: "pdfNotePopover"
                            visible: !root.inPageNoteEditing && pin.selected && !pin.dragged
                            z: 10
                            x: pin.x + root.pinSize + 6 + width > page.width ? -width - 6 : root.pinSize + 6
                            y: 0
                            width: Math.min(240, Math.max(120, page.width - 16))
                            height: noteLabel.implicitHeight + 16
                            radius: 8
                            color: backend.palette.popover || backend.palette.panel
                            border.color: backend.palette.border
                            Text {
                                id: noteLabel
                                objectName: "pdfNoteText"
                                x: 8; y: 8
                                width: parent.width - 16
                                wrapMode: Text.Wrap
                                maximumLineCount: 10
                                elide: Text.ElideRight
                                text: pin.modelData.text !== "" ? pin.modelData.text : "Empty note. Type it in the notes."
                                color: pin.modelData.text !== "" ? backend.palette.text : backend.palette.muted
                                font.family: Qt.application.font.family
                                font.pixelSize: 13
                            }
                        }
                    }
                }
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
                property var pressPin: null
                cursorShape: root.noteTool ? Qt.CrossCursor : (hoverLink || root.hoveredNote) ? Qt.PointingHandCursor : Qt.IBeamCursor
                function toPoints(mouse) { return Qt.point(mouse.x / root.pageScale, mouse.y / root.pageScale); }
                onPressed: function(mouse) {
                    root.clearNoteHover();
                    root.focusPages();
                    pressPoint = toPoints(mouse);
                    dragging = false;
                    wordClick = false;
                    pressPin = root.noteTool ? null : root.pinAt(pageSlot.index, pressPoint);
                }
                onPositionChanged: function(mouse) {
                    var point = toPoints(mouse);
                    if (!pressed) {
                        hoverLink = root.linkAt(pageSlot.links, point);
                        if (root.inPageNoteEditing) {
                            var pin = root.pinAt(pageSlot.index, point);
                            root.hoverNote(pin, pin ? pageMouse.mapToItem(root, pin.x * root.pageScale, pin.y * root.pageScale) : Qt.point(0, 0));
                        }
                        return;
                    }
                    if (!dragging && Math.hypot(point.x - pressPoint.x, point.y - pressPoint.y) * root.pageScale < 3) return;
                    dragging = true;
                    if (root.noteTool) return;
                    if (pressPin) {
                        // Dragging a pin: it follows the pointer, kept on its page.
                        root.dragNote = { index: pressPin.index, page: pageSlot.index,
                            x: Math.max(0, Math.min(pageSlot.points.width, pressPin.x + point.x - pressPoint.x)),
                            y: Math.max(0, Math.min(pageSlot.points.height, pressPin.y + point.y - pressPoint.y)) };
                        return;
                    }
                    root.setSelection(pageSlot.index, root.session.selectionBetween(pageSlot.index, pressPoint, point));
                }
                onReleased: function(mouse) {
                    var released = toPoints(mouse);
                    if (root.noteTool) {
                        root.clearSelection();
                        root.placeNoteAt(pageSlot.index, released, pageMouse.mapToItem(root, mouse.x, mouse.y));
                        return;
                    }
                    if (pressPin) {
                        var pin = pressPin;
                        pressPin = null;
                        if (dragging && root.dragNote) {
                            var moved = root.dragNote;
                            root.dragNote = null;
                            if (root.marks.moveNote(pin.index, moved.x, moved.y)) {
                                var after = root.marks.items[pin.index];
                                if (after && after.kind === "note") root.selectedNoteMarker = after.marker;
                            }
                        } else {
                            root.clearSelection();
                            root.selectPin(pin);
                            if (root.inPageNoteEditing) {
                                var anchor = pageMouse.mapToItem(root, pin.x * root.pageScale, pin.y * root.pageScale);
                                root.openNoteDraft(pageSlot.index, Qt.point(pin.x, pin.y), anchor, pin.index);
                            } else root.revealInNotesRequested(pin.index);
                        }
                        return;
                    }
                    if (dragging && root.markingMode) root.markWithTool();
                    if (dragging || wordClick) return;
                    var link = root.linkAt(pageSlot.links, released);
                    if (link) { root.clearSelection(); root.selectedNoteMarker = ""; root.followLink(link); return; }
                    root.clearSelection();
                    root.selectedNoteMarker = "";
                    var mark = root.markAt(pageSlot.index, released);
                    if (mark) root.revealInNotesRequested(mark.index);
                }
                onDoubleClicked: function(mouse) {
                    if (root.inPageNoteEditing && (root.noteTool || root.pinAt(pageSlot.index, toPoints(mouse)))) {
                        mouse.accepted = false;
                        return;
                    }
                    wordClick = true;
                    root.setSelection(pageSlot.index, root.session.wordAt(pageSlot.index, toPoints(mouse)));
                    if (root.markingMode) root.markWithTool();
                }
                onExited: { hoverLink = null; root.clearNoteHover(); }
                // The platform's context-menu gesture (right-click, Control-click,
                // the Menu key); the selection is kept, as in Preview.
                ContextMenu.onRequested: function(position) {
                    var point = Qt.point(position.x / root.pageScale, position.y / root.pageScale);
                    root.showPageMenu(pageMouse, position, root.linkAt(pageSlot.links, point), root.pinAt(pageSlot.index, point) || root.markAt(pageSlot.index, point));
                }
            }
        }
    }

    Label {
        id: activeToolHint
        objectName: "pdfActiveToolHint"
        visible: root.enhancedPdfToolbar && root.ready && (root.noteTool || root.markingMode)
        anchors.horizontalCenter: pages.horizontalCenter
        anchors.top: pages.top
        anchors.topMargin: 8
        width: Math.min(implicitWidth, Math.max(0, pages.width - 16))
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        topPadding: 5; bottomPadding: 5; leftPadding: 10; rightPadding: 10
        text: root.noteTool ? "Click the page to add a note" :
            root.markTool === "underline" ? "Select text to underline" :
            root.markTool === "strike" ? "Select text to strike through" : "Highlight mode · Select text"
        color: backend.palette.text
        z: 2
        background: Rectangle {
            radius: height / 2
            color: backend.palette.popover
            border.color: backend.palette.focus
        }
    }

    Rectangle {
        objectName: "pdfNoteHover"
        visible: root.hoverReady && !!root.hoveredNote && !root.noteDraftOpen && !pageMenu.visible
        width: Math.max(0, Math.min(252, pages.width - 16))
        height: previewContent.implicitHeight + 20
        x: Math.max(pages.x + 8, Math.min(root.hoverAnchor.x + 12, pages.x + pages.width - width - 8))
        y: root.hoverAnchor.y + 14 + height <= pages.y + pages.height - 8
            ? root.hoverAnchor.y + 14 : Math.max(pages.y + 8, root.hoverAnchor.y - height - 14)
        z: 4
        color: backend.palette.popover
        border.color: backend.palette.border
        radius: 8

        Column {
            id: previewContent
            width: parent.width - 20
            anchors.centerIn: parent
            spacing: 4
            Text {
                objectName: "pdfNoteHoverAuthor"
                width: parent.width
                text: root.hoveredNote && root.hoveredNote.author ? root.hoveredNote.author : "PDF note"
                textFormat: Text.PlainText
                color: backend.palette.text
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            Text {
                objectName: "pdfNoteHoverText"
                width: parent.width
                text: root.hoveredNote ? root.hoveredNote.text || "Empty note" : ""
                textFormat: Text.PlainText
                color: backend.palette.text
                wrapMode: Text.Wrap
                maximumLineCount: 5
                elide: Text.ElideRight
            }
        }
    }

    Popup {
        id: noteEditor
        objectName: "pdfNoteEditor"
        parent: root
        modal: true
        dim: false
        focus: true
        closePolicy: Popup.CloseOnEscape
        property int page: -1
        property int noteIndex: -1
        property point point
        property point anchor
        property point dragOffset
        property string author
        property string created
        property string marker
        property string original
        property string errorText
        readonly property int gap: 12
        width: Math.min(316, Math.max(180, pages.width - 2 * gap))
        x: Math.max(pages.x + gap, Math.min(pages.x + pages.width - width - gap,
            startX + dragOffset.x))
        readonly property real startX: anchor.x + gap + width <= pages.x + pages.width ? anchor.x + gap : anchor.x - gap - width
        y: Math.max(pages.y + gap, Math.min(anchor.y - 16 + dragOffset.y, pages.y + pages.height - height - gap))
        padding: 12
        onOpened: noteInput.forceActiveFocus(Qt.PopupFocusReason)
        onClosed: if (root.visible && root.ready) root.focusPages()

        background: Rectangle {
            radius: 11
            color: root.darkMode ? backend.palette.popover : "#fff8dc"
            border.color: backend.palette.border
        }
        contentItem: ColumnLayout {
            spacing: 8
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                MouseArea {
                    objectName: "pdfNoteDragHandle"
                    anchors.fill: parent
                    cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    property point pressAt
                    property point initialOffset
                    onPressed: function(mouse) {
                        pressAt = mapToItem(root, mouse.x, mouse.y);
                        initialOffset = noteEditor.dragOffset;
                    }
                    onPositionChanged: function(mouse) {
                        if (!pressed) return;
                        var at = mapToItem(root, mouse.x, mouse.y);
                        noteEditor.dragOffset = Qt.point(initialOffset.x + at.x - pressAt.x,
                                                         initialOffset.y + at.y - pressAt.y);
                    }
                }
                Label {
                    anchors.centerIn: parent
                    text: root.session && noteEditor.page >= 0 ? "Note · Page " + root.session.pageLabel(noteEditor.page) : "Note"
                    color: backend.palette.text
                    font.weight: Font.DemiBold
                }
                ToolbarButton {
                    objectName: "pdfNoteClose"
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: 26; height: 26; iconSize: 14
                    iconName: "close"
                    iconColor: root.darkMode ? "#ff9b9b" : "#ad3333"
                    hint: "Close without applying changes"
                    onClicked: noteEditor.close()
                }
                ToolbarButton {
                    objectName: "pdfNoteDelete"
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 26; height: 26; iconSize: 14
                    iconName: "delete"
                    iconColor: root.darkMode ? "#ffd76e" : "#805900"
                    hint: "Delete note"
                    visible: noteEditor.noteIndex >= 0
                    onClicked: root.deleteDraftNote()
                }
            }
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 106
                contentWidth: availableWidth
                TextArea {
                    id: noteInput
                    objectName: "pdfNoteInput"
                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.Wrap
                    placeholderText: "Write a note…"
                    Accessible.name: "PDF note text"
                    color: backend.palette.text
                    topPadding: 8; bottomPadding: 8; leftPadding: 8; rightPadding: 8
                    background: null
                    selectByMouse: true
                    onTextChanged: noteEditor.errorText = ""
                }
            }
            Label {
                visible: text !== ""
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: noteEditor.errorText
                color: backend.palette.text
            }
            RowLayout {
                Layout.fillWidth: true
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: noteEditor.noteIndex >= 0
                    spacing: 2
                    Label {
                        Layout.fillWidth: true
                        visible: text !== ""
                        text: noteEditor.author
                        textFormat: Text.PlainText
                        color: backend.palette.muted
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                    Label {
                        objectName: "pdfNoteCreated"
                        Layout.fillWidth: true
                        text: {
                            var date = new Date(noteEditor.created);
                            return isNaN(date.getTime()) ? "Creation date unavailable" : Qt.formatDateTime(date, "d MMM yyyy · HH:mm");
                        }
                        color: backend.palette.muted
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }
                Item { Layout.fillWidth: true; visible: noteEditor.noteIndex < 0 }
                ToolbarButton {
                    objectName: "pdfNoteDone"
                    text: "Done"
                    hint: "Apply note"
                    enabled: noteInput.text.trim() !== ""
                    onClicked: root.finishNote()
                }
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
    PdfUnplacedList {
        id: unplacedList
        reader: root
    }
    function showUnplaced(anchor) { if (ready) unplacedList.showFrom(anchor); }

    CompactMenu {
        id: pageMenu
        objectName: "pdfPageMenu"
        property var link: null
        // The mark under the pointer ({index, kind, colour, …}), or null.
        property var mark: null
        readonly property bool onHighlight: !!mark && mark.kind === "highlight"
        property var note: null
        readonly property bool hasNote: !!note && !!note.marker
        onClosed: note = null
        readonly property string copyText: hasNote ? (note && note.body ? note.body : "") : root.selectionText
        readonly property bool hasSelection: !hasNote && root.selectionText !== ""
        readonly property string findLabel: {
            var text = root.selectionText.replace(/\s+/g, " ").trim();
            return text.length > 24 ? text.slice(0, 24).trim() + "…" : text;
        }
        CompactMenuItem {
            objectName: "pdfContextCopy"
            text: pageMenu.hasNote ? "Copy Note Text" : "Copy"
            enabled: pageMenu.copyText.trim() !== ""
            onTriggered: {
                if (pageMenu.hasNote) root.session.copyText(pageMenu.copyText);
                else root.copySelection();
                pageMenu.close();
            }
        }
        CompactMenuItem {
            objectName: "pdfContextCopyQuote"
            text: "Copy as Markdown Quote"
            enabled: pageMenu.copyText.trim() !== ""
            onTriggered: {
                if (pageMenu.hasNote) root.session.copyMarkdownQuote(pageMenu.mark.page, pageMenu.copyText);
                else root.copyMarkdownQuote();
                pageMenu.close();
            }
        }
        CompactMenuItem {
            objectName: "pdfContextFind"
            text: "Find “" + pageMenu.findLabel + "” in PDF"
            visible: pageMenu.hasSelection
            height: visible ? implicitHeight : 0
            onTriggered: { pageMenu.close(); root.openFind(true); }
        }
        MenuSeparator { visible: pageMenu.hasSelection; height: visible ? implicitHeight : 0 }
        CompactMenuItem { objectName: "pdfContextHighlight"; text: "Highlight"; visible: pageMenu.hasSelection; height: visible ? implicitHeight : 0; onTriggered: { pageMenu.close(); root.markSelection("highlight", root.markColour); } }
        CompactMenuItem { objectName: "pdfContextUnderline"; text: "Underline"; visible: pageMenu.hasSelection; height: visible ? implicitHeight : 0; onTriggered: { pageMenu.close(); root.markSelection("underline"); } }
        CompactMenuItem { objectName: "pdfContextStrike"; text: "Strikethrough"; visible: pageMenu.hasSelection; height: visible ? implicitHeight : 0; onTriggered: { pageMenu.close(); root.markSelection("strike"); } }
        MenuSeparator { visible: !!pageMenu.mark; height: visible ? implicitHeight : 0 }
        PdfToolMenuItem { objectName: "pdfContextColour_yellow"; text: "Yellow"; swatch: root.markColours.yellow; visible: pageMenu.onHighlight; height: visible ? implicitHeight : 0; ticked: pageMenu.onHighlight && pageMenu.mark.colour === "yellow"; onTriggered: pageMenu.recolour("yellow") }
        PdfToolMenuItem { objectName: "pdfContextColour_green"; text: "Green"; swatch: root.markColours.green; visible: pageMenu.onHighlight; height: visible ? implicitHeight : 0; ticked: pageMenu.onHighlight && pageMenu.mark.colour === "green"; onTriggered: pageMenu.recolour("green") }
        PdfToolMenuItem { objectName: "pdfContextColour_blue"; text: "Blue"; swatch: root.markColours.blue; visible: pageMenu.onHighlight; height: visible ? implicitHeight : 0; ticked: pageMenu.onHighlight && pageMenu.mark.colour === "blue"; onTriggered: pageMenu.recolour("blue") }
        PdfToolMenuItem { objectName: "pdfContextColour_pink"; text: "Pink"; swatch: root.markColours.pink; visible: pageMenu.onHighlight; height: visible ? implicitHeight : 0; ticked: pageMenu.onHighlight && pageMenu.mark.colour === "pink"; onTriggered: pageMenu.recolour("pink") }
        PdfToolMenuItem { objectName: "pdfContextColour_purple"; text: "Purple"; swatch: root.markColours.purple; visible: pageMenu.onHighlight; height: visible ? implicitHeight : 0; ticked: pageMenu.onHighlight && pageMenu.mark.colour === "purple"; onTriggered: pageMenu.recolour("purple") }
        CompactMenuItem {
            objectName: "pdfContextRemoveMark"
            text: pageMenu.mark ? "Remove " + root.markVerb(pageMenu.mark.kind) : ""
            visible: !!pageMenu.mark
            height: visible ? implicitHeight : 0
            onTriggered: { var mark = pageMenu.mark; pageMenu.close(); if (mark && root.marks) root.marks.removeMark(mark.index); }
        }
        function recolour(colour) {
            var mark = pageMenu.mark;
            pageMenu.close();
            if (mark && root.marks) root.marks.recolourMark(mark.index, colour);
        }
        MenuSeparator { visible: !!pageMenu.link; height: visible ? implicitHeight : 0 }
        CompactMenuItem {
            objectName: "pdfContextCopyLink"
            text: "Copy Link Address"
            visible: !!pageMenu.link
            height: visible ? implicitHeight : 0
            onTriggered: { pageMenu.close(); root.copyLinkAddress(pageMenu.link); }
        }
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
