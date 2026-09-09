import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Drawer {
    id: root
    objectName: "documentOutline"
    property var headings: []
    signal jumpRequested(int position)
    width: Math.min(340, parent.width * 0.8)
    height: parent.height
    edge: Qt.LeftEdge
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        Label { text: "Document outline"; font.pixelSize: 18; font.bold: true }
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.headings
            ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate {
                required property var modelData
                width: ListView.view.width
                text: modelData.title
                leftPadding: 12 + (modelData.level - 1) * 12
                onClicked: { root.jumpRequested(modelData.position); root.close(); }
                ToolTip.visible: hovered
                ToolTip.text: modelData.title
            }
            Label {
                anchors.centerIn: parent
                width: parent.width
                visible: root.headings.length === 0
                text: "Add Markdown headings to navigate your document."
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
