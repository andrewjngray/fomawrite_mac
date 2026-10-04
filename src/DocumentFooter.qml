import QtQuick
import QtQuick.Controls

// Pane appearance and output styles follow their content. Editing and layout
// are independent choices, each in its own permanent capsule.
WorkspaceFooter {
    id: root
    property int layoutMode: 0
    property bool visualEditing: false
    property bool canSplit: true
    property real sourcePaneWidth: layoutMode === 1 ? width / 2 : width
    property real previewPaneStart: layoutMode === 1 ? sourcePaneWidth : 0
    property string writingAppearance: "Manuscript"
    property string previewTemplate: "Modern (Sans)"
    property string statusText: ""
    property bool showStatus: false
    property bool showStatistics: false
    property string statisticsText: ""
    readonly property bool hasPreview: layoutMode !== 0
    readonly property real controlsStart: editingControls.x
    // SplitView updates pane widths and positions during polish. Budget against
    // both edges so a previous editor width cannot cover the newly placed
    // Preview tool while switching layouts or contracting the navigation panes.
    readonly property real sourceToolsEnd: layoutMode === 1
        ? Math.min(sourcePaneWidth, previewPaneStart) - 12 : controlsStart - 8
    readonly property real appearanceRoom: Math.max(0, sourceToolsEnd - 12)
    readonly property real templateRoom: Math.max(0, controlsStart - 8 - previewPaneStart - 12)
    readonly property real statusStart: (layoutMode === 2 ? templateButton.x + templateButton.width
                                                                      : appearanceButton.x + appearanceButton.width) + 12
    readonly property real statusEnd: layoutMode === 1 ? sourceToolsEnd : controlsStart - 12
    readonly property real statusWidth: Math.max(0, statusEnd - statusStart)
    readonly property bool statusVisible: statusLabel.visible
    signal editingRequested(bool visual)
    signal layoutRequested(int mode)
    signal appearanceMenuRequested(var anchor)
    signal templateMenuRequested(var anchor)
    signal statisticsRequested()

    Rectangle {
        objectName: "documentFooterPaneDivider"
        visible: root.layoutMode === 1
        x: root.previewPaneStart; width: 1; height: parent.height
        color: backend.palette.border
    }
    FooterButton {
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
        id: templateButton
        objectName: "previewTemplateButton"
        visible: root.hasPreview && root.previewPaneStart >= 0 && root.templateRoom >= 34
        x: root.previewPaneStart + 12; anchors.verticalCenter: parent.verticalCenter
        width: Math.min(200, root.templateRoom)
        text: width >= 170 ? "Preview · " + root.previewTemplate : width >= 70 ? root.previewTemplate : "Aa"
        menuIndicator: width >= 60
        leftPadding: width < 70 ? 6 : 12
        rightPadding: leftPadding
        alignLeft: width >= 70
        hint: "Choose Preview template: " + root.previewTemplate
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
        x: layoutControls.x - width - 12
        anchors.verticalCenter: parent.verticalCenter
        Accessible.role: Accessible.Grouping
        Accessible.name: "Editing mode"
        FooterButton {
            objectName: "sourceModeButton"
            text: "Source"; hint: "Edit Markdown source"
            leftPadding: 6; rightPadding: 6
            grouped: true; width: 56
            checked: root.layoutMode !== 2 && !root.visualEditing
            Accessible.checkable: true; Accessible.checked: checked
            onClicked: root.editingRequested(false)
        }
        FooterButton {
            objectName: "visualEditToggle"
            text: "Visual Edit"
            hint: "Visual Edit — edit the document visually"
            leftPadding: 6; rightPadding: 6
            grouped: true; width: 80
            checked: root.layoutMode !== 2 && root.visualEditing
            Accessible.checkable: true; Accessible.checked: checked
            onClicked: root.editingRequested(true)
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
            enabled: root.canSplit
            checked: root.layoutMode === 1
            Accessible.checkable: true; Accessible.checked: checked
            onClicked: root.layoutRequested(1)
        }
    }
}
