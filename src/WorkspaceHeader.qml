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
    required property var zoomController
    // Explicit bindings can be supplied by a window; defaults retain the
    // per-engine Backend identity used by existing component fixtures.
    property string documentName: backend.fileName
    property bool documentModified: backend.modified
    readonly property bool fullPreview: !editorPane.visible && previewPane.visible
    readonly property bool visualEditor: editorPane.visible && layoutState.visualEditEnabled
    property bool keyboardReveal: false
    // Edge entry reveals until the next editing or scrolling gesture.
    property bool pointerReveal: false
    property bool activityHidden: false
    property bool menuOpen: false
    readonly property bool zoomMenuOpen: editorZoomControls.menuOpen || publishingZoomControls.menuOpen
    readonly property bool documentChromeVisible: !activityHidden || pointerReveal || keyboardReveal || menuOpen
    readonly property bool revealRequested: chromeHover.hovered || pointerReveal || keyboardReveal
    readonly property real titleContentOpacity: documentChromeVisible && (settings.titleBarMode === 1 || revealRequested) ? 1 : 0
    readonly property real toolbarContentOpacity: documentChromeVisible && (settings.toolbarVisibilityMode === 1 || revealRequested) ? 1 : 0
    readonly property bool toolbarContentVisible: settings.toolbarVisibilityMode !== 2
    readonly property real writingControlsWidth: documentHeader.width - documentHeader.nativeInset
        - (paneRestoreControls.visible ? paneRestoreControls.implicitWidth + 8 : 0)
    readonly property bool writingClusterExpanded: writingControlsWidth >= 580
    signal actionRequested(string action, var anchor)
    function focusWorkspaceControl() {
        keyboardReveal = true;
        // F6 from Source can enter formatting without first transferring its
        // selection ownership to an unrelated workspace action.
        if (window.canFormatSource && toolbarContentVisible && toolbarContentOpacity > 0) {
            if (writingClusterExpanded) sourceBoldButton.forceActiveFocus(Qt.TabFocusReason);
            else sourceFormatButton.forceActiveFocus(Qt.TabFocusReason);
        } else workspaceButton.forceActiveFocus(Qt.TabFocusReason);
    }
    // The filename and dirty marker are independent text runs: the filename
    // can elide without hiding whether this particular document is unsaved.
    component PreviewDocumentIdentity: Item {
        id: identity
        required property string identityPrefix
        readonly property string displayName: root.documentName.length > 0 ? root.documentName : "Untitled.md"
        readonly property string accessibleTitle: displayName + (root.documentModified ? " — Edited" : "")
        objectName: identityPrefix + "Identity"
        implicitWidth: identityRow.implicitWidth
        implicitHeight: 20
        clip: true
        Accessible.role: Accessible.StaticText
        Accessible.name: accessibleTitle
        ToolTip.visible: identityHover.hovered
        ToolTip.delay: 700
        ToolTip.text: accessibleTitle
        HoverHandler { id: identityHover }
        RowLayout {
            id: identityRow
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(parent.width, implicitWidth)
            height: parent.height
            spacing: 6
            LineIcon {
                objectName: identity.identityPrefix + "Icon"
                name: "document"
                ink: backend.palette.muted
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                Layout.minimumWidth: 16
                Layout.maximumWidth: 16
                Accessible.ignored: true
            }
            Text {
                objectName: identity.identityPrefix + "Name"
                text: identity.displayName
                textFormat: Text.PlainText
                font.family: Qt.application.font.family
                font.pixelSize: 13
                font.weight: Font.Bold
                color: backend.palette.text
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                elide: Text.ElideMiddle
                maximumLineCount: 1
                verticalAlignment: Text.AlignVCenter
                Accessible.ignored: true
            }
            Text {
                objectName: identity.identityPrefix + "Edited"
                visible: root.documentModified
                text: "— Edited"
                textFormat: Text.PlainText
                font.family: Qt.application.font.family
                font.pixelSize: 13
                font.weight: Font.Normal
                color: backend.palette.muted
                Layout.minimumWidth: implicitWidth
                Layout.maximumWidth: implicitWidth
                verticalAlignment: Text.AlignVCenter
                Accessible.ignored: true
            }
        }
    }
    height: 52
    z: 20
    Rectangle {
        anchors.fill: parent
        color: backend.palette.page
        Rectangle {
            x: root.editorPane.x; width: root.editorPane.width; height: parent.height
            visible: root.editorPane.visible
            color: backend.palette.editor
        }
        MouseArea { anchors.fill: parent; enabled: root.documentChromeVisible; onPressed: root.window.startSystemMove(); onDoubleClicked: root.window.visibility === Window.Maximized ? root.window.showNormal() : root.window.showMaximized() }
    }
    HoverHandler {
        id: chromeHover
        onHoveredChanged: if (hovered && root.settings.autoHideChrome) root.pointerReveal = true
    }
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
            opacity: root.settings.toolbarVisibilityMode === 1 || root.revealRequested ? 1 : 0
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
            visible: root.toolbarContentVisible; opacity: root.settings.toolbarVisibilityMode === 1 || root.revealRequested ? 1 : 0; enabled: opacity > 0
            ChromeButton {
                objectName: "workspaceFolderButton"
                text: backend.library.rootName || "Folder"
                iconName: "folder"
                hint: "Choose library folder — " + (backend.library.rootName || "Folder")
                alignLeft: true
                tonal: true
                implicitHeight: 34
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
        opacity: root.documentChromeVisible ? 1 : 0
        enabled: root.documentChromeVisible
        Behavior on opacity { NumberAnimation { duration: 140 } }
        objectName: "documentHeader"
        readonly property int nativeInset: root.window.isMac && !organizerHeader.visible && !filesHeader.visible ? 84 : 12
        x: root.editorPane.visible ? root.editorPane.x : root.previewPane.x
        width: root.editorPane.visible ? root.editorPane.width : root.previewPane.width
        height: root.height
        color: root.editorPane.visible ? backend.palette.editor : backend.palette.page
        MouseArea { anchors.fill: parent; enabled: root.documentChromeVisible; onPressed: root.window.startSystemMove(); onDoubleClicked: root.window.visibility === Window.Maximized ? root.window.showNormal() : root.window.showMaximized() }
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
                visible: root.toolbarContentVisible && root.writingControlsWidth >= 340; opacity: root.toolbarContentOpacity; enabled: opacity > 0
                spacing: 8
                // Workspace remains available to reveal faded controls before keyboard traversal.
                ToolbarGroup {
                    objectName: "documentHistoryControls"
                    visible: root.writingControlsWidth >= 670
                    ToolbarButton { objectName: "documentBackButton"; grouped: true; iconName: "back"; hint: "Previous document"; enabled: backend.canGoBack; onClicked: root.actionRequested("back", this) }
                    ToolbarButton { objectName: "documentForwardButton"; grouped: true; showDivider: true; iconName: "forward"; hint: "Next document"; enabled: backend.canGoForward; onClicked: root.actionRequested("forward", this) }
                }
                ToolbarButton { objectName: "documentSearchButton"; iconName: "search"; hint: "Find in document"; onClicked: root.actionRequested("find", this) }
            }
            Label {
                objectName: "fullPreviewHeaderModeLabel"
                visible: root.fullPreview && root.toolbarContentVisible && root.writingControlsWidth >= 380
                text: "Preview"
                font.pixelSize: 13
                color: backend.palette.muted
                opacity: root.toolbarContentOpacity
            }
            PreviewDocumentIdentity {
                identityPrefix: "fullPreviewDocument"
                visible: root.fullPreview && root.toolbarContentVisible
                opacity: root.titleContentOpacity
                Layout.fillWidth: true
                Layout.minimumWidth: 0
            }
            Label {
                objectName: "visualEditorHeaderModeLabel"
                visible: root.visualEditor && root.toolbarContentVisible && root.writingControlsWidth >= 540
                text: "Visual Edit"
                font.pixelSize: 13
                color: backend.palette.muted
                opacity: root.toolbarContentOpacity
            }
            PreviewDocumentIdentity {
                identityPrefix: "visualEditorDocument"
                visible: root.visualEditor && root.toolbarContentVisible
                opacity: root.titleContentOpacity
                Layout.fillWidth: true
                Layout.minimumWidth: 0
            }
            Item { visible: !root.fullPreview && !root.visualEditor; Layout.fillWidth: true; Layout.minimumWidth: 6 }
            Row {
                id: trailing
                objectName: "topChromeToolbarTrailing"
                visible: root.toolbarContentVisible; opacity: root.toolbarContentOpacity; enabled: opacity > 0; spacing: 8
                readonly property bool formattingAllowed: root.window.canFormatSource
                ToolbarGroup {
                    objectName: "compactWritingControls"
                    property bool sourceFormattingControl: true
                    visible: !root.fullPreview && !root.visualEditor && root.writingClusterExpanded
                    ToolbarButton { id: sourceBoldButton; objectName: "compactBoldButton"; grouped: true; text: "B"; font.pixelSize: 16; font.weight: Font.Bold; hint: trailing.formattingAllowed ? "Bold selection" : "Choose Source to apply formatting"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("bold", this) }
                    ToolbarButton { objectName: "compactItalicButton"; grouped: true; text: "I"; font.family: "Times New Roman"; font.pixelSize: 18; font.italic: true; hint: "Italic selection"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("italic", this) }
                    ToolbarButton { objectName: "compactLinkButton"; grouped: true; iconName: "link"; hint: "Insert or edit link"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("link", this) }
                    ToolbarButton { objectName: "compactParagraphButton"; grouped: true; iconName: "paragraph"; hint: "Paragraph formatting"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("format", this) }
                }
                ToolbarButton { id: sourceFormatButton; property bool sourceFormattingControl: true; objectName: "compactFormatButton"; visible: !root.fullPreview && !root.visualEditor && !root.writingClusterExpanded; iconName: "paragraph"; hint: "Formatting"; enabled: trailing.formattingAllowed; onClicked: root.actionRequested("format", this) }
                PaneZoomControls {
                    id: editorZoomControls
                    zoomController: root.zoomController
                    pane: root.editorPane.visible ? "source" : "preview"
                }
                ToolbarButton { objectName: "workspaceAppearanceButton"; visible: !root.fullPreview || root.writingControlsWidth >= 540; text: "Aa"; hint: "Writing appearance"; onClicked: root.actionRequested("appearance", this) }
                ToolbarButton { objectName: "exportHubButton"; visible: root.writingControlsWidth >= (root.fullPreview ? 540 : 450); iconName: "export"; hint: "Export and share"; onClicked: root.actionRequested("export", this) }
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
        id: splitPreviewHeader
        opacity: root.documentChromeVisible ? 1 : 0
        enabled: root.documentChromeVisible
        Behavior on opacity { NumberAnimation { duration: 140 } }
        objectName: "previewHeader"
        visible: root.editorPane.visible && root.previewPane.visible
        x: root.previewPane.x; width: root.previewPane.width; height: root.height
        color: backend.palette.page
        MouseArea { anchors.fill: parent; enabled: root.documentChromeVisible; onPressed: root.window.startSystemMove() }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
            visible: root.toolbarContentVisible; opacity: root.toolbarContentOpacity; enabled: opacity > 0
            spacing: 8
            Label {
                objectName: "previewHeaderModeLabel"
                visible: splitPreviewHeader.width >= 500
                text: "Preview"
                color: backend.palette.muted
                font.pixelSize: 13
            }
            PreviewDocumentIdentity {
                identityPrefix: "previewDocument"
                opacity: root.titleContentOpacity
                Layout.fillWidth: true
                Layout.minimumWidth: 0
            }
            PaneZoomControls {
                id: publishingZoomControls
                zoomController: root.zoomController
                pane: "preview"
                controlsActive: root.editorPane.visible && root.previewPane.visible
            }
            ToolbarButton { objectName: "previewHeaderExportButton"; visible: splitPreviewHeader.width >= 640; iconName: "export"; hint: "Export and share"; onClicked: root.actionRequested("export", this) }
            ToolbarButton { objectName: "previewHeaderCloseButton"; iconName: "close"; hint: "Hide preview"; onClicked: root.actionRequested("hidePreview", this) }
        }
    }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: backend.palette.border }
}
