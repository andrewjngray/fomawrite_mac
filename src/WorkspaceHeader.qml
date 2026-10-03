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
    readonly property real writingControlsWidth: documentHeader.width - documentHeader.nativeInset
        - (paneRestoreControls.visible ? paneRestoreControls.implicitWidth + 8 : 0)
    readonly property bool writingClusterExpanded: writingControlsWidth >= 490
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
            anchors.fill: parent; anchors.leftMargin: root.window.isMac ? 84 : 12; anchors.rightMargin: 12; spacing: 8
            visible: root.toolbarContentVisible
            opacity: root.toolbarContentOpacity
            enabled: opacity > 0
            Item { Layout.fillWidth: true }
            ToolbarButton { objectName: "collapseOrganizerButton"; iconName: "panel-left-close"; hint: "Hide organizer"; onClicked: root.actionRequested("hideOrganizer", this) }
        }
        Rectangle { objectName: "organizerHeaderDivider"; x: parent.width; width: 1; height: parent.height; color: backend.palette.border }
    }
    Rectangle {
        id: filesHeader
        objectName: "filesHeader"
        visible: root.filesSlot.visible
        x: root.filesSlot.x; width: root.filesSlot.width; height: root.height
        color: backend.palette.library
        MouseArea { anchors.fill: parent; onPressed: root.window.startSystemMove(); onDoubleClicked: root.window.visibility === Window.Maximized ? root.window.showNormal() : root.window.showMaximized() }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: root.window.isMac && !organizerHeader.visible ? 84 : 12; anchors.rightMargin: 12; spacing: 8
            visible: root.toolbarContentVisible; opacity: root.toolbarContentOpacity; enabled: opacity > 0
            ChromeButton {
                objectName: "workspaceFolderButton"
                text: backend.library.rootName || "Folder"
                iconName: "folder"
                hint: "Choose library folder — " + (backend.library.rootName || "Folder")
                alignLeft: true
                tonal: true
                implicitHeight: 32
                font.weight: Font.Medium
                Layout.fillWidth: true
                Layout.minimumWidth: 32
                onClicked: root.actionRequested("chooseFolder", this)
            }
            ToolbarButton { objectName: "newDocumentButton"; iconName: "compose"; hint: "New document"; onClicked: root.actionRequested("newDocument", this) }
            ToolbarButton { objectName: "collapseFilesButton"; iconName: "panel-left-close"; hint: "Hide library"; onClicked: root.actionRequested("hideFiles", this) }
        }
        Rectangle { objectName: "filesHeaderDivider"; x: parent.width; width: 1; height: parent.height; color: backend.palette.border }
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
                id: paneRestoreControls
                objectName: "paneRestoreControls"
                visible: !organizerHeader.visible || !filesHeader.visible
                spacing: 8
                // Collapsing a pane removes its header. Keep its reopen control in
                // the surviving writing header, independent of toolbar visibility.
                ToolbarButton {
                    objectName: "restoreOrganizerButton"
                    visible: !organizerHeader.visible
                    iconName: "panel-left-open"
                    hint: "Show organizer"
                    onClicked: root.actionRequested("toggleOrganizer", this)
                }
                ToolbarButton {
                    objectName: "restoreFilesButton"
                    visible: !filesHeader.visible
                    iconName: "panel-left-open"
                    hint: "Show library"
                    onClicked: root.actionRequested("toggleFiles", this)
                }
            }
            Row {
                objectName: "topChromeToolbarLeading"
                visible: root.toolbarContentVisible; opacity: root.toolbarContentOpacity; enabled: opacity > 0
                spacing: 8
                // Workspace remains available to reveal faded controls before keyboard traversal.
                ToolbarGroup {
                    objectName: "documentHistoryControls"
                    visible: root.writingControlsWidth >= 360
                    ToolbarButton { objectName: "documentBackButton"; grouped: true; iconName: "back"; hint: "Previous document"; enabled: backend.canGoBack; onClicked: root.actionRequested("back", this) }
                    ToolbarButton { objectName: "documentForwardButton"; grouped: true; iconName: "forward"; hint: "Next document"; enabled: backend.canGoForward; onClicked: root.actionRequested("forward", this) }
                }
                ToolbarButton { objectName: "documentSearchButton"; iconName: "search"; hint: "Find in document"; onClicked: root.actionRequested("find", this) }
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
                    ToolbarButton { objectName: "compactBoldButton"; grouped: true; text: "B"; font.pixelSize: 16; font.weight: Font.Bold; hint: trailing.formattingAllowed ? "Bold selection" : "Choose Source to apply formatting"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("bold", this) }
                    ToolbarButton { objectName: "compactItalicButton"; grouped: true; text: "I"; font.family: "Georgia"; font.pixelSize: 16; font.italic: true; hint: "Italic selection"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("italic", this) }
                    ToolbarButton { objectName: "compactLinkButton"; grouped: true; iconName: "link"; hint: "Insert or edit link"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("link", this) }
                    ToolbarButton { objectName: "compactParagraphButton"; grouped: true; text: "¶"; font.pixelSize: 16; hint: "Paragraph formatting"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("format", this) }
                }
                ToolbarButton { objectName: "compactFormatButton"; visible: !root.writingClusterExpanded; iconName: "paragraph"; hint: "Formatting"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("format", this) }
                ToolbarGroup {
                    objectName: "workspaceZoomControls"
                    ToolbarButton {
                        objectName: "workspaceZoomOutButton"
                        grouped: true
                        iconName: "minus"
                        hint: "Decrease text size"
                        enabled: root.settings.writingSize > 12
                        onClicked: root.actionRequested("smaller", this)
                    }
                    ToolbarButton {
                        objectName: "workspaceZoomInButton"
                        grouped: true
                        iconName: "plus"
                        hint: "Increase text size"
                        enabled: root.settings.writingSize < 32
                        onClicked: root.actionRequested("larger", this)
                    }
                }
                ToolbarButton { objectName: "workspaceAppearanceButton"; text: "Aa"; hint: "Writing appearance"; onClicked: root.actionRequested("appearance", this) }
                ToolbarButton { objectName: "exportHubButton"; visible: root.writingControlsWidth >= 420; iconName: "export"; hint: "Export and share"; onClicked: root.actionRequested("export", this) }
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
