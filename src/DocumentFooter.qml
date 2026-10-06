import QtQuick
import QtQuick.Controls

// Pane appearance and output styles follow their content. Editing and layout
// are independent choices, each in its own permanent capsule.
WorkspaceFooter {
    id: root
    property int layoutMode: 0
    property bool liveEditing: false
    property bool canSplit: true
    property string publishingFormat: "web"
    property bool activityHidden: false
    property bool menuOpen: false
    property bool keyboardReveal: false
    property bool pointerReveal: false
    readonly property bool chromeVisible: !activityHidden || pointerReveal || menuOpen || keyboardReveal
    opacity: chromeVisible ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 140 } }
    HoverHandler {
        id: footerHover
        onHoveredChanged: if (hovered) root.pointerReveal = true
    }
    property real sourcePaneWidth: layoutMode === 1 ? width / 2 : width
    property real previewPaneStart: layoutMode === 1 ? sourcePaneWidth : 0
    property string writingAppearance: "Manuscript"
    property string previewTemplate: "Modern (Sans)"
    property string statusText: ""
    property bool showStatus: false
    property bool showStatistics: false
    property string statisticsText: ""
    readonly property bool hasPreview: layoutMode !== 0
    readonly property real controlsStart: layoutMode === 2 ? publishingControls.x : editingControls.x
    // The three editing options take 182 px (138 px compact). When the pane cannot
    // spare 182 px and still leave the style control reachable, the options use
    // tighter labels; names and hints keep the full wording.
    readonly property real editingRoom: (layoutMode === 1 ? Math.min(sourcePaneWidth, previewPaneStart) : layoutControls.x) - 12
    readonly property bool compactEditing: editingRoom - 54 < 182
    readonly property real sourceToolsEnd: editingControls.x - 8
    readonly property real appearanceRoom: Math.max(0, sourceToolsEnd - 12)
    readonly property real templateRoom: Math.max(0, publishingControls.x - previewPaneStart - 20)
    readonly property real statusStart: (layoutMode === 2 ? templateButton.x + templateButton.width
                                                                      : appearanceButton.x + appearanceButton.width) + 12
    readonly property real statusEnd: layoutMode === 2 ? publishingControls.x - 8 : sourceToolsEnd
    readonly property real statusWidth: Math.max(0, statusEnd - statusStart)
    readonly property bool statusVisible: statusLabel.visible
    signal publishingFormatRequested(string format)
    signal sourceEditingRequested()
    signal liveEditingRequested()
    signal layoutRequested(int mode)
    signal appearanceMenuRequested(var anchor)
    signal templateMenuRequested(var anchor)
    signal statisticsRequested()

    Rectangle {
        x: 0; y: 1; width: root.sourcePaneWidth; height: parent.height - 1
        visible: root.layoutMode !== 2
        color: backend.palette.editor
    }
    Rectangle {
        objectName: "documentFooterPaneDivider"
        visible: root.layoutMode === 1
        x: root.previewPaneStart; width: 1; height: parent.height
        color: backend.palette.border
    }
    FooterButton {
        enabled: root.chromeVisible
        id: appearanceButton
        objectName: "sourceAppearanceButton"
        visible: root.layoutMode !== 2 && root.appearanceRoom >= 34
        x: 12; anchors.verticalCenter: parent.verticalCenter
        width: Math.min(128, root.appearanceRoom)
        text: width >= 110 ? root.writingAppearance : "Aa"
        menuIndicator: width >= 60
        leftPadding: width < 70 ? 6 : 12
        rightPadding: leftPadding
        alignLeft: width >= 110
        hint: "Choose editor writing appearance: " + root.writingAppearance
        onClicked: root.appearanceMenuRequested(this)
    }
    FooterButton {
        enabled: root.chromeVisible
        id: templateButton
        objectName: "previewTemplateButton"
        visible: root.hasPreview && root.previewPaneStart >= 0 && root.templateRoom >= 34
        x: root.previewPaneStart + 12; anchors.verticalCenter: parent.verticalCenter
        width: Math.min(160, root.templateRoom)
        text: width >= 110 ? "Output Style" : "Aa"
        menuIndicator: width >= 60
        leftPadding: width < 70 ? 6 : 12
        rightPadding: leftPadding
        alignLeft: width >= 70
        hint: "Choose output style: " + root.previewTemplate
        onClicked: root.templateMenuRequested(this)
    }
    Label {
        id: statusLabel
        objectName: "sourceFooterStatus"
        x: root.statusStart; width: root.statusWidth
        anchors.verticalCenter: parent.verticalCenter
        visible: root.showStatus && !root.showStatistics && width >= 150
        text: root.statusText
        font.family: Qt.application.font.family; font.pixelSize: 13
        color: backend.palette.muted; elide: Text.ElideRight
        Accessible.name: "Document status: " + text
    }
    FooterButton {
        enabled: root.chromeVisible
        objectName: "toolbarStatistic"
        x: root.statusStart; width: Math.min(200, root.statusWidth)
        anchors.verticalCenter: parent.verticalCenter
        visible: root.showStatistics && width >= 110
        text: root.statisticsText; hint: text
        onClicked: root.statisticsRequested()
    }
    ToolbarGroup {
        id: editingControls
        objectName: "documentEditingControls"
        visible: root.layoutMode !== 2
        x: (root.layoutMode === 1 ? Math.min(root.sourcePaneWidth, root.previewPaneStart) : layoutControls.x) - width - 12
        anchors.verticalCenter: parent.verticalCenter
        Accessible.role: Accessible.Grouping
        Accessible.name: "Editing mode"
        FooterButton {
            enabled: root.chromeVisible
            objectName: "sourceModeButton"
            text: "Source"; hint: "Edit Markdown source"
            leftPadding: root.compactEditing ? 5 : 6; rightPadding: leftPadding
            grouped: true; width: root.compactEditing ? 54 : 56
            checked: root.layoutMode !== 2 && !root.liveEditing
            Accessible.checkable: true; Accessible.checked: checked
            onClicked: root.sourceEditingRequested()
        }
        FooterButton {
            enabled: root.chromeVisible
            objectName: "liveEditToggle"
            text: "Live"
            hint: "Live — edit with Markdown rendered in place"
            leftPadding: root.compactEditing ? 5 : 6; rightPadding: leftPadding
            grouped: true; width: root.compactEditing ? 36 : 44
            checked: root.layoutMode !== 2 && root.liveEditing
            Accessible.checkable: true; Accessible.checked: checked
            onClicked: root.liveEditingRequested()
        }
    }
    ToolbarGroup {
        id: publishingControls
        objectName: "documentPublishingControls"
        visible: root.hasPreview
        x: layoutControls.x - width - 8
        anchors.verticalCenter: parent.verticalCenter
        Accessible.role: Accessible.Grouping
        Accessible.name: "Publishing format"
        FooterButton {
            enabled: root.chromeVisible
            objectName: "webPublishingButton"
            text: "Web"; hint: "Web — read-only HTML publishing output"
            leftPadding: 6; rightPadding: 6
            grouped: true; width: 44
            checked: root.publishingFormat === "web"
            Accessible.checkable: true; Accessible.checked: checked
            onClicked: root.publishingFormatRequested("web")
        }
        FooterButton {
            enabled: root.chromeVisible
            objectName: "pdfPublishingButton"
            text: "PDF"; hint: "PDF — read-only paginated publishing output"
            leftPadding: 6; rightPadding: 6
            grouped: true; width: 44
            checked: root.publishingFormat === "pdf"
            Accessible.checkable: true; Accessible.checked: checked
            onClicked: root.publishingFormatRequested("pdf")
        }
    }
    ToolbarGroup {
        id: layoutControls
        objectName: "documentViewControls"
        anchors.right: parent.right; anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        Accessible.role: Accessible.Grouping
        Accessible.name: "Document layout"
        FooterButton {
            enabled: root.chromeVisible
            objectName: "singleModeButton"
            text: "Single"; hint: "Single — show the chosen editor alone"
            leftPadding: 6; rightPadding: 6
            grouped: true; width: 56
            checked: root.layoutMode === 0
            Accessible.checkable: true; Accessible.checked: checked
            onClicked: root.layoutRequested(0)
        }
        FooterButton {
            objectName: "previewSplitButton"
            text: "Split"
            hint: root.canSplit ? "Split — show the chosen editor beside output Preview" : "Widen the window to use Split layout"
            leftPadding: 6; rightPadding: 6
            grouped: true; width: 44
            enabled: root.chromeVisible && root.canSplit
            checked: root.layoutMode === 1
            Accessible.checkable: true; Accessible.checked: checked
            onClicked: root.layoutRequested(1)
        }
    }
}
