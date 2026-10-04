import QtQuick
import QtQuick.Window

// One command surface for native menus, workspace controls and a future palette.
// Settings and the library remain the owners of persisted state.
QtObject {
    id: root
    objectName: "workspaceCommands"
    required property var settings
    required property var zoomController
    required property var layoutState
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
        { id: "themeStudio", title: "Theme: Studio" },
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
        { id: "dateModified", title: "Date Modified", toggle: true },
        { id: "dateCreated", title: "Date Created", toggle: true },
        { id: "dateNone", title: "None", toggle: true },
        { id: "excerpts", title: "Show Text Excerpts", toggle: true },
        { id: "navigationTree", title: "Tree", toggle: true },
        { id: "navigationList", title: "List", toggle: true },
        { id: "larger", title: "Larger Text" },
        { id: "smaller", title: "Smaller Text" },
        { id: "resetSize", title: "Reset Text Size" },
        { id: "writingManuscript", title: "Manuscript (Mono)", toggle: true },
        { id: "writingEditorial", title: "Editorial (Sans)", toggle: true },
        { id: "writingBook", title: "Book (Serif)", toggle: true },
        { id: "editor", title: "Editor Only", toggle: true },
        { id: "split", title: "Editor and Preview", toggle: true },
        { id: "preview", title: "Preview Only", toggle: true },
        { id: "togglePreview", title: "Preview", toggle: true },
        { id: "reloadPreview", title: "Reload Preview" },
        { id: "webPreview", title: "Web", toggle: true },
        { id: "sans", title: "Sans", toggle: true },
        { id: "serif", title: "Serif", toggle: true },
        { id: "mono", title: "Mono", toggle: true },
        { id: "markup", title: "Show Markdown Syntax", toggle: true },
        { id: "outline", title: "Document Outline" },
        { id: "statistics", title: "Document Statistics" },
        { id: "titleBarFade", title: "Fade In/Out", toggle: true },
        { id: "titleBarAlways", title: "Always Show", toggle: true },
        { id: "toolbarFade", title: "Fade In/Out", toggle: true },
        { id: "toolbarAlways", title: "Always Show", toggle: true },
        { id: "toolbarHide", title: "Hide", toggle: true },
        { id: "toolbarDefault", title: "Default", toggle: true },
        { id: "toolbarStatsOnly", title: "Stats Only", toggle: true },
        { id: "toolbarCharacters", title: "Characters", toggle: true },
        { id: "toolbarCharactersNoSpaces", title: "Characters Without Spaces", toggle: true },
        { id: "toolbarWords", title: "Words", toggle: true },
        { id: "toolbarSentences", title: "Sentences", toggle: true },
        { id: "toolbarReadingTime", title: "Reading Time", toggle: true },
        { id: "toolbarSpeakingTime", title: "Speaking Time", toggle: true },
        { id: "toolbarTasks", title: "Tasks", toggle: true },
        { id: "toolbarHuman", title: "Human", toggle: true },
        { id: "toolbarAI", title: "AI", toggle: true },
        { id: "toolbarReference", title: "Reference", toggle: true },
        { id: "fullscreen", title: "Full Screen" },
        { id: "sentence", title: "Sentence Focus", toggle: true },
        { id: "paragraph", title: "Paragraph Focus", toggle: true },
        { id: "typewriter", title: "Typewriter Scrolling (Source)", toggle: true },
        { id: "fillersStyleCheck", title: "Fillers", toggle: true },
        { id: "customStyleCheck", title: "Custom", toggle: true },
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
        if (id === "library" || id === "organizer") return "Toggle " + item.title;
        if (["sortBar", "filterBar", "togglePreview"].indexOf(id) >= 0)
            return (isChecked(id) ? "Hide " : "Show ") + item.title;
        return item.title;
    }
    function isEnabled(id) {
        if (["strike", "inlineCode", "pageBreak"].indexOf(id) >= 0) return window.canFormatSource;
        if (["duplicate", "rename", "move", "reveal"].indexOf(id) >= 0) return backend.fileUrl.toString() !== "";
        if (["quickOpen", "refreshTags"].indexOf(id) >= 0) return library.rootFolder.toString() !== "";
        if (id === "spelling") return window.isMac && editor.length > 0;
        if (id === "organizer") return true;
        if (id === "split") return layoutState.availableWidth >= 800;
        if (id === "larger") return zoomController.activeZoom < zoomController.maximumZoom;
        if (id === "smaller") return zoomController.activeZoom > zoomController.minimumZoom;
        return entry(id) !== null;
    }
    function isChecked(id) {
        switch (id) {
        case "library": return layoutState.effectiveFilesVisible;
        case "organizer": return layoutState.effectiveOrganizerVisible;
        case "sortBar": return libraryPane.showSortBar;
        case "filterBar": return libraryPane.showFilterBar;
        case "sortName": return library.sortMode === 0;
        case "sortModified": return library.sortMode === 1;
        case "sortCreated": return library.sortMode === 2;
        case "sortExtension": return library.sortMode === 3;
        case "ascending": return library.ascending;
        case "descending": return !library.ascending;
        case "foldersFirst": return library.foldersFirst;
        case "dateModified": return libraryPane.dateMode === 1;
        case "dateCreated": return libraryPane.dateMode === 2;
        case "dateNone": return libraryPane.dateMode === 0;
        case "excerpts": return libraryPane.showExcerpts;
        case "navigationTree": return library.navigationMode === 0;
        case "navigationList": return library.navigationMode === 1;
        case "editor": return layoutState.effectiveLayoutMode === 0;
        case "split": return layoutState.effectiveLayoutMode === 1;
        case "preview": return layoutState.effectiveLayoutMode === 2;
        case "togglePreview": return layoutState.effectiveLayoutMode !== 0;
        case "webPreview": return true;
        case "titleBarFade": return settings.titleBarMode === 0;
        case "titleBarAlways": return settings.titleBarMode === 1;
        case "toolbarFade": return settings.toolbarVisibilityMode === 0;
        case "toolbarAlways": return settings.toolbarVisibilityMode === 1;
        case "toolbarHide": return settings.toolbarVisibilityMode === 2;
        case "toolbarDefault": return settings.toolbarMode === 0;
        case "toolbarStatsOnly": return settings.toolbarMode === 1;
        case "toolbarCharacters": return settings.toolbarCharacters;
        case "toolbarCharactersNoSpaces": return settings.toolbarCharactersNoSpaces;
        case "toolbarWords": return settings.toolbarWords;
        case "toolbarSentences": return settings.toolbarSentences;
        case "toolbarReadingTime": return settings.toolbarReadingTime;
        case "toolbarSpeakingTime": return settings.toolbarSpeakingTime;
        case "toolbarTasks": return settings.toolbarTasks;
        case "toolbarHuman": return settings.toolbarHuman;
        case "toolbarAI": return settings.toolbarAI;
        case "toolbarReference": return settings.toolbarReference;
        case "sans": return backend.outputStyle === 0;
        case "serif": return backend.outputStyle === 1;
        case "mono": return backend.outputStyle === 2;
        case "markup": return settings.showMarkup;
        case "writingManuscript": return settings.writingAppearance === "manuscript";
        case "writingEditorial": return settings.writingAppearance === "editorial";
        case "writingBook": return settings.writingAppearance === "book";
        case "sentence": return settings.sentenceFocus;
        case "paragraph": return settings.paragraphFocus;
        case "typewriter": return settings.typewriter;
        case "fillersStyleCheck": return settings.styleCheckFillers;
        case "customStyleCheck": return settings.styleCheckCustom;
        default: return false;
        }
    }
    function run(id) {
        if (!isEnabled(id)) return;
        switch (id) {
        default: commandRequested(id); break;
        case "library": window.toggleWorkspacePane("files"); break;
        case "organizer": window.toggleWorkspacePane("organizer"); break;
        case "sortBar": libraryPane.showSortBar = !libraryPane.showSortBar; break;
        case "filterBar": libraryPane.showFilterBar = !libraryPane.showFilterBar; break;
        case "sortName": library.sortMode = 0; break;
        case "sortModified": library.sortMode = 1; break;
        case "sortCreated": library.sortMode = 2; break;
        case "sortExtension": library.sortMode = 3; break;
        case "ascending": library.ascending = true; break;
        case "descending": library.ascending = false; break;
        case "foldersFirst": library.foldersFirst = !library.foldersFirst; break;
        case "dateModified": libraryPane.dateMode = 1; break;
        case "dateCreated": libraryPane.dateMode = 2; break;
        case "dateNone": libraryPane.dateMode = 0; break;
        case "excerpts": libraryPane.showExcerpts = !libraryPane.showExcerpts; break;
        case "navigationTree": library.navigationMode = 0; break;
        case "navigationList": library.navigationMode = 1; break;
        case "larger": zoomController.adjustZoom(zoomController.effectivePane, 10); break;
        case "smaller": zoomController.adjustZoom(zoomController.effectivePane, -10); break;
        case "resetSize": zoomController.resetZoom(zoomController.effectivePane); break;
        case "writingManuscript": settings.writingAppearance = "manuscript"; break;
        case "writingEditorial": settings.writingAppearance = "editorial"; break;
        case "writingBook": settings.writingAppearance = "book"; break;
        case "editor": window.selectWritingMode("source"); break;
        case "split": window.setDocumentView(1); break;
        case "preview": window.selectWritingMode("preview"); break;
        case "togglePreview": window.setDocumentView(layoutState.effectiveLayoutMode === 0 ? (layoutState.availableWidth >= 800 ? 1 : 2) : 0); break;
        case "reloadPreview": preview.reload(); break;
        case "webPreview": if (layoutState.effectiveLayoutMode === 0) window.setDocumentView(layoutState.availableWidth >= 800 ? 1 : 2); break;
        case "titleBarFade": settings.titleBarMode = 0; break;
        case "titleBarAlways": settings.titleBarMode = 1; break;
        case "toolbarFade": settings.toolbarVisibilityMode = 0; break;
        case "toolbarAlways": settings.toolbarVisibilityMode = 1; break;
        case "toolbarHide": settings.toolbarVisibilityMode = 2; break;
        case "toolbarDefault": settings.toolbarMode = 0; break;
        case "toolbarStatsOnly": settings.toolbarMode = 1; break;
        case "toolbarCharacters": settings.toolbarCharacters = !settings.toolbarCharacters; settings.toolbarMode = 1; break;
        case "toolbarCharactersNoSpaces": settings.toolbarCharactersNoSpaces = !settings.toolbarCharactersNoSpaces; settings.toolbarMode = 1; break;
        case "toolbarWords": settings.toolbarWords = !settings.toolbarWords; settings.toolbarMode = 1; break;
        case "toolbarSentences": settings.toolbarSentences = !settings.toolbarSentences; settings.toolbarMode = 1; break;
        case "toolbarReadingTime": settings.toolbarReadingTime = !settings.toolbarReadingTime; settings.toolbarMode = 1; break;
        case "toolbarSpeakingTime": settings.toolbarSpeakingTime = !settings.toolbarSpeakingTime; settings.toolbarMode = 1; break;
        case "toolbarTasks": settings.toolbarTasks = !settings.toolbarTasks; settings.toolbarMode = 1; break;
        case "toolbarHuman": settings.toolbarHuman = !settings.toolbarHuman; settings.toolbarMode = 1; break;
        case "toolbarAI": settings.toolbarAI = !settings.toolbarAI; settings.toolbarMode = 1; break;
        case "toolbarReference": settings.toolbarReference = !settings.toolbarReference; settings.toolbarMode = 1; break;
        case "sans": backend.setOutputStyle(0); break;
        case "serif": backend.setOutputStyle(1); break;
        case "mono": backend.setOutputStyle(2); break;
        case "markup": settings.showMarkup = !settings.showMarkup; break;
        case "outline": outlineRequested(); break;
        case "statistics": statisticsRequested(); break;
        case "fullscreen": window.toggleFullScreen(); break;
        case "sentence": settings.sentenceFocus = !settings.sentenceFocus; if (settings.sentenceFocus) settings.paragraphFocus = false; break;
        case "paragraph": settings.paragraphFocus = !settings.paragraphFocus; if (settings.paragraphFocus) settings.sentenceFocus = false; break;
        case "typewriter": settings.typewriter = !settings.typewriter; typewriterChanged(); break;
        case "fillersStyleCheck": settings.styleCheckFillers = !settings.styleCheckFillers; break;
        case "customStyleCheck": settings.styleCheckCustom = !settings.styleCheckCustom; break;
        case "strike": window.tryWrapSelection("~~", "~~"); break;
        case "inlineCode": window.tryWrapSelection("`", "`"); break;
        }
    }
}
