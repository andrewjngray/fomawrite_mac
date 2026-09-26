import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: hub
    objectName: "exportHub"
    required property var backend
    required property var renderer
    required property string markdown
    required property url documentBaseUrl
    required property bool darkMode
    property string selectedFormat: "pdf"
    property string cssName: backend.outputCssName()
    property string cssFeedback: ""
    property bool compact: width < 760
    signal destinationRequested(string format)
    Connections {
        target: hub.backend
        function onOutputCssChanged() { hub.cssName = hub.backend.outputCssName() }
    }
    FileDialog {
        id: cssPicker
        title: "Choose a local CSS file for HTML output"
        nameFilters: ["CSS files (*.css)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            if (hub.backend.loadOutputCss(selectedFile))
                hub.cssFeedback = "CSS applied to HTML export only."
            else
                hub.cssFeedback = hub.backend.status
        }
    }

    title: "Export and share"
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    anchors.centerIn: parent
    width: Math.min(Math.max(500, parent.width - 32), 1060)
    height: Math.min(Math.max(470, parent.height - 32), 680)
    padding: 0
    standardButtons: Dialog.NoButton
    background: Rectangle { color: backend.palette.panel; border.color: backend.palette.border; radius: 12 }
    header: Rectangle {
        implicitHeight: 76; color: "transparent"
        Label {
            x: 22; y: 14; text: hub.title
            color: backend.palette.text; font.pixelSize: 20; font.weight: Font.DemiBold
            Accessible.role: Accessible.Heading
        }
        Label {
            x: 22; y: 44; text: "Choose a format and style, then save or share."
            color: backend.palette.muted; font.pixelSize: 13
        }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: backend.palette.border }
    }
    contentItem: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 20; spacing: 18
            ColumnLayout {
                Layout.preferredWidth: hub.compact ? -1 : 250
                Layout.fillWidth: hub.compact; Layout.alignment: Qt.AlignTop; spacing: 12
                Label { text: "Output"; color: backend.palette.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                ComboBox {
                    objectName: "exportFormatChoice"; Layout.fillWidth: true
                    model: [{ label: "PDF document", value: "pdf" }, { label: "HTML document", value: "html" }]
                    textRole: "label"; valueRole: "value"; currentIndex: hub.selectedFormat === "html" ? 1 : 0
                    onActivated: hub.selectedFormat = currentValue; Accessible.name: "Export format"
                }
                Label { text: "Selected style"; color: backend.palette.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                Label { Layout.fillWidth: true; text: backend.outputTemplateName + " · " + backend.outputFont + " · " + backend.outputPointSize + " pt"; wrapMode: Text.Wrap; color: backend.palette.muted; font.pixelSize: 13; Accessible.name: "Current output style: " + text }
                StyleGallery { visible: hub.compact; Layout.fillWidth: true; Layout.preferredHeight: 260; backend: hub.backend; onStyleChanged: hub.forceActiveFocus() }
                ColumnLayout {
                    visible: hub.selectedFormat === "html"
                    Layout.fillWidth: true; spacing: 4
                    Label { text: "Custom HTML CSS: " + hub.cssName; Layout.fillWidth: true; elide: Text.ElideMiddle; color: backend.palette.muted; font.pixelSize: 11; Accessible.name: text }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "Choose CSS…"; onClicked: cssPicker.open(); Accessible.name: "Choose local CSS for HTML export" }
                        Button { text: "Clear"; enabled: hub.cssName !== "None"; onClicked: { backend.clearOutputCss(); hub.cssFeedback = "" }
                            Accessible.name: "Clear HTML CSS" }
                    }
                    Label { visible: hub.cssFeedback.length > 0; text: hub.cssFeedback; Layout.fillWidth: true; wrapMode: Text.Wrap; color: backend.palette.muted; font.pixelSize: 11 }
                }
                Label { text: "Paper"; color: backend.palette.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                ComboBox {
                    objectName: "exportPaperChoice"; Layout.fillWidth: true
                    model: [{ label: "A4", value: "a4" }, { label: "US Letter", value: "letter" }, { label: "US Legal", value: "legal" }, { label: "A5", value: "a5" }]
                    textRole: "label"; valueRole: "value"
                    currentIndex: { for (var i = 0; i < model.length; ++i) if (model[i].value === backend.exportPaperSize()) return i; return 0; }
                    onActivated: backend.setExportPaperSize(currentValue); Accessible.name: "Paper size"
                }
                Label { text: "Orientation"; color: backend.palette.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                RowLayout {
                    Layout.fillWidth: true
                    ButtonGroup { id: orientationGroup }
                    RadioButton { text: "Portrait"; ButtonGroup.group: orientationGroup; checked: backend.exportOrientation() === "portrait"; onClicked: backend.setExportOrientation("portrait"); Accessible.name: "Portrait orientation" }
                    RadioButton { text: "Landscape"; ButtonGroup.group: orientationGroup; checked: backend.exportOrientation() === "landscape"; onClicked: backend.setExportOrientation("landscape"); Accessible.name: "Landscape orientation" }
                }
                Label { Layout.fillWidth: true; text: selectedFormat === "html" ? "HTML uses the selected output style. Paper settings apply when printing it." : "PDF uses the selected style, paper size and orientation."; wrapMode: Text.Wrap; color: backend.palette.muted; font.pixelSize: 12 }
                Button { visible: hub.compact; Layout.fillWidth: true; text: "Open paginated preview…"; flat: true; onClicked: backend.printPreview(); Accessible.name: "Open paginated print preview" }
            }
            Rectangle { visible: !hub.compact; Layout.fillHeight: true; Layout.preferredWidth: 1; color: backend.palette.border }
            StyleGallery { visible: !hub.compact; Layout.preferredWidth: 245; Layout.fillHeight: true; backend: hub.backend; onStyleChanged: hub.forceActiveFocus() }
            Rectangle { visible: !hub.compact; Layout.fillHeight: true; Layout.preferredWidth: 1; color: backend.palette.border }
            ColumnLayout {
                visible: !hub.compact; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8
                Label { text: "Live output preview"; color: backend.palette.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true; color: backend.palette.page; border.color: backend.palette.border; radius: 5; clip: true
                    PreviewPane { objectName: "exportHubPreviewPane"; visualEditorObjectName: "exportHubVisualEditor"; allowVisualEdit: false; anchors.fill: parent; renderer: hub.renderer; markdown: hub.markdown; documentBaseUrl: hub.documentBaseUrl; darkMode: hub.darkMode; typeface: backend.outputFont; textSize: backend.outputPointSize; layoutMode: 2; onLayoutRequested: function(mode) {}; onLinkRequested: function(link) {} }
                }
                Label { Layout.fillWidth: true; text: "Continuous preview of the current output style. Custom HTML CSS and exact page breaks appear only in saved output or paginated preview."; wrapMode: Text.Wrap; color: backend.palette.muted; font.pixelSize: 12 }
                Button { objectName: "exportPaginatedPreviewButton"; text: "Open paginated preview…"; flat: true; onClicked: backend.printPreview(); Accessible.name: "Open paginated print preview" }
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: backend.palette.border }
        RowLayout {
            Layout.fillWidth: true; Layout.margins: 20
            Button { objectName: "exportShareMarkdownButton"; text: "Share Markdown…"; flat: true; onClicked: backend.nativeWindowAction("share"); Accessible.name: "Share Markdown" }
            Item { Layout.fillWidth: true }
            Button { text: "Cancel"; flat: true; onClicked: hub.close(); Accessible.name: "Cancel export" }
            Button {
                id: saveAction
                objectName: "exportDestinationButton"
                text: selectedFormat === "pdf" ? "Save PDF…" : "Save HTML…"
                font.weight: Font.DemiBold
                leftPadding: 18; rightPadding: 18
                background: Rectangle {
                    radius: 7
                    color: saveAction.down ? "#204b7e" : saveAction.hovered ? "#3170b8" : "#285e9e"
                }
                contentItem: Text {
                    text: saveAction.text; font: saveAction.font; color: "white"
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: { hub.destinationRequested(hub.selectedFormat); hub.close(); }
                Accessible.name: text
            }
        }
    }
}
