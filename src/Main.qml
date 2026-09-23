import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs as Dialogs
import Qt.labs.platform as Platform
import QtQuick.Layouts
import QtQuick.Window
import "EditorMutations.js" as EditorMutations

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
    title: (backend.modified ? "* " : "") + backend.fileName + " - Omawrite"

    readonly property bool isMac: Qt.platform.os === "osx"
    readonly property bool darkMode: backend.darkMode
    readonly property color pageColor: backend.themeBackground
    readonly property color textColor: backend.themeForeground
    readonly property color strongTextColor: backend.themeForeground
    readonly property color mutedColor: backend.palette.muted
    readonly property color selectionFill: backend.themeSelection
    // The desktop's text size knob (GNOME's text-scaling-factor, which
    // `omarchy display text size` drives) anchored so its 12px default leaves
    // the app at the sizes it was designed around.
    readonly property real textScale: backend.textScale
    readonly property int editorFontPixelSize: scaledSize(workspaceSettings.writingSize)
    readonly property int editorWidth: Math.min(
        Math.round(writerFontMetrics.averageCharacterWidth * 65),
        Math.max(180, editorPane.width - 64))
    property int tabInset: 0
    Timer { interval: 250; repeat: true; running: win.visible && win.isMac; onTriggered: win.tabInset = backend.nativeTabInset() }
    readonly property int workspaceCursor: editor.cursorPosition
    function restoreWorkspaceCursor(position) {
        Qt.callLater(function() { editor.cursorPosition = Math.max(0, Math.min(position, editor.length)); editorFlick.ensureCursorVisible(); });
    }
    property bool synchronizingScroll: false
    property bool closeConfirmed: false
    property bool searchOpen: false
    property bool searchUpdating: false
    property var searchMatches: []
    property int searchMatchIndex: -1
    property url pendingOpenUrl
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
    onActiveFocusItemChanged: topChrome.keyboardReveal = isInside(activeFocusItem, topChrome)

    Settings {
        id: workspaceSettings
        objectName: "workspaceSettings"
        category: "workspace"
        property bool libraryVisible: true
        property bool organizerVisible: true
        property int layoutMode: 1
        property int writingSize: 16
        property int appearanceRevision: 0
        property bool showMarkup: true
        property string reviewWords: ""
        property bool autosaveEnabled: false
        property bool synchronizedScroll: false
        property bool typewriter: false
        property bool sentenceFocus: false
        onSentenceFocusChanged: backend.setFocusPosition(editor.cursorPosition, paragraphFocus, sentenceFocus)
        property bool paragraphFocus: false
        onParagraphFocusChanged: backend.setFocusPosition(editor.cursorPosition, paragraphFocus, sentenceFocus)
        property int titleBarMode: 1
        property int toolbarVisibilityMode: 1
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
        onShowMarkupChanged: backend.setShowMarkup(showMarkup)
    }

    WorkspaceCommands {
        id: workspaceCommands
        settings: workspaceSettings
        library: backend.library
        libraryPane: libraryPane
        window: win
        editor: editor
        preview: previewPane
        onOutlineRequested: { outlineDrawer.headings = backend.documentOutline(editor.text); outlineDrawer.open(); }
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
            case "exportHtml": exportDialog.outputFormat="html"; exportDialog.nameFilters=["HTML (*.html)"]; exportDialog.open(); break;
            case "printPreview": backend.printPreview(); break;
            case "exportPdf": exportDialog.outputFormat="pdf"; exportDialog.nameFilters=["PDF (*.pdf)"]; exportDialog.open(); break;
            case "pageBreak": editor.replaceSelectionWith("\n\n<!-- pagebreak -->\n\n"); break;
            case "writingReview": analysisDialog.open(); break;
            case "spelling": spellingDialog.open(); break;
            case "authorship": authorshipDialog.ranges=backend.authorshipRanges(); authorshipDialog.open(); break;
            case "themeSystem": backend.themePreset="system"; break;
            case "themeLight": backend.themePreset="light"; break;
            case "themeDark": backend.themePreset="dark"; break;
            case "themePaper": backend.themePreset="paper"; break;
            }
        }
    }

    component NativeCommand: NativeCommandMenuItem { commands: workspaceCommands }

    ToolBar {
        id: topChrome
        objectName: "topChrome"
        property bool keyboardReveal: false
        readonly property bool revealRequested: chromeHover.hovered || keyboardReveal
        readonly property real titleContentOpacity: workspaceSettings.titleBarMode === 1 || revealRequested ? 1 : 0
        readonly property real toolbarContentOpacity: workspaceSettings.toolbarVisibilityMode === 1 || revealRequested ? 1 : 0
        readonly property bool toolbarContentVisible: workspaceSettings.toolbarVisibilityMode !== 2
        padding: 0
        topPadding: 0
        bottomPadding: 0
        anchors.top: parent.top
        width: parent.width
        height: 44
        z: 20
        background: Rectangle {
            color: backend.palette.panel
            MouseArea { anchors.fill: parent; onPressed: win.startSystemMove(); onDoubleClicked: win.visibility === Window.Maximized ? win.showNormal() : win.showMaximized() }
        }
        HoverHandler { id: chromeHover }
        // Align the toolbar groups with the panes below, including native window controls.
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: win.isMac ? 84 : 8
            anchors.rightMargin: 10
            spacing: 5
            RowLayout {
                id: toolbarLeading
                objectName: "topChromeToolbarLeading"
                visible: topChrome.toolbarContentVisible
                opacity: topChrome.toolbarContentOpacity
                spacing: 5
                Behavior on opacity { NumberAnimation { duration: 160 } }
                ChromeButton { objectName: "topChromeLibraryButton"; iconName: "library"; hint: "Show or hide library"; darkMode: win.darkMode; checkable: true; checked: workspaceSettings.libraryVisible; onClicked: workspaceCommands.run("library") }
                ChromeButton { iconName: "organizer"; hint: "Show or hide organizer"; darkMode: win.darkMode; visible: workspaceSettings.libraryVisible; enabled: workspaceCommands.isEnabled("organizer"); onClicked: workspaceCommands.run("organizer") }
                Item { visible: organizerPane.visible; Layout.preferredWidth: Math.max(0, organizerPane.width - (win.isMac ? 164 : 88)) }
                RowLayout {
                    visible: libraryPane.visible
                    Layout.preferredWidth: libraryPane.width - 10
                    Layout.minimumWidth: libraryPane.width - 10
                    Layout.maximumWidth: libraryPane.width - 10
                    ChromeButton { iconName: "folder"; font.pixelSize: 15; font.bold: false; text: backend.library.rootName || "Choose folder"; hint: "Choose library folder"; darkMode: win.darkMode; Layout.fillWidth: true; onClicked: libraryPane.chooseFolder() }
                    ChromeButton { iconName: "plus"; hint: "New document"; darkMode: win.darkMode; onClicked: libraryPane.newDocument() }
                    ChromeButton { iconName: "down"; hint: "Library options"; darkMode: win.darkMode; onClicked: libraryPane.showOptions(this) }
                }
            }
            Label {
                objectName: "topChromeTitle"
                Accessible.description: backend.status
                text: backend.fileName
                color: backend.palette.muted
                font.pixelSize: 15
                font.weight: Font.Normal
                elide: Text.ElideMiddle
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
                opacity: topChrome.titleContentOpacity
                Behavior on opacity { NumberAnimation { duration: 160 } }
            }
            RowLayout {
                id: toolbarTrailing
                objectName: "topChromeToolbarTrailing"
                visible: topChrome.toolbarContentVisible
                opacity: topChrome.toolbarContentOpacity
                spacing: 5
                Behavior on opacity { NumberAnimation { duration: 160 } }
                ChromeButton { iconName: "outline"; hint: "Document outline"; darkMode: win.darkMode; onClicked: workspaceCommands.run("outline") }
                ChromeButton { text: "Aa"; hint: "Writing options"; darkMode: win.darkMode; onClicked: writingOptions.open() }
                ChromeButton { iconName: "search"; hint: "Find in document"; darkMode: win.darkMode; onClicked: win.openSearch(false, false) }
                ChromeButton { iconName: "preview"; hint: "Show or hide preview"; darkMode: win.darkMode; checked: workspaceSettings.layoutMode !== 0; onClicked: workspaceCommands.run("togglePreview") }
            }
        }
    }

    footer: ToolBar {
        implicitHeight: 18
        background: Rectangle { color: backend.palette.panel }
        Label {
            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
            text: backend.status; font.pixelSize: 10; color: win.mutedColor
            elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
            Accessible.name: "Document status: " + backend.status
            ToolTip.visible: statusHover.hovered
            ToolTip.text: backend.status
            HoverHandler { id: statusHover }
        }
    }

    DocumentOutline {
        id: outlineDrawer
        onJumpRequested: function(position) {
            if (workspaceSettings.layoutMode === 2) workspaceSettings.layoutMode = 1;
            editor.cursorPosition = position;
            editor.forceActiveFocus();
            editorFlick.ensureCursorVisible();
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

    Menu {
        id: formatPopover
        x: Math.max(0, editorPane.x + 12)
        y: Math.max(0, win.contentItem.height - height - 38)
        MenuItem { objectName: "saveButton"; text: "Save"; onTriggered: backend.save() }
        MenuItem { objectName: "openButton"; text: "Open…"; onTriggered: backend.openDialog() }
        MenuItem { text: "Open by Path…"; onTriggered: openPathDialog.open() }
        MenuSeparator {}
        MenuItem { text: "Strikethrough"; onTriggered: workspaceCommands.run("strike") }
        MenuItem { text: "Inline code"; onTriggered: workspaceCommands.run("inlineCode") }
    }
    Menu {
        id: writingOptions
        objectName: "writingOptions"
        width: 250
        x: Math.max(0, win.width - width - 160)
        y: 44
        Menu { title: "Theme"
            MenuItem { objectName: "themeSystem"; text: "Follow system (reset)"; checkable: true; checked: backend.themePreset === "system"; onTriggered: backend.themePreset = "system" }
            MenuItem { objectName: "themeLight"; text: "Light"; checkable: true; checked: backend.themePreset === "light"; onTriggered: backend.themePreset = "light" }
            MenuItem { objectName: "themeDark"; text: "Dark"; checkable: true; checked: backend.themePreset === "dark"; onTriggered: backend.themePreset = "dark" }
            MenuItem { objectName: "themePaper"; text: "Warm paper"; checkable: true; checked: backend.themePreset === "paper"; onTriggered: backend.themePreset = "paper" }
        }
        MenuSeparator {}
        MenuItem { text: "Show Markdown syntax"; checkable: true; checked: workspaceSettings.showMarkup; onTriggered: workspaceCommands.run("markup") }
        MenuItem { text: "Paragraph focus"; checkable: true; checked: workspaceSettings.paragraphFocus; onTriggered: workspaceCommands.run("paragraph") }
        MenuItem { text: "Typewriter scrolling"; checkable: true; checked: workspaceSettings.typewriter; onTriggered: workspaceCommands.run("typewriter") }
        MenuSeparator {}
        MenuItem { text: "Larger text"; enabled: workspaceSettings.writingSize < 32; onTriggered: workspaceCommands.run("larger") }
        MenuItem { text: "Smaller text"; enabled: workspaceSettings.writingSize > 12; onTriggered: workspaceCommands.run("smaller") }
        MenuItem { text: "Reset text size"; onTriggered: workspaceCommands.run("resetSize") }
        MenuSeparator {}
        MenuItem { text: "Template: Modern"; checkable: true; checked: backend.outputStyle === 0; onTriggered: workspaceCommands.run("sans") }
        MenuItem { text: "Template: Classic"; checkable: true; checked: backend.outputStyle === 1; onTriggered: workspaceCommands.run("serif") }
        MenuItem { text: "Template: Manuscript"; checkable: true; checked: backend.outputStyle === 2; onTriggered: workspaceCommands.run("mono") }
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

    function editMarkdown(action) {
        var result = backend.editMarkdown(action, editor.selectionStart, editor.selectionEnd);
        editor.forceActiveFocus();
        if (result.start !== undefined) editor.select(result.start, result.end);
    }

    function showCompletions() {
        completionPopup.close();
        if (editor.inputMethodComposing || editor.selectionStart !== editor.selectionEnd) return false;
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
        Qt.callLater(function() { completionList.forceActiveFocus(); });
        return true;
    }

    function acceptCompletion(index) {
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

    function openQuickSearchState(query, contents, requestedRoot, creating) {
        var root = requestedRoot && requestedRoot.toString() !== "" ? requestedRoot : backend.library.rootFolder;
        if (!root || root.toString() === "") return false;
        backend.library.rootFolder = root;
        // FileLibrary rejects missing/unreadable roots and either retains the
        // previous folder or reports an error for a now-missing current root.
        // Do not show a query against the wrong or unavailable library.
        if (backend.library.rootFolder.toString() !== root.toString() || backend.library.error !== "") {
            workspaceSettings.libraryVisible = true; // LibraryPane shows the concrete refusal.
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
        backend.rememberCursor(editor.cursorPosition);
        if (!backend.modified) {
            backend.open(url);
            return;
        }
        pendingOpenUrl = url;
        pendingAction = "open";
        unsavedChangesDialog.open();
    }

    function requestNewDocument() {
        pendingAction = "new";
        if (backend.modified) unsavedChangesDialog.open();
        else completePendingAction();
    }

    function requestCreateDocument(name, inNewWindow) {
        workspaceSettings.libraryVisible = true;
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
        workspaceSettings.libraryVisible = true;
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
            backend.open(pendingOpenUrl);
        } else if (action === "history") {
            var position = backend.navigateHistory(pendingHistoryDirection);
            if (position >= 0) Qt.callLater(function() { editor.cursorPosition = position; editor.forceActiveFocus(); editorFlick.ensureCursorVisible(); });
        } else if (action === "new") {
            backend.newDocument();
        } else if (action === "create") {
            var file = backend.library.createDocument(pendingFileName);
            if (file.toString() !== "") backend.open(file);
        }
    }

    FontMetrics {
        id: writerFontMetrics
        font.family: "iA Writer Mono S"
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

    function openSearch(withReplace, useSelection) {
        var selected = editor.selectedText;
        if (workspaceSettings.layoutMode === 2) workspaceSettings.layoutMode = 1;
        searchOpen = true;
        replaceOpen = withReplace;
        if (useSelection && selected.length > 0) searchField.text = selected;
        searchField.forceActiveFocus();
        searchField.selectAll();
        updateSearch();
    }

    function updateSearch() {
        searchMatches = backend.searchPositions(searchField.text);
        searchMatchIndex = searchMatches.length > 0 ? 0 : -1;
        showSearchMatch();
    }

    function replaceSearch(all) {
        if (searchMatchIndex < 0) return;
        var start = searchMatches[searchMatchIndex];
        searchUpdating = true;
        backend.replaceMatches(searchField.text, replaceField.text, all ? -1 : start);
        searchUpdating = false;
        backend.editorTextChanged();
        updateSearch();
    }

    function showSearchMatch() {
        var start = searchMatchIndex >= 0 ? searchMatches[searchMatchIndex] : -1;
        searchUpdating = true;
        backend.setSearchHighlight(searchField.text, start);
        if (start >= 0) {
            editor.select(start, start + searchField.text.length);
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

    function closeSearch() {
        searchOpen = false;
        searchUpdating = true;
        backend.setSearchHighlight("", -1);
        editor.deselect();
        searchUpdating = false;
        replaceOpen = false;
        editor.forceActiveFocus();
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
        onActivated: editor.wrapSelection("**", "**")
    }

    Shortcut {
        sequence: "Ctrl+I"
        context: Qt.WindowShortcut
        onActivated: editor.wrapSelection("*", "*")
    }

    Shortcut {
        sequence: "Ctrl+K"
        context: Qt.WindowShortcut
        onActivated: editor.insertLink()
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
        onActivated: win.editTarget.undo()
    }

    Shortcut {
        sequences: ["Ctrl+Shift+Z", "Ctrl+Y"]
        context: Qt.WindowShortcut
        onActivated: win.editTarget.redo()
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
    Platform.MenuBar {
        Platform.Menu {
            title: "File"
            Platform.MenuItem { objectName: "fileNewLibraryWindow"; text: "New in Library in Window…"; enabled: backend.library.rootFolder.toString() !== ""; onTriggered: libraryPane.newDocument(true) }
            Platform.MenuItem { objectName: "fileNewLibrary"; text: "New in Library…"; enabled: backend.library.rootFolder.toString() !== ""; onTriggered: libraryPane.newDocument(false) }
            Platform.MenuItem { objectName: "fileNew"; text: "New"; onTriggered: win.requestNewDocument() }
            Platform.MenuSeparator {}
            Platform.MenuItem { objectName: "fileOpen"; text: "Open…"; onTriggered: backend.openDialog() }
            RecentFilesMenu { title: "Open Recent"; library: backend.library; onOpenRequested: function(url) { win.requestOpen(url); } }
            Platform.MenuSeparator {}
            Platform.MenuItem { objectName: "fileClose"; text: "Close"; onTriggered: win.close() }
            Platform.MenuSeparator {}
            Platform.MenuItem { objectName: "fileSave"; text: "Save"; onTriggered: backend.save() }
            Platform.MenuItem { objectName: "fileDuplicate"; text: "Duplicate…"; enabled: backend.fileUrl.toString() !== ""; onTriggered: fileNameDialog.showFor(false) }
            Platform.MenuItem { objectName: "fileRename"; text: "Rename…"; enabled: backend.fileUrl.toString() !== ""; onTriggered: fileNameDialog.showFor(true) }
            Platform.MenuItem {
                objectName: "fileMove"; text: "Move To…"; enabled: backend.fileUrl.toString() !== ""
                onTriggered: { moveFolderDialog.currentFolder = backend.documentBaseUrl; moveFolderDialog.open(); }
            }
            Platform.Menu {
                title: "Versions"
                Platform.MenuItem { objectName: "fileKeepVersions"; text: "Keep Previous Version on Save"; checkable: true; checked: workspaceSettings.automaticVersions; onTriggered: { workspaceSettings.automaticVersions = !workspaceSettings.automaticVersions; backend.setAutomaticVersions(workspaceSettings.automaticVersions); } }
                Platform.MenuItem { objectName: "fileCreateVersion"; text: "Create Version of Saved File"; enabled: backend.fileUrl.toString() !== ""; onTriggered: backend.createVersion() }
                Platform.MenuItem { objectName: "fileRestoreVersion"; text: "Restore Version in Editor…"; enabled: backend.fileUrl.toString() !== ""; onTriggered: { versionsDialog.items = backend.versions(); versionsDialog.open(); } }
            }
            Platform.MenuSeparator {}
            Platform.MenuItem { objectName: "fileRevealFinder"; text: "Show in Finder"; enabled: backend.fileUrl.toString() !== ""; onTriggered: backend.showInFinder() }
            Platform.MenuItem { objectName: "fileRevealLibrary"; text: "Show in Library"; enabled: backend.fileUrl.toString() !== ""; onTriggered: win.showCurrentFileInLibrary() }
            Platform.MenuSeparator {}
            Platform.Menu {
                title: "Share"
                Platform.MenuItem { objectName: "fileShareMarkdown"; text: "Share Markdown…"; onTriggered: backend.nativeWindowAction("share") }
            }
            Platform.Menu {
                title: "Export"
                Platform.MenuItem { objectName: "fileExportHtml"; text: "Export HTML…"; onTriggered: { exportDialog.outputFormat = "html"; exportDialog.nameFilters = ["HTML (*.html)"]; exportDialog.open(); } }
                Platform.MenuItem { objectName: "fileExportPdf"; text: "Export PDF…"; onTriggered: { exportDialog.outputFormat = "pdf"; exportDialog.nameFilters = ["PDF (*.pdf)"]; exportDialog.open(); } }
            }
            Platform.Menu {
                title: "Print"
                Platform.MenuItem { objectName: "filePrintRendered"; text: "Print Rendered Document…"; onTriggered: backend.printDocument(false) }
                Platform.MenuItem { objectName: "filePrintSource"; text: "Print Markdown Source…"; onTriggered: backend.printDocument(true) }
                Platform.MenuItem { objectName: "filePrintPreview"; text: "Paginated Preview…"; onTriggered: backend.printPreview() }
            }
            Platform.MenuItem { objectName: "filePageSetup"; text: "Page Setup…"; onTriggered: backend.pageSetup() }
            Platform.MenuSeparator {}
            Platform.Menu {
                title: "Omawrite Extras"
                Platform.MenuItem { objectName: "fileNewWindow"; text: "New Window"; onTriggered: backend.newWindow() }
                Platform.MenuItem { objectName: "fileNewFolder"; text: "New Folder…"; enabled: backend.library.rootFolder.toString() !== ""; onTriggered: libraryPane.newFolder() }
                Platform.MenuItem { objectName: "fileOpenPath"; text: "Open by Path…"; shortcut: "Ctrl+Shift+O"; onTriggered: openPathDialog.open() }
                Platform.MenuItem { objectName: "fileSaveAs"; text: "Save As…"; onTriggered: backend.saveAsDialog() }
                Platform.MenuSeparator {}
                Platform.MenuItem { objectName: "fileAutosave"; text: "Autosave Saved Files Every Minute"; checkable: true; checked: workspaceSettings.autosaveEnabled; onTriggered: workspaceSettings.autosaveEnabled = !workspaceSettings.autosaveEnabled }
            }
            Platform.MenuSeparator {}
            Platform.MenuItem {
                text: "Quit Omawrite"
                role: Platform.MenuItem.QuitRole
                onTriggered: backend.requestQuit()
            }
        }
        Platform.Menu {
            title: "Edit"
            Platform.MenuItem { objectName: "editUndo"; text: "Undo"; enabled: win.editTarget.canUndo; onTriggered: win.editTarget.undo() }
            Platform.MenuItem { objectName: "editRedo"; text: "Redo"; enabled: win.editTarget.canRedo; onTriggered: win.editTarget.redo() }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Cut"; enabled: !win.editTarget.readOnly && win.editTarget.selectedText.length > 0; onTriggered: { if (win.editTarget === editor) { backend.copySelection(editor.selectionStart, editor.selectionEnd, "markdown"); editor.remove(editor.selectionStart, editor.selectionEnd); } else win.editTarget.cut(); } }
            Platform.MenuItem { text: "Copy"; enabled: win.editTarget.selectedText.length > 0; onTriggered: { if (win.editTarget === editor) backend.copySelection(editor.selectionStart, editor.selectionEnd, "markdown"); else win.editTarget.copy(); } }
            Platform.MenuItem { objectName: "editCopyFormatted"; text: "Copy Formatted"; enabled: win.editTarget === editor && editor.selectedText.length > 0; onTriggered: backend.copySelection(editor.selectionStart, editor.selectionEnd, "formatted") }
            Platform.MenuItem { objectName: "editCopyHtml"; text: "Copy HTML"; enabled: win.editTarget === editor && editor.selectedText.length > 0; onTriggered: backend.copySelection(editor.selectionStart, editor.selectionEnd, "html") }
            Platform.MenuItem { objectName: "editCopyMarkdown"; text: "Copy Markdown"; enabled: win.editTarget === editor && editor.selectedText.length > 0; onTriggered: backend.copySelection(editor.selectionStart, editor.selectionEnd, "markdown") }
            Platform.MenuItem { text: "Paste"; enabled: !win.editTarget.readOnly && win.editTarget.canPaste; onTriggered: { if (win.editTarget === editor) editor.pasteClipboardAsPlainText(); else win.editTarget.paste(); } }
            Platform.Menu {
                title: "Paste As"
                Platform.MenuItem { objectName: "editPastePlain"; text: "Plain Text"; enabled: win.editTarget === editor && editor.canPaste; onTriggered: { editor.forceActiveFocus(); editor.replaceAtomic(editor.selectionStart, editor.selectionEnd, backend.clipboardText()); } }
                Platform.MenuItem { objectName: "editPasteMarkdown"; text: "Markdown from HTML"; enabled: win.editTarget === editor && editor.canPaste; onTriggered: { editor.forceActiveFocus(); editor.replaceAtomic(editor.selectionStart, editor.selectionEnd, backend.clipboardMarkdown()); } }
            }
            Platform.MenuSeparator {}
            Platform.MenuItem { objectName: "editDelete"; text: "Delete"; enabled: !win.editTarget.readOnly && win.editTarget.selectedText.length > 0; onTriggered: win.editTarget.remove(win.editTarget.selectionStart, win.editTarget.selectionEnd) }
            Platform.MenuItem { text: "Select All"; enabled: win.editTarget.length > 0; onTriggered: win.editTarget.selectAll() }
            Platform.MenuSeparator {}
            Platform.Menu {
                title: "Find"
                Platform.MenuItem { objectName: "editFind"; text: "Find…"; onTriggered: win.openSearch(false, false) }
                Platform.MenuItem { objectName: "editReplace"; text: "Find and Replace…"; onTriggered: win.openSearch(true, false) }
                Platform.MenuItem { objectName: "editFindNext"; text: "Find Next"; enabled: win.searchOpen && win.searchMatches.length > 0; onTriggered: win.moveSearch(1) }
                Platform.MenuItem { objectName: "editFindPrevious"; text: "Find Previous"; enabled: win.searchOpen && win.searchMatches.length > 0; onTriggered: win.moveSearch(-1) }
                Platform.MenuItem { objectName: "editFindSelection"; text: "Use Selection for Find"; enabled: editor.selectedText.length > 0; onTriggered: win.openSearch(false, true) }
            }
            Platform.MenuItem { objectName: "editSpelling"; text: "Spelling and Grammar…"; enabled: editor.length > 0; onTriggered: spellingDialog.open() }
            Platform.Menu {
                title: "Transformations"
                Platform.MenuItem { objectName: "editUppercase"; text: "UPPERCASE"; enabled: win.editTarget === editor && editor.selectedText.length > 0; onTriggered: win.editMarkdown("uppercase") }
                Platform.MenuItem { objectName: "editLowercase"; text: "lowercase"; enabled: win.editTarget === editor && editor.selectedText.length > 0; onTriggered: win.editMarkdown("lowercase") }
                Platform.MenuItem { objectName: "editTitleCase"; text: "Title Case"; enabled: win.editTarget === editor && editor.selectedText.length > 0; onTriggered: win.editMarkdown("titlecase") }
            }
            Platform.Menu {
                title: "Speech"
                Platform.MenuItem { objectName: "editSpeakSelection"; text: "Speak Selection"; enabled: win.editTarget === editor && editor.selectedText.length > 0; onTriggered: backend.speakText(editor.selectedText) }
                Platform.MenuItem { objectName: "editStopSpeaking"; text: "Stop Speaking"; onTriggered: backend.stopSpeaking() }
            }
            Platform.MenuSeparator {}
            Platform.MenuItem { objectName: "editAuthorship"; text: "Authorship Annotations…"; onTriggered: { authorshipDialog.ranges = backend.authorshipRanges(); authorshipDialog.open(); } }
            Platform.MenuItem { objectName: "editExportAuthorship"; text: "Export Authorship Metadata…"; onTriggered: authorshipExportDialog.open() }
        }
        Platform.Menu {
            title: "Format"
            Platform.Menu {
                title: "Headings"
                Platform.MenuItem { text: "Heading 1"; onTriggered: win.editMarkdown("heading1") }
                Platform.MenuItem { text: "Heading 2"; onTriggered: win.editMarkdown("heading2") }
                Platform.MenuItem { text: "Heading 3"; onTriggered: win.editMarkdown("heading3") }
                Platform.MenuItem { text: "Heading 4"; onTriggered: win.editMarkdown("heading4") }
                Platform.MenuItem { text: "Heading 5"; onTriggered: win.editMarkdown("heading5") }
                Platform.MenuItem { text: "Heading 6"; onTriggered: win.editMarkdown("heading6") }
            }
            Platform.Menu {
                title: "Lists"
                Platform.MenuItem { text: "List"; onTriggered: win.editMarkdown("bullet") }
                Platform.MenuItem { text: "Task List"; onTriggered: win.editMarkdown("task") }
                Platform.MenuItem { text: "Ordered List"; onTriggered: win.editMarkdown("ordered") }
                Platform.MenuItem { text: "Ordered Task List"; onTriggered: win.editMarkdown("orderedTask") }
                Platform.MenuSeparator {}
                Platform.MenuItem { text: "Mark Task as Completed"; onTriggered: win.editMarkdown("toggleTask") }
            }
            Platform.MenuItem { text: "Blockquote"; onTriggered: win.editMarkdown("quote") }
            Platform.MenuItem { text: "Body"; onTriggered: win.editMarkdown("body") }
            Platform.Menu {
                title: "Structure"
                Platform.MenuItem { text: "Indent"; onTriggered: win.editMarkdown("indent") }
                Platform.MenuItem { text: "Outdent"; onTriggered: win.editMarkdown("outdent") }
                Platform.MenuItem { text: "Move Line Up"; onTriggered: win.editMarkdown("lineUp") }
                Platform.MenuItem { text: "Move Line Down"; onTriggered: win.editMarkdown("lineDown") }
            }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Bold"; onTriggered: editor.wrapSelection("**", "**") }
            Platform.MenuItem { text: "Italic"; onTriggered: editor.wrapSelection("*", "*") }
            NativeCommand { commandId: "strike" }
            Platform.MenuItem { text: "Highlight"; onTriggered: editor.wrapSelection("==", "==") }
            Platform.MenuSeparator {}
            NativeCommand { commandId: "inlineCode"; text: "Code" }
            Platform.MenuItem { text: "Code Block"; onTriggered: win.editMarkdown("codeBlock") }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Add Link"; onTriggered: editor.insertLink() }
            Platform.MenuItem { text: "Add Wikilink"; onTriggered: editor.wrapSelection("[[", "]]") }
            Platform.MenuItem { text: "Add Footnote"; onTriggered: editor.replaceAtomic(editor.selectionStart, editor.selectionEnd, "[^note]\n\n[^note]: Note text") }
            Platform.MenuItem { text: "Add Content Block"; onTriggered: editor.replaceAtomic(editor.selectionStart, editor.selectionEnd, "\n/chapter.md\n") }
            Platform.MenuItem { text: "Add Hashtag"; onTriggered: editor.replaceAtomic(editor.selectionStart, editor.selectionEnd, "#tag") }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Add Date"; onTriggered: win.editMarkdown("date") }
            Platform.MenuItem { text: "Add Table"; onTriggered: win.editMarkdown("table") }
            Platform.MenuItem { text: "Add Table of Contents"; onTriggered: editor.replaceAtomic(editor.selectionStart, editor.selectionEnd, backend.tableOfContents(editor.text)) }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Add Horizontal Rule"; onTriggered: win.editMarkdown("rule") }
            Platform.MenuItem { text: "Add Page Break"; onTriggered: editor.replaceSelectionWith("\n\n<!-- pagebreak -->\n\n") }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Clear Styles"; enabled: editor.selectedText.length > 0; onTriggered: win.editMarkdown("clearStyles") }
        }
        Platform.Menu {
            title: "View"
            Platform.Menu {
                title: "Template"
                Platform.MenuItem { text: "Modern (Sans)"; checkable: true; checked: backend.outputStyle === 0; onTriggered: backend.setOutputStyle(0) }
                Platform.MenuItem { text: "Classic (Serif)"; checkable: true; checked: backend.outputStyle === 1; onTriggered: backend.setOutputStyle(1) }
                Platform.MenuItem { text: "Manuscript (Mono)"; checkable: true; checked: backend.outputStyle === 2; onTriggered: backend.setOutputStyle(2) }
                Platform.MenuSeparator {}
                Platform.MenuItem { text: "GitHub"; checkable: true; checked: backend.outputStyle === 4; onTriggered: backend.setOutputStyle(4) }
                Platform.MenuItem { text: "Helvetica"; checkable: true; checked: backend.outputStyle === 5; onTriggered: backend.setOutputStyle(5) }
                Platform.MenuItem { text: "Palatino"; checkable: true; checked: backend.outputStyle === 6; onTriggered: backend.setOutputStyle(6) }
                Platform.MenuItem { text: "MLA Draft"; checkable: true; checked: backend.outputStyle === 7; onTriggered: backend.setOutputStyle(7) }
                Platform.MenuSeparator {}
                Platform.MenuItem { text: "Custom"; checkable: true; checked: backend.outputStyle === 3; onTriggered: backend.setOutputStyle(3) }
                Platform.MenuItem { text: "Load Custom Template…"; onTriggered: outputStyleDialog.open() }
            }
            Platform.MenuItem { text: "Synchronized Scrolling"; checkable: true; checked: workspaceSettings.synchronizedScroll; onTriggered: workspaceSettings.synchronizedScroll = !workspaceSettings.synchronizedScroll }
            NativeCommand { commandId: "library" }
            NativeCommand { commandId: "organizer" }
            Platform.MenuSeparator {}
            NativeCommand { commandId: "sortBar" }
            NativeCommand { commandId: "filterBar" }
            Platform.Menu {
                title: "Sort Files By"
                NativeCommand { commandId: "sortName" }
                NativeCommand { commandId: "sortModified" }
                NativeCommand { commandId: "sortCreated" }
                NativeCommand { commandId: "sortExtension" }
                Platform.MenuSeparator {}
                NativeCommand { commandId: "ascending" }
                NativeCommand { commandId: "descending" }
                Platform.MenuSeparator {}
                NativeCommand { commandId: "foldersFirst" }
            }
            Platform.Menu {
                title: "View Options"
                Platform.Menu {
                    title: "Show Date"
                    NativeCommand { commandId: "dateModified" }
                    NativeCommand { commandId: "dateCreated" }
                    NativeCommand { commandId: "dateNone" }
                }
                NativeCommand { commandId: "excerpts" }
                Platform.Menu {
                    title: "Navigation"
                    NativeCommand { commandId: "navigationTree" }
                    NativeCommand { commandId: "navigationList" }
                }
            }
            Platform.MenuSeparator {}
            Platform.Menu {
                title: "Text Size"
                NativeCommand { commandId: "larger" }
                NativeCommand { commandId: "smaller" }
                NativeCommand { commandId: "resetSize" }
            }
            Platform.MenuItem {
                objectName: "native_showCompletions"
                text: "Show Completions"
                enabled: win.editTarget === editor && editor.selectionStart === editor.selectionEnd
                onTriggered: win.showCompletions()
            }
            NativeCommand { commandId: "markup" }
            Platform.MenuSeparator {}
            NativeCommand { commandId: "reloadPreview" }
            Platform.Menu {
                title: "Preview"
                NativeCommand { commandId: "preview"; text: "Full" }
                NativeCommand { commandId: "split"; text: "Split" }
                Platform.MenuSeparator {}
                NativeCommand { commandId: "webPreview" }
                Platform.Menu {
                    title: "PDF"
                    Platform.MenuItem { objectName: "native_pdfPreview"; text: "Paginated Preview…"; onTriggered: backend.printPreview() }
                }
            }
            NativeCommand { commandId: "editor" }
            Platform.MenuSeparator {}
            NativeCommand { commandId: "outline" }
            NativeCommand { commandId: "statistics" }
            Platform.Menu {
                title: "Title Bar"
                NativeCommand { commandId: "titleBarFade" }
                NativeCommand { commandId: "titleBarAlways" }
            }
            Platform.Menu {
                title: "Toolbar"
                NativeCommand { commandId: "toolbarFade" }
                NativeCommand { commandId: "toolbarAlways" }
                NativeCommand { commandId: "toolbarHide" }
                Platform.MenuSeparator {}
                NativeCommand { commandId: "toolbarDefault" }
                Platform.Menu {
                    id: toolbarStatsOnlyMenu
                    title: "Stats Only"
                    Binding { target: toolbarStatsOnlyMenu.menuItem; property: "objectName"; value: "native_toolbarStatsOnly" }
                    Binding { target: toolbarStatsOnlyMenu.menuItem; property: "checkable"; value: true }
                    Binding { target: toolbarStatsOnlyMenu.menuItem; property: "checked"; value: workspaceCommands.isChecked("toolbarStatsOnly") }
                    NativeCommand { commandId: "toolbarCharacters" }
                    NativeCommand { commandId: "toolbarCharactersNoSpaces" }
                    NativeCommand { commandId: "toolbarWords" }
                    NativeCommand { commandId: "toolbarSentences" }
                    NativeCommand { commandId: "toolbarReadingTime" }
                    NativeCommand { commandId: "toolbarSpeakingTime" }
                    NativeCommand { commandId: "toolbarTasks" }
                    NativeCommand { commandId: "toolbarHuman" }
                    NativeCommand { commandId: "toolbarAI" }
                    NativeCommand { commandId: "toolbarReference" }
                }
            }
            Platform.MenuSeparator {}
            // AppKit supplies the native Full Screen item automatically.
        }
        Platform.Menu {
            title: "Window"
            visible: win.isMac
            Platform.MenuItem { text: "Minimize"; onTriggered: backend.nativeWindowAction("minimize") }
            Platform.MenuItem { text: "Zoom"; onTriggered: backend.nativeWindowAction("zoom") }
            Platform.MenuItem { text: "Bring All to Front"; onTriggered: backend.nativeWindowAction("front") }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Merge All Windows"; onTriggered: backend.nativeWindowAction("merge") }
            Platform.MenuItem { text: "Next Tab"; onTriggered: backend.nativeWindowAction("nextTab") }
            Platform.MenuItem { text: "Previous Tab"; onTriggered: backend.nativeWindowAction("previousTab") }
            Platform.MenuItem { text: "Move Tab to New Window"; onTriggered: backend.nativeWindowAction("detach") }
            Platform.MenuItem { text: "Toggle Tab Bar"; onTriggered: backend.nativeWindowAction("tabBar") }
            Platform.MenuItem { text: "Tab Overview"; onTriggered: backend.nativeWindowAction("overview") }
        }
        Platform.Menu {
            title: "Focus"
            Platform.MenuItem { text: "Writing Review…"; onTriggered: analysisDialog.open() }
            NativeCommand { commandId: "paragraph" }
            NativeCommand { commandId: "sentence" }
            NativeCommand { commandId: "typewriter" }
        }
        Platform.Menu {
            title: "Go"
            Platform.MenuItem { objectName: "goBack"; text: "Back"; enabled: backend.canGoBack; onTriggered: win.requestHistory(-1) }
            Platform.MenuItem { objectName: "goForward"; text: "Forward"; enabled: backend.canGoForward; onTriggered: win.requestHistory(1) }
            Platform.MenuSeparator {}
            Platform.MenuItem { objectName: "goLibraryBack"; text: "Back in Library"; enabled: backend.library.canGoBack; onTriggered: backend.library.navigateHistory(-1) }
            Platform.MenuItem { objectName: "goLibraryForward"; text: "Forward in Library"; enabled: backend.library.canGoForward; onTriggered: backend.library.navigateHistory(1) }
            Platform.MenuItem { objectName: "goEnclosingFolder"; text: "Enclosing Folder"; enabled: backend.library.rootFolder.toString() !== ""; onTriggered: backend.library.enclosingFolder() }
            Platform.MenuSeparator {}
            Platform.MenuItem { objectName: "goOpenLink"; text: "Open Link"; enabled: backend.sourceLinkAt(editor.cursorPosition).toString() !== ""; onTriggered: win.openSourceLink() }
            Platform.MenuItem { objectName: "goQuickSearch"; text: "Quick Search…"; enabled: backend.library.rootFolder.toString() !== ""; onTriggered: win.openQuickSearchState("", false, backend.library.rootFolder, false) }
            Platform.MenuItem { objectName: "goCommandPalette"; text: "Command Palette…"; shortcut: "Ctrl+Shift+P"; onTriggered: commandPalette.open() }
            Platform.MenuSeparator {}
            Platform.Menu {
                id: locationsMenu
                title: "Locations"
                Instantiator {
                    model: backend.library.locations
                    delegate: Platform.MenuItem {
                        required property var modelData
                        required property int index
                        objectName: "goLocation_" + index
                        text: modelData.name + (modelData.available ? "" : " (Unavailable)")
                        enabled: modelData.available
                        onTriggered: { backend.library.rootFolder = modelData.url; workspaceSettings.libraryVisible = true; }
                    }
                    onObjectAdded: function(index, object) { locationsMenu.insertItem(index, object); }
                    onObjectRemoved: function(index, object) { locationsMenu.removeItem(object); }
                }
                Platform.MenuSeparator {}
                Platform.MenuItem { objectName: "goAddLocation"; text: "Add Location…"; onTriggered: libraryPane.chooseFolder() }
            }
            Platform.Menu {
                id: smartFoldersMenu
                title: "Smart Folders"
                RecentFilesMenu { title: "Recents"; library: backend.library; onOpenRequested: function(url) { win.requestOpen(url); } }
                Platform.MenuItem {
                    objectName: "goNewSmartFolder"
                    text: "New Smart Folder…"
                    enabled: backend.library.rootFolder.toString() !== ""
                    onTriggered: win.openQuickSearchState("", false, backend.library.rootFolder, true)
                }
                Platform.MenuSeparator { visible: backend.library.savedSearches.length > 0 }
                Instantiator {
                    model: backend.library.savedSearches
                    delegate: Platform.MenuItem {
                        required property var modelData
                        required property int index
                        objectName: "goSavedSearch_" + index
                        text: modelData.query + (modelData.contents ? " — contents" : " — filenames")
                        onTriggered: win.openSavedSearch(modelData)
                    }
                    onObjectAdded: function(index, object) { smartFoldersMenu.insertItem(index + 3, object); }
                    onObjectRemoved: function(index, object) { smartFoldersMenu.removeItem(object); }
                }
            }
            Platform.Menu {
                id: hashtagsMenu
                title: "Hashtags"
                Instantiator {
                    model: backend.library.tagIndex
                    delegate: Platform.MenuItem {
                        required property var modelData
                        required property int index
                        objectName: "goHashtag_" + modelData.tag
                        text: "#" + modelData.tag + " (" + modelData.count + ")"
                        enabled: backend.library.rootFolder.toString() !== ""
                        onTriggered: win.openHashtag(modelData.tag)
                    }
                    onObjectAdded: function(index, object) { hashtagsMenu.insertItem(index, object); }
                    onObjectRemoved: function(index, object) { hashtagsMenu.removeItem(object); }
                }
                Platform.MenuItem {
                    objectName: "goHashtagsEmpty"
                    text: backend.library.tagStatus
                    enabled: false
                    visible: backend.library.tagIndex.length === 0
                }
                Platform.MenuSeparator {}
                Platform.MenuItem {
                    objectName: "goHashtagStatus"
                    text: backend.library.tagStatus
                    enabled: false
                    visible: backend.library.tagIndex.length > 0
                }
                Platform.MenuItem { objectName: "goRefreshHashtags"; text: "Refresh Hashtags"; enabled: backend.library.rootFolder.toString() !== ""; onTriggered: backend.library.refreshTags() }
            }
        }
        Platform.Menu {
            title: "Help"
            Platform.MenuItem { text: "Keyboard Shortcuts"; onTriggered: shortcutsDialog.open() }
        }
    }

        }
    }

    Connections {
        target: backend

        function onDocumentLoaded() {
            Qt.callLater(function() {
                editor.cursorPosition = 0;
                editorFlick.contentY = 0;
            });
        }

        function onOpenDialogRequested() {
            openFileDialog.open();
        }

        function onSaveDialogRequested(suggestedUrl) {
            saveFileDialog.selectedFile = suggestedUrl;
            saveFileDialog.open();
        }

        function onCloseAfterSave() {
            win.closeConfirmed = true;
            win.close();
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
            Label { Layout.fillWidth: true; wrapMode: Text.Wrap; text: "Manual labels, not verified provenance. Edits can inherit nearby labels. Save writes a hidden .omawrite-authors.json sidecar; keep it with the Markdown file. External edits invalidate labels. Copy/paste between Omawrite windows preserves labels; other apps may remove them. External plain text is unlabelled. Export metadata separately to keep manual assertions with matching Markdown." }
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
    Dialogs.FileDialog {
        id: outputStyleDialog
        title: "Load Custom Template"
        nameFilters: ["Custom template (*.json)"]
        onAccepted: backend.loadOutputStyle(selectedFile)
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
                workspaceSettings.libraryVisible = true;
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
        onCancelRequested: { win.pendingAction = ""; backend.cancelQuit(); }
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
    }

    Dialog {
        id: shortcutsDialog
        modal: true
        title: "Keyboard shortcuts"
        width: Math.min(win.width - 40, win.scaledSize(440))
        standardButtons: Dialog.Close
        anchors.centerIn: parent
        contentItem: Label {
            text: win.isMac ? "⌘S  Save\n⇧⌘S  Save As\n⌘O  Open\n⌘N  New Window\n⌘W  Close Window\n⌘F  Find\n⌥⌘F  Find and Replace\n⌘B  Bold\n⌘I  Italic\n⌘K  Link\n⌘P  Print\n⌃⌘F  Fullscreen\n⌘?  Shortcuts" : "Ctrl+S  Save\nCtrl+Shift+S  Save As\nCtrl+O  Open\nCtrl+N  New Window\nCtrl+F  Find\nCtrl+H  Find and Replace\nCtrl+B  Bold\nCtrl+I  Italic\nCtrl+K  Link\nCtrl+P  Print\nF11 / Super+F  Fullscreen\nCtrl+?  Shortcuts"
            lineHeight: 1.5
        }
    }

    SplitView {
        anchors.top: topChrome.bottom
        anchors.topMargin: win.tabInset
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        orientation: Qt.Horizontal
        handle: Rectangle {
            implicitWidth: 1
            color: SplitHandle.hovered || SplitHandle.pressed ? "#426da7" : (backend.palette.border)
        }
        OrganizerPane {
            commands: workspaceCommands
            id: organizerPane
            onSearchRequested: function(query, contents, folder) {
                backend.library.rootFolder = folder;
                quickQuery.text = query; quickContents.checked = contents; quickOpenDialog.open();
            }
            library: backend.library
            currentFile: backend.fileUrl
            darkMode: win.darkMode
            visible: workspaceSettings.libraryVisible && workspaceSettings.organizerVisible && win.width >= 1000
            SplitView.preferredWidth: 210
            SplitView.minimumWidth: 160
            SplitView.maximumWidth: 300
            onOpenRequested: function(file) { win.requestOpen(file); }
        }
        LibraryPane {
            id: libraryPane
            commands: workspaceCommands
            onCreateRequested: function(name, inNewWindow) { win.requestCreateDocument(name, inNewWindow); }
            library: backend.library
            currentFile: backend.fileUrl
            darkMode: win.darkMode
            visible: workspaceSettings.libraryVisible
            SplitView.preferredWidth: 290
            SplitView.minimumWidth: 180
            SplitView.maximumWidth: 420
            onOpenRequested: function(file) { win.requestOpen(file); }
        }
        Rectangle {
            id: editorPane
            color: backend.palette.page
            visible: workspaceSettings.layoutMode !== 2
            SplitView.fillWidth: true
            SplitView.minimumWidth: 260

        Flickable {
            id: editorFlick
            onContentYChanged: {
                if (!workspaceSettings.synchronizedScroll || win.synchronizingScroll || workspaceSettings.layoutMode !== 1) return;
                win.synchronizingScroll = true;
                previewPane.scrollToFraction(contentY / Math.max(1, contentHeight - height));
                win.synchronizingScroll = false;
            }
            objectName: "editorScroll"
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            anchors.bottomMargin: 34
            anchors.topMargin: win.searchOpen ? searchPane.height + 24 : 0
            clip: true
            contentWidth: width
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
                    scrollLinger.restart();
                    if (wheel.pixelDelta.y !== 0)
                        editorFlick.scrollTo(editorFlick.clampContentY(editorFlick.contentY - wheel.pixelDelta.y));
                    else
                        editorFlick.scrollByWheel(wheel);
                    wheel.accepted = true;
                }
            }

            onMovementStarted: wheelScroll.stop()

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
                x: Math.round((editorFlick.width - width) / 2)
                y: workspaceSettings.typewriter ? editorFlick.height / 2 : 10
                width: win.editorWidth
                height: Math.max(editorFlick.height - y - 96, implicitHeight + 20)
                text: ""
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                persistentSelection: true
                activeFocusOnPress: true
                color: win.textColor
                selectedTextColor: "#ffffff"
                selectionColor: win.selectionFill
                font.family: "iA Writer Mono S"
                font.pixelSize: win.editorFontPixelSize
                font.weight: Font.Normal
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
                onCursorRectangleChanged: editorFlick.ensureCursorVisible()
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
                    var result = backend.replaceText(start, end, text);
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

                function smartReturn(softBreak) {
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
                    var end = backend.pasteWithAuthorship(selectionStart, selectionEnd);
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
                    if ((event.key === Qt.Key_C || event.key === Qt.Key_X) && (event.modifiers & Qt.ControlModifier)
                        && !(event.modifiers & (Qt.AltModifier | Qt.MetaModifier | Qt.ShiftModifier))) {
                        if (backend.copySelection(selectionStart, selectionEnd, "markdown") && event.key === Qt.Key_X) remove(selectionStart, selectionEnd);
                        event.accepted = true; return;
                    }
                    var pasteKey = (event.key === Qt.Key_V)
                        && (event.modifiers & Qt.ControlModifier)
                        && !(event.modifiers & (Qt.AltModifier | Qt.MetaModifier | Qt.ShiftModifier));
                    var shiftInsert = (event.key === Qt.Key_Insert)
                        && (event.modifiers & Qt.ShiftModifier)
                        && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier));
                    if (pasteKey || shiftInsert) {
                        if (!pasteClipboardUrlAsMarkdownLink())
                            pasteClipboardAsPlainText();
                        event.accepted = true;
                        return;
                    }

                    var returnKey = event.key === Qt.Key_Return || event.key === Qt.Key_Enter;
                    var commandModifier = event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier);
                    if (returnKey && !commandModifier) {
                        smartReturn(event.modifiers & Qt.ShiftModifier);
                        event.accepted = true;
                    } else if (!commandModifier && event.key === Qt.Key_Backspace
                               && deleteParagraphBreakBehindCursor()) {
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

                onTextChanged: {
                    if (win.searchUpdating)
                        return;
                    var contentChanged = backend.editorTextChanged();
                    if (win.searchOpen && contentChanged)
                        win.updateSearch();
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
                    backend.setShowMarkup(workspaceSettings.showMarkup);
                    backend.attachDocument(textDocument);
                    backend.setFocusPosition(cursorPosition, workspaceSettings.paragraphFocus, workspaceSettings.sentenceFocus);
                    forceActiveFocus();
                }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 34
            color: backend.palette.panel
            Rectangle { width: parent.width; height: 1; color: backend.palette.border }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 10
                spacing: 2
                ChromeButton { visible: workspaceSettings.toolbarMode !== 1; text: "Bold"; hint: "Bold selection"; darkMode: win.darkMode; onClicked: editor.wrapSelection("**", "**") }
                ChromeButton { visible: workspaceSettings.toolbarMode !== 1; text: "Italic"; hint: "Italic selection"; darkMode: win.darkMode; onClicked: editor.wrapSelection("*", "*") }
                ChromeButton { visible: workspaceSettings.toolbarMode !== 1; text: "Link"; hint: "Insert link"; darkMode: win.darkMode; onClicked: editor.insertLink() }
                ChromeButton { visible: workspaceSettings.toolbarMode !== 1; text: "More"; hint: "More formatting"; darkMode: win.darkMode; onClicked: formatPopover.open() }
                Item { visible: workspaceSettings.toolbarMode !== 1; Layout.fillWidth: true }
                ChromeButton {
                    objectName: "toolbarStatistic"
                    text: workspaceSettings.toolbarMode === 1 ? win.compactToolbarStatistics() : win.compactStatistic("words")
                    hint: workspaceSettings.toolbarMode === 1 ? win.compactToolbarStatistics() : "Document statistics"
                    darkMode: win.darkMode
                    Layout.fillWidth: workspaceSettings.toolbarMode === 1
                    Layout.maximumWidth: workspaceSettings.toolbarMode === 1 ? editorPane.width - 20 : implicitWidth
                    onClicked: workspaceCommands.run("statistics")
                }
            }
        }


        Pane {
            id: searchPane
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 12
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            height: win.scaledSize(win.replaceOpen ? 104 : 56)
            visible: win.searchOpen
            z: 10
            leftPadding: 16
            rightPadding: 8
            topPadding: 0
            bottomPadding: 0
            Material.elevation: 8

            background: Rectangle {
                radius: 9
                color: win.darkMode ? "#22221f" : "#fffef2"
            }

            RowLayout {
                anchors.fill: parent
                spacing: 8

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TextInput {
                        id: searchField
                        objectName: "searchField"
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        height: win.replaceOpen ? parent.height / 2 : parent.height
                        verticalAlignment: TextInput.AlignVCenter
                        selectByMouse: true
                        color: win.textColor
                        selectionColor: win.selectionFill
                        selectedTextColor: "#ffffff"
                        font.pixelSize: win.scaledSize(17)
                        clip: true
                        onTextChanged: win.updateSearch()
                        Keys.onReturnPressed: function(event) {
                            win.moveSearch((event.modifiers & Qt.ShiftModifier) ? -1 : 1);
                            event.accepted = true;
                        }
                        Keys.onEscapePressed: function(event) {
                            win.closeSearch();
                            event.accepted = true;
                        }
                    }

                    TextInput {
                        id: replaceField
                        objectName: "replaceField"
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: parent.height / 2
                        visible: win.replaceOpen
                        verticalAlignment: TextInput.AlignVCenter
                        color: win.textColor
                        selectionColor: win.selectionFill
                        selectedTextColor: "#ffffff"
                        font.pixelSize: win.scaledSize(17)
                        Keys.onReturnPressed: replaceCurrentButton.clicked()
                    }

                    Label {
                        anchors.verticalCenter: replaceField.verticalCenter
                        text: "Replace with"
                        visible: win.replaceOpen && replaceField.text.length === 0
                        color: win.mutedColor
                        font.pixelSize: win.scaledSize(17)
                    }

                    Label {
                        anchors.verticalCenter: searchField.verticalCenter
                        text: "Find"
                        visible: searchField.text.length === 0
                        color: win.mutedColor
                        font.pixelSize: win.scaledSize(17)
                    }
                }

                Label {
                    Layout.preferredWidth: win.scaledSize(58)
                    Layout.fillHeight: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: win.searchMatches.length === 0
                        ? "0/0"
                        : (win.searchMatchIndex + 1) + "/" + win.searchMatches.length
                    color: win.darkMode ? win.textColor : "#62635f"
                    font.pixelSize: win.scaledSize(16)
                }

                Button {
                    id: replaceCurrentButton
                    visible: win.replaceOpen
                    text: "Replace"
                    enabled: win.searchMatches.length > 0
                    onClicked: win.replaceSearch(false)
                }

                Button {
                    objectName: "replaceAllButton"
                    visible: win.replaceOpen
                    text: "All"
                    enabled: win.searchMatches.length > 0
                    onClicked: win.replaceSearch(true)
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 34
                    color: win.darkMode ? "#6f6f62" : "#d5d56e"
                }

                SearchIconButton {
                    iconName: "up"
                    iconColor: win.darkMode ? win.textColor : "#62635f"
                    onClicked: win.moveSearch(-1)
                }

                SearchIconButton {
                    iconName: "down"
                    iconColor: win.darkMode ? win.textColor : "#62635f"
                    onClicked: win.moveSearch(1)
                }

                SearchIconButton {
                    iconName: "close"
                    iconColor: win.darkMode ? win.textColor : "#62635f"
                    onClicked: win.closeSearch()
                }
            }
        }
    }

        PreviewPane {
            id: previewPane
            renderer: backend
            onScrollFractionChanged: function(fraction) {
                if (!workspaceSettings.synchronizedScroll || win.synchronizingScroll || workspaceSettings.layoutMode !== 1) return;
                win.synchronizingScroll = true;
                editorFlick.contentY = Math.max(0, editorFlick.contentHeight - editorFlick.height) * fraction;
                win.synchronizingScroll = false;
            }
            markdown: editor.text
            documentBaseUrl: backend.documentBaseUrl
            darkMode: win.darkMode
            visible: workspaceSettings.layoutMode !== 0
            SplitView.fillWidth: workspaceSettings.layoutMode === 2
            SplitView.preferredWidth: Math.max(260, (win.width - (organizerPane.visible ? organizerPane.width : 0) - (libraryPane.visible ? libraryPane.width : 0)) / 2)
            SplitView.minimumWidth: 220
            typeface: backend.outputFont
            textSize: Math.max(12, backend.outputPointSize * 4 / 3 + workspaceSettings.writingSize - 19)
            layoutMode: workspaceSettings.layoutMode
            onLayoutRequested: function(mode) { workspaceCommands.run(["editor", "split", "preview"][mode]); }
            onLinkRequested: function(link) {
                var resolved = backend.resolveDocumentLink(link);
                if (/^file:.*\.(md|markdown|mdown|txt|text)(#.*)?$/i.test(String(resolved)))
                    win.requestOpen(resolved);
                else
                    backend.openExternalUrl(resolved);
            }
        }
    }

    Component.onCompleted: {
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
