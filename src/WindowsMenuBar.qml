import QtQuick
import QtQuick.Controls

// Desktop menu access on Windows. Commands share the workspace registry;
// editing actions use the same focus-aware handlers as the existing shortcuts.
// Shortcuts remain in Main.qml, avoiding two registrations for each keystroke.
MenuBar {
    id: root
    objectName: "windowsMenuBar"
    property var previousFocusItem: null
    property var focusMenus: []
    function ownsFocus(item) {
        if (!item) return false;
        if (win.isInside(item, root)) return true;
        for (var popup of focusMenus)
            if (popup.contentItem && win.isInside(item, popup.contentItem.parent)) return true;
        return false;
    }
    function rememberEditingFocus(item) {
        if (item && !ownsFocus(item)) previousFocusItem = item;
    }
    function restoreEditingFocus() {
        if (previousFocusItem && previousFocusItem.visible && previousFocusItem.enabled)
            previousFocusItem.forceActiveFocus(Qt.MenuBarFocusReason);
    }
    function dispatch(action) {
        restoreEditingFocus();
        action();
    }
    Component.onCompleted: rememberEditingFocus(win.activeFocusItem)
    component WorkspaceMenu: Menu {
        id: popup
        // Long menus must scroll inside short windows instead of placing
        // enabled commands below the area that can receive a pointer click.
        height: Math.min(implicitHeight, Math.max(0, win.contentItem.height - 16))
        margins: 8
        // Include nested popup content in the same focus ownership boundary.
        Component.onCompleted: root.focusMenus = root.focusMenus.concat([popup])
        onAboutToShow: root.rememberEditingFocus(win.activeFocusItem)
        onClosed: Qt.callLater(function() {
            if (!root) return;
            for (var other of root.focusMenus) if (other.visible) return;
            if (root.ownsFocus(win.activeFocusItem)) root.restoreEditingFocus();
        })
    }
    component Command: MenuItem {
        id: item
        required property string commandId
        objectName: "windows_" + commandId
        text: workspaceCommands.label(commandId)
        enabled: workspaceCommands.isEnabled(commandId)
        checkable: !!workspaceCommands.entry(commandId).toggle
        checked: workspaceCommands.isChecked(commandId)
        onTriggered: {
            root.dispatch(function() { workspaceCommands.run(commandId) })
            checked = Qt.binding(function() { return workspaceCommands.isChecked(item.commandId) })
        }
    }
    WorkspaceMenu {
        title: "&File"
        Command { commandId: "new" }
        Command { commandId: "newWindow" }
        Command { commandId: "open" }
        Command { commandId: "openPath" }
        MenuSeparator {}
        Command { commandId: "save" }
        Command { commandId: "saveAs" }
        Command { commandId: "duplicate" }
        Command { commandId: "rename" }
        Command { commandId: "move" }
        MenuItem { text: "Open Containing Folder"; enabled: backend.fileUrl.toString() !== ""; onTriggered: root.dispatch(function() { backend.showInFinder() }) }
        MenuSeparator {}
        Command { commandId: "exportHtml" }
        Command { commandId: "exportPdf" }
        MenuItem { text: "Print…"; onTriggered: root.dispatch(function() { backend.printDocument(false) }) }
        Command { commandId: "printPreview" }
        MenuItem { text: "Page Setup…"; onTriggered: root.dispatch(function() { backend.pageSetup() }) }
        MenuSeparator {}
        MenuItem { text: "&Close Window"; onTriggered: root.dispatch(function() { win.close() }) }
        MenuItem { text: "E&xit"; onTriggered: root.dispatch(function() { backend.requestQuit() }) }
    }
    WorkspaceMenu {
        title: "&Edit"
        MenuItem { text: "&Undo"; enabled: win.canFormatLive || win.editTarget.canUndo; onTriggered: root.dispatch(function() { win.performDocumentEdit(function() { win.undoEditing() }) }) }
        MenuItem { text: "&Redo"; enabled: win.canFormatLive || win.editTarget.canRedo; onTriggered: root.dispatch(function() { win.performDocumentEdit(function() { win.redoEditing() }) }) }
        MenuSeparator {}
        MenuItem { text: "Cu&t"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && !win.editTarget.readOnly && win.editTarget.selectedText.length > 0); onTriggered: root.dispatch(function() { if (win.canFormatLive) win.liveWebAction("Cut"); else if (win.editTarget === editor) { if (backend.copySelection(editor.selectionStart, editor.selectionEnd, "markdown")) win.performDocumentEdit(function() { editor.remove(editor.selectionStart, editor.selectionEnd); }); } else win.editTarget.cut() }) }
        MenuItem { text: "&Copy"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget.selectedText.length > 0); onTriggered: root.dispatch(function() { if (win.canFormatLive) win.liveWebAction("Copy"); else if (win.editTarget === editor) backend.copySelection(editor.selectionStart, editor.selectionEnd, "markdown"); else win.editTarget.copy() }) }
        MenuItem { objectName: "windows_editCopyFormatted"; text: "Copy Formatted"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget === editor && editor.selectedText.length > 0); onTriggered: root.dispatch(function() { if (win.canFormatLive) backend.liveCopySelection("formatted"); else backend.copySelection(editor.selectionStart, editor.selectionEnd, "formatted"); }) }
        MenuItem { objectName: "windows_editCopyHtml"; text: "Copy HTML"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget === editor && editor.selectedText.length > 0); onTriggered: root.dispatch(function() { if (win.canFormatLive) backend.liveCopySelection("html"); else backend.copySelection(editor.selectionStart, editor.selectionEnd, "html"); }) }
        MenuItem { objectName: "windows_editCopyMarkdown"; text: "Copy Markdown"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget === editor && editor.selectedText.length > 0); onTriggered: root.dispatch(function() { if (win.canFormatLive) backend.liveCopySelection("markdown"); else backend.copySelection(editor.selectionStart, editor.selectionEnd, "markdown"); }) }
        MenuItem { text: "&Paste"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && !win.editTarget.readOnly && win.editTarget.canPaste); onTriggered: root.dispatch(function() { if (win.canFormatLive) win.liveWebAction("Paste"); else if (win.editTarget === editor) editor.pasteClipboardAsPlainText(); else win.editTarget.paste() }) }
        WorkspaceMenu {
            title: "Paste As"
            MenuItem { objectName: "windows_editPastePlain"; text: "Plain Text"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget === editor && editor.canPaste); onTriggered: root.dispatch(function() { if (win.canFormatLive) { backend.liveReplaceSelection(backend.clipboardText()); return; } editor.forceActiveFocus(); editor.replaceAtomic(editor.selectionStart, editor.selectionEnd, backend.clipboardText()); }) }
            MenuItem { objectName: "windows_editPasteMarkdown"; text: "Markdown from HTML"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget === editor && editor.canPaste); onTriggered: root.dispatch(function() { if (win.canFormatLive) { backend.liveReplaceSelection(backend.clipboardMarkdown()); return; } editor.forceActiveFocus(); editor.replaceAtomic(editor.selectionStart, editor.selectionEnd, backend.clipboardMarkdown()); }) }
        }
        MenuItem { objectName: "windows_editDelete"; text: "Delete"; enabled: win.sourceClipboardAllowed && !win.editTarget.readOnly && win.editTarget.selectedText.length > 0; onTriggered: root.dispatch(function() { win.performDocumentEdit(function() { win.editTarget.remove(win.editTarget.selectionStart, win.editTarget.selectionEnd); }); }) }
        MenuItem { text: "Select &All"; onTriggered: root.dispatch(function() { if (win.canFormatLive) win.liveWebAction("SelectAll"); else if (win.sourceClipboardAllowed) win.editTarget.selectAll() }) }
        MenuSeparator {}
        MenuItem { objectName: "windows_editFind"; text: "&Find…"; onTriggered: root.dispatch(function() { win.openSearch(false, false) }) }
        MenuItem { objectName: "windows_editReplace"; text: "&Replace…"; onTriggered: root.dispatch(function() { win.openSearch(true, false) }) }
        MenuItem { objectName: "windows_editFindNext"; text: "Find Next"; enabled: win.searchOpen && win.searchMatches.length > 0; onTriggered: root.dispatch(function() { win.moveSearch(1); }) }
        MenuItem { objectName: "windows_editFindPrevious"; text: "Find Previous"; enabled: win.searchOpen && win.searchMatches.length > 0; onTriggered: root.dispatch(function() { win.moveSearch(-1); }) }
        MenuItem { objectName: "windows_editFindSelection"; text: "Use Selection for Find"; enabled: editor.selectedText.length > 0; onTriggered: root.dispatch(function() { win.openSearch(false, true); }) }
        MenuSeparator {}
        Command { commandId: "spelling" }
        Command { commandId: "nextIssue" }
        Command { commandId: "previousIssue" }
        WorkspaceMenu {
            title: "Writing Checker"
            Command { commandId: "engineHarper" }
            MenuSeparator {}
            Command { commandId: "dialectAustralian" }
            Command { commandId: "dialectBritish" }
            Command { commandId: "dialectAmerican" }
            Command { commandId: "dialectCanadian" }
            MenuSeparator {}
            Command { commandId: "checkSpellingWhileTyping" }
            Command { commandId: "checkGrammarWhileTyping" }
            Command { commandId: "checkStyleWhileTyping" }
        }
    }
    WorkspaceMenu {
        title: "F&ormat"
        MenuItem { text: "&Bold"; enabled: win.canFormatSource || win.canFormatLive; onTriggered: root.dispatch(function() { win.tryWrapSelection("**", "**") }) }
        MenuItem { text: "&Italic"; enabled: win.canFormatSource || win.canFormatLive; onTriggered: root.dispatch(function() { win.tryWrapSelection("*", "*") }) }
        Command { commandId: "strike" }
        Command { commandId: "inlineCode" }
        MenuItem { text: "&Link…"; enabled: win.canFormatSource || win.canFormatLive; onTriggered: root.dispatch(function() { win.tryInsertLink() }) }
        Command { commandId: "pageBreak" }
    }
    WorkspaceMenu {
        title: "&View"
        Command { commandId: "sourceEditing" }
        Command { commandId: "liveEditing" }
        MenuSeparator {}
        Command { commandId: "editor" }
        Command { commandId: "split" }
        Command { commandId: "preview" }
        Command { commandId: "webPreview" }
        Command { commandId: "pdfPreview" }
        Command { commandId: "reloadPreview" }
        MenuSeparator {}
        Command { commandId: "library" }
        Command { commandId: "organizer" }
        Command { commandId: "sortBar" }
        Command { commandId: "filterBar" }
        WorkspaceMenu {
            title: "Sort Files By"
            Command { commandId: "sortName" }
            Command { commandId: "sortModified" }
            Command { commandId: "sortCreated" }
            Command { commandId: "sortExtension" }
            MenuSeparator {}
            Command { commandId: "ascending" }
            Command { commandId: "descending" }
            MenuSeparator {}
            Command { commandId: "foldersFirst" }
        }
        WorkspaceMenu {
            title: "View Options"
            WorkspaceMenu {
                title: "Show Date"
                Command { commandId: "dateModified" }
                Command { commandId: "dateCreated" }
                Command { commandId: "dateNone" }
            }
            Command { commandId: "excerpts" }
            WorkspaceMenu {
                title: "Navigation"
                Command { commandId: "navigationTree" }
                Command { commandId: "navigationList" }
            }
        }
        Command { commandId: "outline" }
        Command { commandId: "statistics" }
        WorkspaceMenu {
            objectName: "windowsTitleBarMenu"
            title: "Title Bar"
            Command { commandId: "titleBarFade" }
            Command { commandId: "titleBarAlways" }
        }
        WorkspaceMenu {
            objectName: "windowsToolbarMenu"
            title: "Toolbar"
            // Menu creates its submenu entry through this delegate. Keep the
            // Stats Only heading checked while its persisted mode is active.
            delegate: MenuItem {
                id: toolbarSubmenuItem
                objectName: subMenu === toolbarStatsOnlyMenu ? "windows_toolbarStatsOnly" : ""
                checkable: subMenu === toolbarStatsOnlyMenu
                checked: subMenu === toolbarStatsOnlyMenu && workspaceCommands.isChecked("toolbarStatsOnly")
                onTriggered: checked = Qt.binding(function() {
                    return toolbarSubmenuItem.subMenu === toolbarStatsOnlyMenu && workspaceCommands.isChecked("toolbarStatsOnly")
                })
            }
            Command { commandId: "toolbarFade" }
            Command { commandId: "toolbarAlways" }
            Command { commandId: "toolbarHide" }
            MenuSeparator {}
            Command { commandId: "toolbarDefault" }
            WorkspaceMenu {
                id: toolbarStatsOnlyMenu
                objectName: "windowsToolbarStatsOnlyMenu"
                title: "Stats Only"
                Command { commandId: "toolbarCharacters" }
                Command { commandId: "toolbarCharactersNoSpaces" }
                Command { commandId: "toolbarWords" }
                Command { commandId: "toolbarSentences" }
                Command { commandId: "toolbarReadingTime" }
                Command { commandId: "toolbarSpeakingTime" }
                Command { commandId: "toolbarTasks" }
                Command { commandId: "toolbarHuman" }
                Command { commandId: "toolbarAI" }
                Command { commandId: "toolbarReference" }
            }
        }
        WorkspaceMenu {
            title: "Writing Appearance"
            Command { commandId: "writingManuscript" }
            Command { commandId: "writingEditorial" }
            Command { commandId: "writingBook" }
            Command { commandId: "writingCode" }
            Command { commandId: "writingOmawrite" }
        }
        WorkspaceMenu {
            title: "Appearance"
            Command { commandId: "themeSystem" }
            Command { commandId: "themeLight" }
            Command { commandId: "themeDark" }
            Command { commandId: "appearanceFollowsOutputStyle" }
        }
        Command { commandId: "larger" }
        Command { commandId: "smaller" }
        Command { commandId: "resetSize" }
        Command { commandId: "fullscreen" }
    }
    WorkspaceMenu {
        title: "F&ocus"
        WorkspaceMenu {
            objectName: "windowsFocusModeMenu"
            title: "Enable Focus Mode"
            Command { commandId: "sentence"; text: "Sentence" }
            Command { commandId: "paragraph"; text: "Paragraph" }
            Command { commandId: "typewriter"; text: "Typewriter (Source)" }
        }
        WorkspaceMenu {
            objectName: "windowsFocusStyleCheckMenu"
            title: "Enable Style Check"
            Command { commandId: "fillersStyleCheck" }
            Command { commandId: "customStyleCheck" }
        }
        MenuSeparator {}
        Command { commandId: "writingReview" }
    }
    WorkspaceMenu {
        title: "&Help"
        MenuItem { text: "Fomawrite Help"; onTriggered: root.dispatch(function() { helpDialog.showPage("help", "Fomawrite Help") }) }
        MenuItem { text: "Keyboard Shortcuts"; onTriggered: root.dispatch(function() { shortcutsDialog.open() }) }
        MenuSeparator {}
        Command { commandId: "checkWritingCheckerUpdates" }
        Command { commandId: "useBuiltInWritingChecker" }
        Command { commandId: "checkWritingCheckerAutomatically" }
        MenuSeparator {}
        MenuItem { text: "About Fomawrite"; onTriggered: root.dispatch(function() { aboutDialog.open() }) }
    }
}
