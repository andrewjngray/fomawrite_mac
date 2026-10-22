import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

// A read-only publishing consumer. Opening this window never initializes a
// printer or changes the document workspace's layout, format or zoom settings.
Window {
    id: root
    objectName: "windowsPaginatedPreview"
    required property var renderer
    property string markdown: ""
    property url documentBaseUrl
    property int zoomPercent: 100
    function focusPreview() { preview.focusRenderedSurface(); }
    signal dismissed()
    title: "Fomawrite — Paginated Preview"
    width: 900
    height: 700
    minimumWidth: 420
    minimumHeight: 320
    flags: Qt.Dialog | Qt.WindowTitleHint | Qt.WindowSystemMenuHint | Qt.WindowCloseButtonHint
    modality: Qt.ApplicationModal
    color: renderer.palette.page
    onClosing: {
        preview.visible = false;
        preview.cancelRequest();
        dismissed();
    }
    Shortcut { sequence: "Escape"; context: Qt.WindowShortcut; onActivated: root.close() }
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        // A transient Qt Quick window can otherwise leave its owner's
        // WindowShortcut bindings eligible. This read-only viewer owns input.
        Keys.onShortcutOverride: function(event) {
            var systemClose = event.key === Qt.Key_F4 && (event.modifiers & Qt.AltModifier);
            if (event.key !== Qt.Key_Escape && !systemClose) event.accepted = true;
        }
        Keys.onPressed: function(event) {
            if (event.matches(StandardKey.Close)) {
                event.accepted = true;
                root.close();
            }
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 44
            color: root.renderer.palette.panel
            RowLayout {
                anchors.centerIn: parent
                spacing: 6
                ToolbarButton {
                    objectName: "paginatedZoomOut"
                    iconName: "minus"
                    hint: "Decrease preview zoom"
                    enabled: root.zoomPercent > 75
                    onClicked: root.zoomPercent = Math.max(75, root.zoomPercent - 10)
                }
                Label {
                    objectName: "paginatedZoomLabel"
                    text: root.zoomPercent + "%"
                    color: root.renderer.palette.text
                    horizontalAlignment: Text.AlignHCenter
                    Layout.minimumWidth: 48
                }
                ToolbarButton {
                    objectName: "paginatedZoomIn"
                    iconName: "plus"
                    hint: "Increase preview zoom"
                    enabled: root.zoomPercent < 200
                    onClicked: root.zoomPercent = Math.min(200, root.zoomPercent + 10)
                }
            }
        }
        PublishingPreview {
            id: preview
            objectName: "paginatedPdfPreview"
            Layout.fillWidth: true
            Layout.fillHeight: true
            renderer: root.renderer
            markdown: root.markdown
            documentBaseUrl: root.documentBaseUrl
            publishingMode: "pdf"
            zoom: root.zoomPercent / 100
            visible: root.visible
        }
    }
}
