import QtQuick
import QtQuick.Window

// One command surface for native menus, workspace controls and a future palette.
// Settings and the library remain the owners of persisted state.
QtObject {
    id: root
    objectName: "workspaceCommands"
    required property var settings
    required property var library
    required property var libraryPane
    required property var window
    required property var editor
    required property var preview
    signal outlineRequested()
    signal statisticsRequested()
    signal typewriterChanged()
    signal commandRequested(string commandId)

    readonly property var entries: [
        { id: "new", title: "New Document" },
        { id: "newWindow", title: "New Window" },
        { id: "open", title: "Open File…" },
        { id: "openPath", title: "Open by Path…" },
        { id: "save", title: "Save" },
        { id: "saveAs", title: "Save As…" },
        { id: "duplicate", title: "Duplicate…" },
        { id: "rename", title: "Rename…" },
        { id: "move", title: "Move To…" },
        { id: "reveal", title: "Show in Library" },
        { id: "quickOpen", title: "Quick Open…" },
        { id: "refreshTags", title: "Refresh Tags" },
        { id: "exportHtml", title: "Export HTML…" },
        { id: "printPreview", title: "Paginated Preview…" },
        { id: "exportPdf", title: "Export PDF…" },
        { id: "pageBreak", title: "Insert Page Break" },
        { id: "writingReview", title: "Writing Review…" },
        { id: "spelling", title: "Spelling and Grammar…" },
        { id: "authorship", title: "Authorship Annotations…" },
        { id: "themeSystem", title: "Theme: Follow System" },
        { id: "themeLight", title: "Theme: Light" },
        { id: "themeDark", title: "Theme: Dark" },
        { id: "themePaper", title: "Theme: Warm Paper" },
        { id: "library", title: "Library", toggle: true },
        { id: "organizer", title: "Organizer", toggle: true },
        { id: "sortBar", title: "Sort Bar", toggle: true },
        { id: "filterBar", title: "Filter Bar", toggle: true },
        { id: "sortName", title: "Name", toggle: true },
        { id: "sortModified", title: "Date Modified", toggle: true },
        { id: "sortCreated", title: "Date Created", toggle: true },
        { id: "sortExtension", title: "Extension", toggle: true },
        { id: "ascending", title: "A to Z", toggle: true },
        { id: "descending", title: "Z to A", toggle: true },
        { id: "foldersFirst", title: "Pin Folders to Top", toggle: true },
        { id: "dates", title: "Show Date", toggle: true },
        { id: "excerpts", title: "Show Text Excerpts", toggle: true },
        { id: "larger", title: "Larger Text" },
        { id: "smaller", title: "Smaller Text" },
        { id: "resetSize", title: "Reset Text Size" },
        { id: "editor", title: "Editor Only", toggle: true },
        { id: "split", title: "Editor and Preview", toggle: true },
        { id: "preview", title: "Preview Only", toggle: true },
        { id: "togglePreview", title: "Preview", toggle: true },
        { id: "reloadPreview", title: "Reload Preview" },
        { id: "sans", title: "Sans", toggle: true },
        { id: "serif", title: "Serif", toggle: true },
        { id: "mono", title: "Mono", toggle: true },
        { id: "markup", title: "Show Markdown Syntax", toggle: true },
        { id: "outline", title: "Document Outline" },
        { id: "statistics", title: "Document Statistics" },
        { id: "fullscreen", title: "Full Screen" },
        { id: "sentence", title: "Sentence Focus", toggle: true },
        { id: "paragraph", title: "Paragraph Focus", toggle: true },
        { id: "typewriter", title: "Typewriter Scrolling", toggle: true },
        { id: "strike", title: "Strikethrough" },
        { id: "inlineCode", title: "Inline Code" }
    ]
    function entry(id) {
        for (var i = 0; i < entries.length; ++i)
            if (entries[i].id === id) return entries[i];
        return null;
    }
    function label(id) {
        if (id === "fullscreen")
            return window.visibility === Window.FullScreen ? "Exit Full Screen" : "Enter Full Screen";
        var item = entry(id);
        if (!item) return "";
        if (["library", "organizer", "sortBar", "filterBar", "togglePreview"].indexOf(id) >= 0)
            return (isChecked(id) ? "Hide " : "Show ") + item.title;
        return item.title;
    }
    function isEnabled(id) {
        if (["duplicate", "rename", "move", "reveal"].indexOf(id) >= 0) return backend.fileUrl.toString() !== "";
        if (["quickOpen", "refreshTags"].indexOf(id) >= 0) return library.rootFolder.toString() !== "";
        if (id === "spelling") return window.isMac && editor.length > 0;
        if (id === "organizer") return settings.libraryVisible && window.width >= 1000;
        if (id === "larger") return settings.writingSize < 32;
        if (id === "smaller") return settings.writingSize > 12;
        return entry(id) !== null;
    }
    function isChecked(id) {
        switch (id) {
        case "library": return settings.libraryVisible;
        case "organizer": return settings.organizerVisible;
        case "sortBar": return libraryPane.showSortBar;
        case "filterBar": return libraryPane.showFilterBar;
        case "sortName": return library.sortMode === 0;
        case "sortModified": return library.sortMode === 1;
        case "sortCreated": return library.sortMode === 2;
        case "sortExtension": return library.sortMode === 3;
        case "ascending": return library.ascending;
        case "descending": return !library.ascending;
        case "foldersFirst": return library.foldersFirst;
        case "dates": return libraryPane.showDates;
        case "excerpts": return libraryPane.showExcerpts;
        case "editor": return settings.layoutMode === 0;
        case "split": return settings.layoutMode === 1;
        case "preview": return settings.layoutMode === 2;
        case "togglePreview": return settings.layoutMode !== 0;
        case "sans": return settings.previewStyle === 0;
        case "serif": return settings.previewStyle === 1;
        case "mono": return settings.previewStyle === 2;
        case "markup": return settings.showMarkup;
        case "sentence": return settings.sentenceFocus;
        case "paragraph": return settings.paragraphFocus;
        case "typewriter": return settings.typewriter;
        default: return false;
        }
    }
    function run(id) {
        if (!isEnabled(id)) return;
        switch (id) {
        default: commandRequested(id); break;
        case "library": settings.libraryVisible = !settings.libraryVisible; break;
        case "organizer": settings.organizerVisible = !settings.organizerVisible; break;
        case "sortBar": libraryPane.showSortBar = !libraryPane.showSortBar; break;
        case "filterBar": libraryPane.showFilterBar = !libraryPane.showFilterBar; break;
        case "sortName": library.sortMode = 0; break;
        case "sortModified": library.sortMode = 1; break;
        case "sortCreated": library.sortMode = 2; break;
        case "sortExtension": library.sortMode = 3; break;
        case "ascending": library.ascending = true; break;
        case "descending": library.ascending = false; break;
        case "foldersFirst": library.foldersFirst = !library.foldersFirst; break;
        case "dates": libraryPane.showDates = !libraryPane.showDates; break;
        case "excerpts": libraryPane.showExcerpts = !libraryPane.showExcerpts; break;
        case "larger": settings.writingSize = Math.min(32, settings.writingSize + 2); break;
        case "smaller": settings.writingSize = Math.max(12, settings.writingSize - 2); break;
        case "resetSize": settings.writingSize = 16; break;
        case "editor": settings.layoutMode = 0; break;
        case "split": settings.layoutMode = 1; break;
        case "preview": settings.layoutMode = 2; break;
        case "togglePreview": settings.layoutMode = settings.layoutMode === 0 ? 1 : 0; break;
        case "reloadPreview": preview.reload(); break;
        case "sans": settings.previewStyle = 0; break;
        case "serif": settings.previewStyle = 1; break;
        case "mono": settings.previewStyle = 2; break;
        case "markup": settings.showMarkup = !settings.showMarkup; break;
        case "outline": outlineRequested(); break;
        case "statistics": statisticsRequested(); break;
        case "fullscreen": window.toggleFullScreen(); break;
        case "sentence": settings.sentenceFocus = !settings.sentenceFocus; if (settings.sentenceFocus) settings.paragraphFocus = false; break;
        case "paragraph": settings.paragraphFocus = !settings.paragraphFocus; if (settings.paragraphFocus) settings.sentenceFocus = false; break;
        case "typewriter": settings.typewriter = !settings.typewriter; typewriterChanged(); break;
        case "strike": editor.wrapSelection("~~", "~~"); break;
        case "inlineCode": editor.wrapSelection("`", "`"); break;
        }
    }
}
