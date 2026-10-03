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
    focus: true
    background: Rectangle {
        color: backend.palette.panel
        border.color: backend.palette.divider
        border.width: 1
    }
    onOpened: outlineCloseButton.forceActiveFocus()
    ColumnLayout {
        Accessible.name: "Document outline"
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label {
                text: "Document outline"
                color: backend.palette.text
                font.family: Qt.application.font.family
                font.pixelSize: 13
                font.weight: Font.Medium
                Layout.fillWidth: true
            }
            ToolbarButton {
                id: outlineCloseButton
                objectName: "documentOutlineCloseButton"
                iconName: "close"
                hint: "Close document outline"
                implicitWidth: 32
                onClicked: root.close()
            }
        }
        ListView {
            id: headingList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.headings
            ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate {
                id: headingRow
                required property var modelData
                width: ListView.view.width
                implicitHeight: 32
                text: modelData.title
                leftPadding: 12 + (modelData.level - 1) * 12
                rightPadding: 12
                font.family: Qt.application.font.family
                font.pixelSize: 13
                focusPolicy: Qt.StrongFocus
                Accessible.name: "Heading level " + modelData.level + ": " + modelData.title
                contentItem: Text {
                    text: headingRow.text
                    font: headingRow.font
                    color: backend.palette.text
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    radius: 6
                    color: headingRow.down ? backend.palette.controlPressed
                        : headingRow.highlighted ? backend.palette.selectedRow
                        : headingRow.hovered ? backend.palette.controlHover : "transparent"
                    border.width: headingRow.activeFocus ? 2 : 0
                    border.color: backend.palette.focus
                }
                onClicked: { root.jumpRequested(modelData.position); root.close(); }
                ToolTip.visible: hovered
                ToolTip.text: modelData.title
            }
            Label {
                anchors.centerIn: parent
                width: parent.width
                visible: root.headings.length === 0
                text: "Add Markdown headings to navigate your document."
                color: backend.palette.muted
                font.family: Qt.application.font.family
                font.pixelSize: 13
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
