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
        MenuItem { text: "&Paste"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && !win.editTarget.readOnly && win.editTarget.canPaste); onTriggered: root.dispatch(function() { if (win.canFormatLive) win.liveWebAction("Paste"); else if (win.editTarget === editor) editor.pasteClipboardAsPlainText(); else win.editTarget.paste() }) }
        MenuItem { text: "Select &All"; onTriggered: root.dispatch(function() { if (win.canFormatLive) win.liveWebAction("SelectAll"); else if (win.sourceClipboardAllowed) win.editTarget.selectAll() }) }
        MenuSeparator {}
        MenuItem { text: "&Find…"; onTriggered: root.dispatch(function() { win.openSearch(false, false) }) }
        MenuItem { text: "&Replace…"; onTriggered: root.dispatch(function() { win.openSearch(true, false) }) }
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
        Command { commandId: "outline" }
        Command { commandId: "statistics" }
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
        title: "&Help"
        MenuItem { text: "Fomawrite Help"; onTriggered: root.dispatch(function() { helpDialog.showPage("help", "Fomawrite Help") }) }
        MenuItem { text: "Keyboard Shortcuts"; onTriggered: root.dispatch(function() { shortcutsDialog.open() }) }
        MenuSeparator {}
        MenuItem { text: "About Fomawrite"; onTriggered: root.dispatch(function() { aboutDialog.open() }) }
    }
}
