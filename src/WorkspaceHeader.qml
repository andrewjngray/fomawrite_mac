import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Item {
    id: root
    objectName: "topChrome"
    required property var window
    required property var settings
    required property var layoutState
    required property var organizerSlot
    required property var filesSlot
    required property var editorPane
    required property var previewPane
    property bool keyboardReveal: false
    readonly property bool revealRequested: chromeHover.hovered || keyboardReveal
    readonly property real titleContentOpacity: settings.titleBarMode === 1 || revealRequested ? 1 : 0
    readonly property real toolbarContentOpacity: settings.toolbarVisibilityMode === 1 || revealRequested ? 1 : 0
    readonly property bool toolbarContentVisible: settings.toolbarVisibilityMode !== 2
    readonly property bool writingClusterExpanded: documentHeader.width - documentHeader.nativeInset >= 490
    signal actionRequested(string action, var anchor)
    function focusWorkspaceControl() { workspaceButton.forceActiveFocus(Qt.TabFocusReason); }
    height: 52
    z: 20
    Rectangle {
        anchors.fill: parent
        color: backend.palette.page
        MouseArea { anchors.fill: parent; onPressed: root.window.startSystemMove(); onDoubleClicked: root.window.visibility === Window.Maximized ? root.window.showNormal() : root.window.showMaximized() }
    }
    HoverHandler { id: chromeHover }
    Rectangle {
        id: organizerHeader
        objectName: "organizerHeader"
        visible: root.organizerSlot.visible
        x: root.organizerSlot.x; width: root.organizerSlot.width; height: root.height
        color: backend.palette.organizer
        MouseArea { anchors.fill: parent; onPressed: root.window.startSystemMove(); onDoubleClicked: root.window.visibility === Window.Maximized ? root.window.showNormal() : root.window.showMaximized() }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: root.window.isMac ? 84 : 12; anchors.rightMargin: 10; spacing: 6
            visible: root.toolbarContentVisible
            opacity: root.toolbarContentOpacity
            enabled: opacity > 0
            Item { Layout.fillWidth: true }
            ToolbarButton { iconName: "plus"; hint: "Add a library location"; onClicked: root.actionRequested("addLocation", this) }
            ToolbarButton { objectName: "collapseOrganizerButton"; iconName: "organizer"; hint: "Hide organizer"; onClicked: root.actionRequested("hideOrganizer", this) }
        }
        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: backend.palette.border }
    }
    Rectangle {
        id: filesHeader
        objectName: "filesHeader"
        visible: root.filesSlot.visible
        x: root.filesSlot.x; width: root.filesSlot.width; height: root.height
        color: backend.palette.library
        MouseArea { anchors.fill: parent; onPressed: root.window.startSystemMove(); onDoubleClicked: root.window.visibility === Window.Maximized ? root.window.showNormal() : root.window.showMaximized() }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: root.window.isMac && !organizerHeader.visible ? 84 : 12; anchors.rightMargin: 10; spacing: 6
            visible: root.toolbarContentVisible; opacity: root.toolbarContentOpacity; enabled: opacity > 0
            ChromeButton { text: backend.library.rootName || "Folder"; hint: "Choose library folder"; alignLeft: true; Layout.fillWidth: true; onClicked: root.actionRequested("chooseFolder", this) }
            ToolbarButton { iconName: "plus"; hint: "New document"; onClicked: root.actionRequested("newDocument", this) }
            ToolbarButton { iconName: "more"; hint: "Folder actions, sorting and search"; onClicked: root.actionRequested("libraryOptions", this) }
            ToolbarButton { objectName: "collapseFilesButton"; iconName: "library"; hint: "Hide files"; onClicked: root.actionRequested("hideFiles", this) }
        }
        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: backend.palette.border }
    }
    Rectangle {
        id: documentHeader
        objectName: "documentHeader"
        readonly property int nativeInset: root.window.isMac && !organizerHeader.visible && !filesHeader.visible ? 84 : 12
        x: root.editorPane.visible ? root.editorPane.x : root.previewPane.x
        width: root.editorPane.visible ? root.editorPane.width : root.previewPane.width
        height: root.height
        color: backend.palette.page
        MouseArea { anchors.fill: parent; onPressed: root.window.startSystemMove(); onDoubleClicked: root.window.visibility === Window.Maximized ? root.window.showNormal() : root.window.showMaximized() }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: documentHeader.nativeInset; anchors.rightMargin: 12; spacing: 8
            Row {
                objectName: "topChromeToolbarLeading"
                visible: root.toolbarContentVisible; opacity: root.toolbarContentOpacity; enabled: opacity > 0
                spacing: 8
                // Workspace remains available to reveal faded controls before keyboard traversal.
                ToolbarGroup {
                    ToolbarButton { objectName: "documentBackButton"; grouped: true; iconName: "back"; hint: "Previous document"; enabled: backend.canGoBack; onClicked: root.actionRequested("back", this) }
                    ToolbarButton { objectName: "documentForwardButton"; grouped: true; iconName: "forward"; hint: "Next document"; enabled: backend.canGoForward; onClicked: root.actionRequested("forward", this) }
                }
                ToolbarButton { iconName: "search"; hint: "Find in document"; onClicked: root.actionRequested("find", this) }
            }
            Item { Layout.fillWidth: true; Layout.minimumWidth: 6 }
            Row {
                id: trailing
                objectName: "topChromeToolbarTrailing"
                visible: root.toolbarContentVisible; opacity: root.toolbarContentOpacity; enabled: opacity > 0; spacing: 8
                readonly property bool formattingAllowed: root.editorPane.visible && !(root.previewPane.visualEditEnabled && root.window.lastWritingSurface === "visual")
                ToolbarGroup {
                    objectName: "compactWritingControls"
                    visible: root.writingClusterExpanded
                    ToolbarButton { objectName: "compactBoldButton"; grouped: true; text: "B"; font.bold: true; hint: trailing.formattingAllowed ? "Bold selection" : "Choose Source to apply formatting"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("bold", this) }
                    ToolbarButton { objectName: "compactItalicButton"; grouped: true; text: "I"; font.italic: true; hint: "Italic selection"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("italic", this) }
                    ToolbarButton { objectName: "compactLinkButton"; grouped: true; iconName: "link"; hint: "Insert or edit link"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("link", this) }
                    ToolbarButton { objectName: "compactParagraphButton"; grouped: true; iconName: "paragraph"; hint: "Paragraph formatting"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("format", this) }
                }
                ToolbarButton { objectName: "compactFormatButton"; visible: !root.writingClusterExpanded; iconName: "paragraph"; hint: "Formatting"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("format", this) }
                ToolbarButton { text: "Aa"; hint: "Writing appearance"; onClicked: root.actionRequested("appearance", this) }
                ToolbarButton { objectName: "exportHubButton"; visible: documentHeader.width - documentHeader.nativeInset >= 420; iconName: "export"; hint: "Export and share"; onClicked: root.actionRequested("export", this) }
            }
            ToolbarButton {
                id: workspaceButton
                objectName: "topChromeLibraryButton"
                iconName: "workspace"; hint: "Workspace columns and document modes"
                // This reveal control remains reachable even when the toolbar is hidden.
                onClicked: root.actionRequested("workspace", this)
            }
        }
    }
    Rectangle {
        objectName: "previewHeader"
        visible: root.editorPane.visible && root.previewPane.visible
        x: root.previewPane.x; width: root.previewPane.width; height: root.height
        color: backend.palette.page
        MouseArea { anchors.fill: parent; onPressed: root.window.startSystemMove() }
        RowLayout {
            anchors.fill: parent; anchors.margins: 12
            visible: root.toolbarContentVisible; opacity: root.toolbarContentOpacity; enabled: opacity > 0
            Label { text: root.previewPane.visualEditEnabled ? "Visual Edit" : "Preview"; color: backend.palette.muted; font.pixelSize: 13; Layout.fillWidth: true }
            ToolbarButton { iconName: "export"; hint: "Export and share"; onClicked: root.actionRequested("export", this) }
            ToolbarButton { iconName: "close"; hint: "Hide preview"; onClicked: root.actionRequested("hidePreview", this) }
        }
    }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: backend.palette.border }
}
