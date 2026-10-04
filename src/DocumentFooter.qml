import QtQuick
import QtQuick.Controls

// Pane tools follow the divider; document view controls retain one fixed home.
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
    readonly property real paneStart: hasPreview ? previewPaneStart : 0
    readonly property real paneRoom: Math.max(0, viewControls.x - 6 - paneStart - 12)
    readonly property bool compactVisual: paneRoom < (hasPreview ? 250 : 218)
    readonly property bool statusVisible: statusLabel.visible
    readonly property real statusStart: (hasPreview && layoutMode === 2 ? templateButton.x + templateButton.width
                                                               : appearanceButton.x + appearanceButton.width) + 12
    readonly property real statusEnd: layoutMode === 1 ? sourcePaneWidth - 12 : visualButton.x - 12
    readonly property real statusWidth: Math.max(0, statusEnd - statusStart)
    signal viewRequested(int mode)
    signal visualEditRequested()
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
        visible: root.layoutMode !== 2
        x: 12; anchors.verticalCenter: parent.verticalCenter
        width: Math.min(128, Math.max(34, (root.hasPreview ? root.sourcePaneWidth - 12 : visualButton.x - 6) - x))
        text: width >= 110 ? root.writingAppearance : "Aa"
        menuIndicator: width >= 60
        alignLeft: width >= 110
        hint: "Choose Source writing appearance: " + root.writingAppearance
        onClicked: root.appearanceMenuRequested(this)
    }
    FooterButton {
        id: templateButton
        objectName: "previewTemplateButton"
        visible: root.hasPreview
        x: root.previewPaneStart + 12; anchors.verticalCenter: parent.verticalCenter
        width: Math.min(200, Math.max(34, root.paneRoom - visualButton.width - 6))
        text: width >= 170 ? "Preview · " + root.previewTemplate : width >= 70 ? root.previewTemplate : "Aa"
        menuIndicator: width >= 60
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
    FooterButton {
        id: visualButton
        objectName: "visualEditToggle"
        width: root.compactVisual ? 34 : 84
        anchors.verticalCenter: parent.verticalCenter
        x: root.hasPreview
            ? Math.max(templateButton.x + templateButton.width + 6,
                       Math.min(root.previewPaneStart + (root.width - root.previewPaneStart - width) / 2,
                                viewControls.x - 6 - width))
            : viewControls.x - 6 - width
        leftPadding: root.compactVisual ? 8 : 6; rightPadding: leftPadding
        text: root.compactVisual ? "" : "Visual Edit"
        iconName: root.compactVisual ? "compose" : ""
        hint: root.visualEditing ? "Turn off Visual Edit" : "Visual Edit — edit the rendered document"
        checked: root.visualEditing
        Accessible.checkable: true; Accessible.checked: checked
        onClicked: root.visualEditRequested()
    }
    ToolbarGroup {
        id: viewControls
        objectName: "documentViewControls"
        anchors.right: parent.right; anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        FooterButton {
            objectName: "sourceModeButton"
            text: "Source"; hint: "Show Markdown source"
            leftPadding: 8; rightPadding: 8
            grouped: true; width: 62
            checked: root.layoutMode === 0
            Accessible.checkable: true; Accessible.checked: checked
            onClicked: root.viewRequested(0)
        }
        FooterButton {
            objectName: "previewSplitButton"
            text: "Split"
            hint: root.canSplit ? "Show source and rendered document side by side" : "Widen the window to use Split view"
            leftPadding: 8; rightPadding: 8
            grouped: true; width: 50
            enabled: root.canSplit
            checked: root.layoutMode === 1
            Accessible.checkable: true; Accessible.checked: checked
            onClicked: root.viewRequested(1)
        }
        FooterButton {
            objectName: "previewFullButton"
            text: "Full"; hint: "Show the rendered document at full width"
            leftPadding: 8; rightPadding: 8
            grouped: true; width: 46
            checked: root.layoutMode === 2
            Accessible.checkable: true; Accessible.checked: checked
            onClicked: root.viewRequested(2)
        }
    }
}
