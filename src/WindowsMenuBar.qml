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
        // A closing Material popup blocks WindowShortcut until its exit ends.
        // Release that boundary before the next writing shortcut arrives.
        exit: null
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
    component RecentMenu: WorkspaceMenu {
        id: recents
        Instantiator {
            model: backend.library.recentFiles
            delegate: MenuItem {
                required property var modelData
                required property int index
                objectName: "recentFile_" + index
                text: modelData.name + " — " + modelData.url.toString().replace(/^file:\/\//, "")
                    + (modelData.available ? "" : " (Unavailable)")
                enabled: modelData.available
                onTriggered: {
                    // Opening can rebuild the model; retain the URL before dispatch.
                    var file = modelData.url;
                    root.dispatch(function() { win.requestOpen(file) })
                }
            }
            onObjectAdded: function(index, object) { recents.insertItem(index, object); }
            onObjectRemoved: function(index, object) { recents.removeItem(object); }
        }
        MenuItem { text: "No Recent Files"; enabled: false; visible: backend.library.recentFiles.length === 0 }
        MenuSeparator { visible: backend.library.recentFiles.length > 0 }
        MenuItem { text: "Clear Recent Files"; enabled: backend.library.recentFiles.length > 0; onTriggered: root.dispatch(function() { backend.library.clearRecentFiles() }) }
    }
    WorkspaceMenu {
        title: "&File"
        Command { commandId: "new" }
        Command { commandId: "newWindow" }
        Command { commandId: "open" }
        Command { commandId: "openPath" }
        RecentMenu { objectName: "windowsOpenRecentMenu"; title: "Open Recent" }
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
        WorkspaceMenu {
            title: "Substitutions"
            MenuItem {
                objectName: "windows_editSmartQuotes"
                text: "Smart Quotes"
                checkable: true
                checked: workspaceSettings.smartQuotes
                onTriggered: {
                    root.dispatch(function() { workspaceSettings.smartQuotes = !workspaceSettings.smartQuotes })
                    checked = Qt.binding(function() { return workspaceSettings.smartQuotes })
                }
            }
            MenuItem {
                objectName: "windows_editSmartDashes"
                text: "Smart Dashes"
                checkable: true
                checked: workspaceSettings.smartDashes
                onTriggered: {
                    root.dispatch(function() { workspaceSettings.smartDashes = !workspaceSettings.smartDashes })
                    checked = Qt.binding(function() { return workspaceSettings.smartDashes })
                }
            }
        }
        WorkspaceMenu {
            objectName: "windows_editTransformations"
            title: "Transformations"
            MenuItem { objectName: "windows_editUppercase"; text: "Make Upper Case"; enabled: win.canFormatSource && editor.selectedText.length > 0; onTriggered: root.dispatch(function() { win.editMarkdown("uppercase") }) }
            MenuItem { objectName: "windows_editLowercase"; text: "Make Lower Case"; enabled: win.canFormatSource && editor.selectedText.length > 0; onTriggered: root.dispatch(function() { win.editMarkdown("lowercase") }) }
            MenuItem { objectName: "windows_editCapitalize"; text: "Capitalize"; enabled: win.canFormatSource && editor.selectedText.length > 0; onTriggered: root.dispatch(function() { win.editMarkdown("capitalize") }) }
            MenuItem { objectName: "windows_editTitleCase"; text: "Make Title Case"; enabled: win.canFormatSource && editor.selectedText.length > 0; onTriggered: root.dispatch(function() { win.editMarkdown("titlecase") }) }
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
        Command { commandId: "liveThemeExact" }
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
            id: outputStyleMenu
            objectName: "windowsTemplateMenu"
            title: "Output Style"
            onAboutToShow: win.restorePublishingMenuChecks(outputStyleMenu, false)
            component PublishingChoice: MenuItem {
                id: choice
                property string publishingId: ""
                property int basicStyle: -1
                readonly property bool backendSelected: publishingId.length > 0
                    ? backend.publishingThemeId === publishingId
                    : backend.publishingThemeId.length === 0 && backend.outputStyle === basicStyle
                checkable: true
                checked: backendSelected
                function restoreSelectionCheck() { checked = Qt.binding(function() { return choice.backendSelected; }); }
                onTriggered: {
                    root.dispatch(function() { win.choosePublishingStyle(publishingId, basicStyle) })
                    // MenuItem toggles first, even for repeated or rejected choices.
                    restoreSelectionCheck();
                }
            }
            WorkspaceMenu {
                id: customThemesMenu
                objectName: "windowsCustomThemesMenu"
                title: "Custom Themes"
                onAboutToShow: win.restorePublishingMenuChecks(customThemesMenu, false)
                Instantiator {
                    model: backend.publishingThemes
                    delegate: PublishingChoice {
                        required property var modelData
                        objectName: "windowsPublishingTheme_" + modelData.id
                        text: modelData.name
                        publishingId: modelData.id
                    }
                    onObjectAdded: function(index, object) { customThemesMenu.insertItem(index, object); }
                    onObjectRemoved: function(index, object) { customThemesMenu.removeItem(object); }
                }
            }
            MenuSeparator {}
            MenuItem { text: "Basic Font & Page Settings"; enabled: false }
            PublishingChoice { text: "Modern (Sans)"; objectName: "windowsTemplate0"; basicStyle: 0 }
            PublishingChoice { text: "Classic (Serif)"; objectName: "windowsTemplate1"; basicStyle: 1 }
            PublishingChoice { text: "Manuscript (Mono)"; objectName: "windowsTemplate2"; basicStyle: 2 }
            MenuSeparator {}
            PublishingChoice { text: "GitHub"; objectName: "windowsTemplate4"; basicStyle: 4 }
            PublishingChoice { text: "Helvetica"; objectName: "windowsTemplate5"; basicStyle: 5 }
            PublishingChoice { text: "Palatino"; objectName: "windowsTemplate6"; basicStyle: 6 }
            PublishingChoice { text: "MLA Draft"; objectName: "windowsTemplate7"; basicStyle: 7 }
            MenuSeparator {}
            PublishingChoice { text: "Custom Settings"; objectName: "windowsTemplate3"; basicStyle: 3 }
            MenuItem { objectName: "windowsTemplateCustomEdit"; text: "Edit Basic Settings…"; onTriggered: root.dispatch(function() { win.openExportHub("pdf") }) }
            MenuItem { objectName: "windowsTemplateCustomLoad"; text: "Load Custom Settings…"; onTriggered: root.dispatch(function() { outputStyleDialog.open() }) }
            MenuSeparator {}
            MenuItem { objectName: "windowsGetMoreThemes"; text: "Get More Themes…"; onTriggered: root.dispatch(function() { Qt.openUrlExternally("https://theme.typora.io/") }) }
            MenuItem { objectName: "windowsThemeImport"; text: "Import Theme…"; onTriggered: root.dispatch(function() { publishingThemeDialog.open() }) }
            MenuItem { objectName: "windowsThemesFolder"; text: "Open Themes Folder"; onTriggered: root.dispatch(function() { win.openPublishingThemesFolder() }) }
            MenuItem { objectName: "windowsThemesReload"; text: "Reload Themes"; onTriggered: root.dispatch(function() { win.reloadPublishingThemes() }) }
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
        objectName: "windowsGoMenu"
        title: "&Go"
        MenuItem { objectName: "goBack"; text: "Back"; enabled: backend.canGoBack; onTriggered: root.dispatch(function() { win.requestHistory(-1) }) }
        MenuItem { objectName: "goForward"; text: "Forward"; enabled: backend.canGoForward; onTriggered: root.dispatch(function() { win.requestHistory(1) }) }
        MenuSeparator {}
        MenuItem { objectName: "goLibraryBack"; text: "Back in Library"; enabled: backend.library.canGoBack; onTriggered: root.dispatch(function() { backend.library.navigateHistory(-1) }) }
        MenuItem { objectName: "goLibraryForward"; text: "Forward in Library"; enabled: backend.library.canGoForward; onTriggered: root.dispatch(function() { backend.library.navigateHistory(1) }) }
        MenuItem { objectName: "goEnclosingFolder"; text: "Enclosing Folder"; enabled: backend.library.rootFolder.toString() !== ""; onTriggered: root.dispatch(function() { backend.library.enclosingFolder() }) }
        MenuSeparator {}
        MenuItem { objectName: "goOpenLink"; text: "Open Link"; enabled: backend.sourceLinkAt(editor.cursorPosition).toString() !== ""; onTriggered: root.dispatch(function() { win.openSourceLink() }) }
        Command { commandId: "quickOpen"; objectName: "goQuickSearch"; text: "Quick Search…" }
        MenuItem { objectName: "goCommandPalette"; text: "Command Palette…"; onTriggered: root.dispatch(function() { commandPalette.open() }) }
        MenuSeparator {}
        WorkspaceMenu {
            id: locationsMenu
            objectName: "windowsLocationsMenu"
            title: "Locations"
            Instantiator {
                model: backend.library.locations
                delegate: MenuItem {
                    required property var modelData
                    required property int index
                    objectName: "goLocation_" + index
                    text: modelData.name + (modelData.available ? "" : " (Unavailable)")
                    enabled: modelData.available
                    onTriggered: root.dispatch(function() { backend.library.rootFolder = modelData.url; workspaceLayout.filesVisible = true; })
                }
                onObjectAdded: function(index, object) { locationsMenu.insertItem(index, object); }
                onObjectRemoved: function(index, object) { locationsMenu.removeItem(object); }
            }
            MenuSeparator {}
            MenuItem { objectName: "goAddLocation"; text: "Add Location…"; onTriggered: root.dispatch(function() { libraryPane.chooseFolder() }) }
        }
        WorkspaceMenu {
            id: smartFoldersMenu
            objectName: "windowsSmartFoldersMenu"
            title: "Smart Folders"
            RecentMenu { objectName: "windowsSmartFolderRecentsMenu"; title: "Recents" }
            MenuItem { objectName: "goNewSmartFolder"; text: "New Smart Folder…"; enabled: backend.library.rootFolder.toString() !== ""; onTriggered: root.dispatch(function() { win.openQuickSearchState("", false, backend.library.rootFolder, true) }) }
            MenuSeparator { visible: backend.library.savedSearches.length > 0 }
            Instantiator {
                model: backend.library.savedSearches
                delegate: MenuItem {
                    required property var modelData
                    required property int index
                    objectName: "goSavedSearch_" + index
                    text: modelData.query + (modelData.contents ? " — contents" : " — filenames")
                    onTriggered: root.dispatch(function() { win.openSavedSearch(modelData) })
                }
                onObjectAdded: function(index, object) { smartFoldersMenu.insertItem(index + 3, object); }
                onObjectRemoved: function(index, object) { smartFoldersMenu.removeItem(object); }
            }
        }
        WorkspaceMenu {
            id: hashtagsMenu
            objectName: "windowsHashtagsMenu"
            title: "Hashtags"
            Instantiator {
                model: backend.library.tagIndex
                delegate: MenuItem {
                    required property var modelData
                    required property int index
                    objectName: "goHashtag_" + modelData.tag
                    text: "#" + modelData.tag + " (" + modelData.count + ")"
                    enabled: backend.library.rootFolder.toString() !== ""
                    onTriggered: root.dispatch(function() { win.openHashtag(modelData.tag) })
                }
                onObjectAdded: function(index, object) { hashtagsMenu.insertItem(index, object); }
                onObjectRemoved: function(index, object) { hashtagsMenu.removeItem(object); }
            }
            MenuItem { objectName: "goHashtagsEmpty"; text: backend.library.tagStatus; enabled: false; visible: backend.library.tagIndex.length === 0 }
            MenuSeparator {}
            MenuItem { objectName: "goHashtagStatus"; text: backend.library.tagStatus; enabled: false; visible: backend.library.tagIndex.length > 0 }
            Command { commandId: "refreshTags"; objectName: "goRefreshHashtags"; text: "Refresh Hashtags" }
        }
    }
    WorkspaceMenu {
        title: "&Help"
        MenuItem { text: "Fomawrite Help"; onTriggered: root.dispatch(function() { helpDialog.showPage("help", "Fomawrite Help") }) }
        MenuItem { objectName: "windows_helpWhatsNew"; text: "What’s New in Fomawrite"; onTriggered: root.dispatch(function() { helpDialog.showPage("whats-new", "What’s New in Fomawrite") }) }
        MenuItem { text: "Keyboard Shortcuts"; onTriggered: root.dispatch(function() { shortcutsDialog.open() }) }
        MenuSeparator {}
        Command { commandId: "checkWritingCheckerUpdates" }
        Command { commandId: "useBuiltInWritingChecker" }
        Command { commandId: "checkWritingCheckerAutomatically" }
        MenuSeparator {}
        MenuItem { text: "About Fomawrite"; onTriggered: root.dispatch(function() { aboutDialog.open() }) }
    }
}
