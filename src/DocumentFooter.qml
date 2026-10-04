import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// One document-level bar survives every arrangement of the writing panes.
WorkspaceFooter {
    id: root
    property int layoutMode: 0
    property bool visualEditing: false
    property bool canSplit: true
    property string writingAppearance: "Manuscript"
    property string previewTemplate: "Modern (Sans)"
    property string statusText: ""
    property bool showStatus: false
    property bool showStatistics: false
    property string statisticsText: ""
    // Use the width available with the requested navigation columns so
    // switching views cannot expand/collapse the style controls by itself.
    property real styleWidthBudget: width
    readonly property bool compact: Math.min(width, styleWidthBudget) < 660
    readonly property bool statusVisible: statusLabel.visible
    property alias stylesAnchor: stylesButton
    signal viewRequested(int mode)
    signal visualEditRequested()
    signal appearanceMenuRequested(var anchor)
    signal templateMenuRequested(var anchor)
    signal stylesMenuRequested(var anchor)
    signal statisticsRequested()

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12; anchors.rightMargin: 12
        spacing: 6
        Label {
            id: statusLabel
            objectName: "sourceFooterStatus"
            visible: root.showStatus && !root.showStatistics && root.width >= 880
            text: root.statusText
            font.family: Qt.application.font.family; font.pixelSize: 13
            color: backend.palette.muted
            elide: Text.ElideRight
            Layout.fillWidth: true; Layout.minimumWidth: 0
            Accessible.name: "Document status: " + text
        }
        FooterButton {
            objectName: "toolbarStatistic"
            visible: root.showStatistics && root.width >= 880
            text: root.statisticsText; hint: text
            Layout.fillWidth: true; Layout.minimumWidth: 34
            onClicked: root.statisticsRequested()
        }
        Item { Layout.fillWidth: true; visible: !statusLabel.visible && !(root.showStatistics && root.width >= 880) }
        FooterButton {
            objectName: "sourceAppearanceButton"
            visible: !root.compact
            text: root.writingAppearance; menuIndicator: true
            hint: "Choose writing appearance"
            Layout.preferredWidth: 128
            onClicked: root.appearanceMenuRequested(this)
        }
        FooterButton {
            objectName: "previewTemplateButton"
            visible: !root.compact
            text: "Preview · " + root.previewTemplate; menuIndicator: true
            hint: "Choose preview template: " + root.previewTemplate
            Layout.preferredWidth: 200
            onClicked: root.templateMenuRequested(this)
        }
        FooterButton {
            id: stylesButton
            objectName: "footerStylesButton"
            visible: root.compact
            leftPadding: 6; rightPadding: 6
            text: "Aa"; hint: "Writing appearance and preview template"
            Layout.preferredWidth: 34
            onClicked: root.stylesMenuRequested(this)
        }
        FooterButton {
            objectName: "visualEditToggle"
            leftPadding: 8; rightPadding: 8
            text: "Visual Edit"
            hint: root.visualEditing ? "Turn off visual editing" : "Edit the rendered document"
            checked: root.visualEditing
            Accessible.checkable: true; Accessible.checked: checked
            Layout.preferredWidth: 84
            onClicked: root.visualEditRequested()
        }
        ToolbarGroup {
            objectName: "documentViewControls"
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
}
