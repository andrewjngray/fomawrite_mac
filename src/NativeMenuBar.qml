// The native macOS menu bar. Extracted verbatim from Main.qml (Cycle 133): it
// is instantiated from Main.qml's Loader, so it resolves `win`, `backend`,
// `commands`, the dialogs and the panes through that instantiation context,
// exactly as it did inline. It contains no logic of its own.
import QtQuick
import QtQuick.Controls
import Qt.labs.platform as Platform

Platform.MenuBar {
    // Inline components are file-scoped; these two are used only by the native menus.
    component NativeCommand: NativeCommandMenuItem { commands: workspaceCommands }
    component NativePublishingMenuItem: Platform.MenuItem {
        id: choice
        property string publishingId: ""
        property int basicStyle: -1
        readonly property bool backendSelected: publishingId.length > 0
            ? backend.publishingThemeId === publishingId
            : backend.publishingThemeId.length === 0 && backend.outputStyle === basicStyle
        checkable: true
        checked: backendSelected
        function restoreSelectionCheck() { checked = Qt.binding(function() { return choice.backendSelected; }); }
        onTriggered: win.choosePublishingStyle(publishingId, basicStyle)
    }
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
            Platform.MenuItem { objectName: "fileExportHtml"; text: "Export HTML…"; onTriggered: win.openExportHub("html") }
            Platform.MenuItem { objectName: "fileExportPdf"; text: "Export PDF…"; onTriggered: win.openExportHub("pdf") }
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
            title: "Fomawrite Extras"
            Platform.MenuItem { objectName: "fileNewWindow"; text: "New Window"; onTriggered: backend.newWindow() }
            Platform.MenuItem { objectName: "fileNewFolder"; text: "New Folder…"; enabled: backend.library.rootFolder.toString() !== ""; onTriggered: libraryPane.newFolder() }
            Platform.MenuItem { objectName: "fileOpenPath"; text: "Open by Path…"; shortcut: "Ctrl+Shift+O"; onTriggered: openPathDialog.open() }
            Platform.MenuItem { objectName: "fileSaveAs"; text: "Save As…"; onTriggered: backend.saveAsDialog() }
            Platform.MenuSeparator {}
            Platform.MenuItem { objectName: "fileAutosave"; text: "Autosave Saved Files Every Minute"; checkable: true; checked: workspaceSettings.autosaveEnabled; onTriggered: workspaceSettings.autosaveEnabled = !workspaceSettings.autosaveEnabled }
        }
        Platform.MenuSeparator {}
        Platform.MenuItem {
            text: "Quit Fomawrite"
            role: Platform.MenuItem.QuitRole
            onTriggered: backend.requestQuit()
        }
    }
    Platform.Menu {
        title: "Edit"
        Platform.MenuItem { objectName: "editUndo"; text: "Undo"; enabled: win.canFormatLive || win.editTarget.canUndo; onTriggered: win.performDocumentEdit(function() { win.undoEditing(); }) }
        Platform.MenuItem { objectName: "editRedo"; text: "Redo"; enabled: win.canFormatLive || win.editTarget.canRedo; onTriggered: win.performDocumentEdit(function() { win.redoEditing(); }) }
        Platform.MenuSeparator {}
        Platform.MenuItem { text: "Cut"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && !win.editTarget.readOnly && win.editTarget.selectedText.length > 0); onTriggered: { if (win.canFormatLive) win.liveWebAction("Cut"); else if (win.editTarget === editor) { backend.copySelection(editor.selectionStart, editor.selectionEnd, "markdown"); win.performDocumentEdit(function() { editor.remove(editor.selectionStart, editor.selectionEnd); }); } else win.editTarget.cut(); } }
        Platform.MenuItem { text: "Copy"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget.selectedText.length > 0); onTriggered: { if (win.canFormatLive) win.liveWebAction("Copy"); else if (win.editTarget === editor) backend.copySelection(editor.selectionStart, editor.selectionEnd, "markdown"); else win.editTarget.copy(); } }
        Platform.MenuItem { objectName: "editCopyFormatted"; text: "Copy Formatted"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget === editor && editor.selectedText.length > 0); onTriggered: { if (win.canFormatLive) backend.liveCopySelection("formatted"); else backend.copySelection(editor.selectionStart, editor.selectionEnd, "formatted"); } }
        Platform.MenuItem { objectName: "editCopyHtml"; text: "Copy HTML"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget === editor && editor.selectedText.length > 0); onTriggered: { if (win.canFormatLive) backend.liveCopySelection("html"); else backend.copySelection(editor.selectionStart, editor.selectionEnd, "html"); } }
        Platform.MenuItem { objectName: "editCopyMarkdown"; text: "Copy Markdown"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget === editor && editor.selectedText.length > 0); onTriggered: { if (win.canFormatLive) backend.liveCopySelection("markdown"); else backend.copySelection(editor.selectionStart, editor.selectionEnd, "markdown"); } }
        Platform.MenuItem { text: "Paste"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && !win.editTarget.readOnly && win.editTarget.canPaste); onTriggered: { if (win.canFormatLive) win.liveWebAction("Paste"); else if (win.editTarget === editor) editor.pasteClipboardAsPlainText(); else win.editTarget.paste(); } }
        Platform.Menu {
            title: "Paste As"
            Platform.MenuItem { objectName: "editPastePlain"; text: "Plain Text"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget === editor && editor.canPaste); onTriggered: { if (win.canFormatLive) { backend.liveReplaceSelection(backend.clipboardText()); return; } editor.forceActiveFocus(); editor.replaceAtomic(editor.selectionStart, editor.selectionEnd, backend.clipboardText()); } }
            Platform.MenuItem { objectName: "editPasteMarkdown"; text: "Markdown from HTML"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget === editor && editor.canPaste); onTriggered: { if (win.canFormatLive) { backend.liveReplaceSelection(backend.clipboardMarkdown()); return; } editor.forceActiveFocus(); editor.replaceAtomic(editor.selectionStart, editor.selectionEnd, backend.clipboardMarkdown()); } }
        }
        Platform.MenuSeparator {}
        Platform.MenuItem { objectName: "editDelete"; text: "Delete"; enabled: win.sourceClipboardAllowed && !win.editTarget.readOnly && win.editTarget.selectedText.length > 0; onTriggered: win.performDocumentEdit(function() { win.editTarget.remove(win.editTarget.selectionStart, win.editTarget.selectionEnd); }) }
        Platform.MenuItem { text: "Select All"; enabled: win.canFormatLive || (win.sourceClipboardAllowed && win.editTarget.length > 0); onTriggered: { if (win.canFormatLive) win.liveWebAction("SelectAll"); else win.editTarget.selectAll(); } }
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
            title: "Substitutions"
            Platform.MenuItem {
                objectName: "editSmartQuotes"
                text: "Smart Quotes"
                checkable: true
                checked: workspaceSettings.smartQuotes
                onTriggered: workspaceSettings.smartQuotes = !workspaceSettings.smartQuotes
            }
            Platform.MenuItem {
                objectName: "editSmartDashes"
                text: "Smart Dashes"
                checkable: true
                checked: workspaceSettings.smartDashes
                onTriggered: workspaceSettings.smartDashes = !workspaceSettings.smartDashes
            }
        }
        Platform.Menu {
            objectName: "editTransformations"
            title: "Transformations"
            Platform.MenuItem { objectName: "editUppercase"; text: "Make Upper Case"; enabled: win.canFormatSource && editor.selectedText.length > 0; onTriggered: win.editMarkdown("uppercase") }
            Platform.MenuItem { objectName: "editLowercase"; text: "Make Lower Case"; enabled: win.canFormatSource && editor.selectedText.length > 0; onTriggered: win.editMarkdown("lowercase") }
            Platform.MenuItem { objectName: "editCapitalize"; text: "Capitalize"; enabled: win.canFormatSource && editor.selectedText.length > 0; onTriggered: win.editMarkdown("capitalize") }
            Platform.MenuItem { objectName: "editTitleCase"; text: "Make Title Case"; enabled: win.canFormatSource && editor.selectedText.length > 0; onTriggered: win.editMarkdown("titlecase") }
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
        objectName: "nativeFormatMenu"
        title: "Format"
        enabled: win.canFormatSource
        Platform.Menu {
            title: "Headings"
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Heading 1"; onTriggered: win.editMarkdown("heading1") }
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Heading 2"; onTriggered: win.editMarkdown("heading2") }
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Heading 3"; onTriggered: win.editMarkdown("heading3") }
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Heading 4"; onTriggered: win.editMarkdown("heading4") }
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Heading 5"; onTriggered: win.editMarkdown("heading5") }
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Heading 6"; onTriggered: win.editMarkdown("heading6") }
        }
        Platform.Menu {
            title: "Lists"
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "List"; onTriggered: win.editMarkdown("bullet") }
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Task List"; onTriggered: win.editMarkdown("task") }
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Ordered List"; onTriggered: win.editMarkdown("ordered") }
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Ordered Task List"; onTriggered: win.editMarkdown("orderedTask") }
            Platform.MenuSeparator {}
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Mark Task as Completed"; onTriggered: win.editMarkdown("toggleTask") }
        }
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Blockquote"; onTriggered: win.editMarkdown("quote") }
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Body"; onTriggered: win.editMarkdown("body") }
        Platform.Menu {
            title: "Structure"
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Indent"; onTriggered: win.editMarkdown("indent") }
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Outdent"; onTriggered: win.editMarkdown("outdent") }
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Move Line Up"; onTriggered: win.editMarkdown("lineUp") }
            Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Move Line Down"; onTriggered: win.editMarkdown("lineDown") }
        }
        Platform.MenuSeparator {}
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Bold"; onTriggered: win.tryWrapSelection("**", "**") }
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Italic"; onTriggered: win.tryWrapSelection("*", "*") }
        NativeCommand { commandId: "strike" }
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Highlight"; onTriggered: win.tryWrapSelection("==", "==") }
        Platform.MenuSeparator {}
        NativeCommand { commandId: "inlineCode"; text: "Code" }
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Code Block"; onTriggered: win.editMarkdown("codeBlock") }
        Platform.MenuSeparator {}
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Add Link"; onTriggered: win.tryInsertLink() }
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Add Wikilink"; onTriggered: win.tryWrapSelection("[[", "]]") }
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Add Footnote"; onTriggered: win.tryInsertSourceSnippet("[^note]\n\n[^note]: Note text") }
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Add Content Block"; onTriggered: win.tryInsertSourceSnippet("\n/chapter.md\n") }
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Add Hashtag"; onTriggered: win.tryInsertSourceSnippet("#tag") }
        Platform.MenuSeparator {}
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Add Date"; onTriggered: win.editMarkdown("date") }
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Add Table"; onTriggered: win.editMarkdown("table") }
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Add Table of Contents"; onTriggered: win.tryInsertSourceSnippet(backend.tableOfContents(editor.text)) }
        Platform.MenuSeparator {}
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Add Horizontal Rule"; onTriggered: win.editMarkdown("rule") }
        Platform.MenuItem { enabled: win.canFormatSource || win.canFormatLive; text: "Add Page Break"; onTriggered: win.tryInsertSourceSnippet("\n\n<!-- pagebreak -->\n\n") }
        Platform.MenuSeparator {}
        Platform.MenuItem { enabled: win.canFormatSource && editor.selectedText.length > 0; text: "Clear Styles"; onTriggered: win.editMarkdown("clearStyles") }
    }
    Platform.Menu {
        objectName: "authorsMenu"
        title: "Authors"
        visible: win.isMac
        Platform.MenuItem {
            objectName: "authorsSetupAction"
            text: "Set Up Authorship…"
            onTriggered: authorshipSetupDialog.open()
        }
    }
    Platform.Menu {
        objectName: "nativeViewMenu"
        title: "View"
        NativeCommand { commandId: "library" }
        NativeCommand { commandId: "organizer" }
        Platform.MenuSeparator {}
        Platform.MenuItem {
            objectName: "nativeAutoHideDocumentChrome"
            text: "Auto-Hide Document Bars"
            checkable: true
            checked: workspaceSettings.autoHideChrome
            onTriggered: workspaceSettings.autoHideChrome = !workspaceSettings.autoHideChrome
        }

        Platform.MenuItem { text: "Synchronized Scrolling"; checkable: true; checked: workspaceSettings.synchronizedScroll; onTriggered: workspaceSettings.synchronizedScroll = !workspaceSettings.synchronizedScroll }
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
        Platform.Menu {
            title: "Writing Appearance"
            NativeCommand { commandId: "writingManuscript"; text: "Manuscript (Mono)" }
            NativeCommand { commandId: "writingEditorial"; text: "Editorial (Sans)" }
            NativeCommand { commandId: "writingBook"; text: "Book (Serif)" }
            NativeCommand { commandId: "writingCode"; text: "Code" }
        }
        Platform.MenuItem {
            objectName: "native_showCompletions"
            text: "Show Completions"
            enabled: win.sourceEditorVisible && win.editTarget === editor && editor.selectionStart === editor.selectionEnd
            onTriggered: win.showCompletions()
        }
        Platform.MenuSeparator { objectName: "nativeBeforeTemplate" }
        Platform.Menu {
            id: nativeTemplateMenu
            Component.onCompleted: win.nativePublishingMenu = nativeTemplateMenu
            objectName: "nativeTemplateMenu"
            title: "Output Style"
            Binding { target: nativeTemplateMenu.menuItem; property: "objectName"; value: "nativeTemplateEntry" }
            onAboutToShow: win.restorePublishingChecks()
            Platform.Menu {
                id: nativeCustomThemesMenu
                objectName: "nativeCustomThemesMenu"
                title: "Custom Themes"
                onAboutToShow: win.restorePublishingChecks()
                Instantiator {
                    model: backend.publishingThemes
                    delegate: NativePublishingMenuItem {
                        required property var modelData
                        objectName: "nativePublishingTheme_" + modelData.id
                        text: modelData.id === "claude-like" ? "Claude Like" : modelData.name
                        publishingId: modelData.id
                    }
                    onObjectAdded: function(index, object) { nativeCustomThemesMenu.insertItem(index, object); }
                    onObjectRemoved: function(index, object) { nativeCustomThemesMenu.removeItem(object); }
                }
            }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: "Basic Font & Page Settings"; enabled: false }
            NativePublishingMenuItem { text: "Modern (Sans)"; objectName: "nativeTemplate0"; basicStyle: 0 }
            NativePublishingMenuItem { text: "Classic (Serif)"; objectName: "nativeTemplate1"; basicStyle: 1 }
            NativePublishingMenuItem { text: "Manuscript (Mono)"; objectName: "nativeTemplate2"; basicStyle: 2 }
            Platform.MenuSeparator {}
            NativePublishingMenuItem { text: "GitHub"; objectName: "nativeTemplate4"; basicStyle: 4 }
            NativePublishingMenuItem { text: "Helvetica"; objectName: "nativeTemplate5"; basicStyle: 5 }
            NativePublishingMenuItem { text: "Palatino"; objectName: "nativeTemplate6"; basicStyle: 6 }
            NativePublishingMenuItem { text: "MLA Draft"; objectName: "nativeTemplate7"; basicStyle: 7 }
            Platform.MenuSeparator {}
            NativePublishingMenuItem { text: "Custom Settings"; objectName: "nativeTemplate3"; basicStyle: 3 }
            Platform.MenuItem { objectName: "nativeTemplateCustomEdit"; text: "Edit Basic Settings…"; onTriggered: win.openExportHub("pdf") }
            Platform.MenuItem { objectName: "nativeTemplateCustomLoad"; text: "Load Custom Settings…"; onTriggered: outputStyleDialog.open() }
            Platform.MenuSeparator {}
            Platform.MenuItem { objectName: "nativeGetMoreThemes"; text: "Get More Themes…"; onTriggered: Qt.openUrlExternally("https://theme.typora.io/") }
            Platform.MenuItem { objectName: "nativeThemeImport"; text: "Import Theme…"; onTriggered: publishingThemeDialog.open() }
            Platform.MenuItem { objectName: "nativeThemesFolder"; text: "Open Themes Folder"; onTriggered: win.openPublishingThemesFolder() }
            Platform.MenuItem { objectName: "nativeThemesReload"; text: "Reload Themes"; onTriggered: win.reloadPublishingThemes() }
        }
        Platform.MenuSeparator { objectName: "nativeAfterTemplate" }
        Platform.Menu {
            title: "Editing"
            NativeCommand { commandId: "sourceEditing" }
            NativeCommand { commandId: "liveEditing" }
            Platform.MenuSeparator {}
            NativeCommand { commandId: "liveThemeExact"; enabled: workspaceLayout.liveEditEnabled }
        }
        Platform.Menu {
            title: "Layout"
            NativeCommand { commandId: "editor" }
            NativeCommand { commandId: "split" }
        }
        Platform.MenuSeparator {}
        NativeCommand { commandId: "preview" }
        NativeCommand { commandId: "webPreview"; text: "Web Preview" }
        NativeCommand { commandId: "reloadPreview" }
        Platform.MenuItem { objectName: "native_pdfPreview"; text: "Paginated Preview…"; onTriggered: backend.printPreview() }
        Platform.MenuSeparator {}
        NativeCommand { commandId: "outline" }
        NativeCommand { commandId: "statistics" }
        Platform.Menu {
            objectName: "nativeTitleBarMenu"
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
        Platform.MenuItem {
            objectName: "windowCenterAction"
            text: "Center"
            enabled: win.visibility === Window.Windowed
            onTriggered: backend.nativeWindowAction("center")
        }
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
        Platform.Menu {
            objectName: "focusModeMenu"
            title: "Enable Focus Mode"
            NativeCommand { commandId: "sentence"; text: "Sentence" }
            NativeCommand { commandId: "paragraph"; text: "Paragraph" }
            NativeCommand { commandId: "typewriter"; text: "Typewriter (Source)" }
        }
        Platform.Menu {
            objectName: "focusStyleCheckMenu"
            title: "Enable Style Check"
            NativeCommand { commandId: "fillersStyleCheck"; text: "Fillers" }
            NativeCommand { commandId: "customStyleCheck"; text: "Custom" }
        }
        Platform.MenuSeparator {}
        Platform.MenuItem { objectName: "focusWritingReview"; text: "Writing Review…"; onTriggered: analysisDialog.open() }
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
                    onTriggered: { backend.library.rootFolder = modelData.url; workspaceLayout.filesVisible = true; }
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
        Platform.MenuItem {
            objectName: "helpAboutFomawrite"
            text: "About Fomawrite"
            role: Platform.MenuItem.AboutRole
            onTriggered: aboutDialog.open()
        }
        Platform.MenuSeparator {}
        Platform.MenuItem {
            objectName: "helpFomawrite"
            text: "Fomawrite Help"
            onTriggered: helpDialog.showPage("help", "Fomawrite Help")
        }
        Platform.MenuItem {
            objectName: "helpWhatsNew"
            text: "What’s New in Fomawrite"
            onTriggered: helpDialog.showPage("whats-new", "What’s New in Fomawrite")
        }
        Platform.MenuSeparator {}
        Platform.MenuItem { objectName: "helpKeyboardShortcuts"; text: "Keyboard Shortcuts"; onTriggered: shortcutsDialog.open() }
    }
}
