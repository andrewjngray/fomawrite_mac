import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Find in the PDF, with the document find bar's look: query, "3 of 17",
// previous / next and close. Return and Shift+Return move; Escape closes.
Rectangle {
    id: root
    objectName: "pdfFindBar"
    required property var reader
    property alias query: searchField.text
    function focusQuery() { searchField.forceActiveFocus(); searchField.selectAll(); }
    implicitHeight: 54
    height: implicitHeight
    radius: 10
    color: backend.palette.popover || backend.palette.panel
    border.color: backend.palette.border
    Accessible.name: "Find in PDF"
    Keys.onEscapePressed: function(event) { root.reader.closeFind(); event.accepted = true; }

    Timer { id: searchDelay; interval: 120; onTriggered: root.reader.runFind() }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6
        TextField {
            id: searchField
            objectName: "pdfFindField"
            Layout.fillWidth: true
            Layout.minimumWidth: 60
            Layout.preferredHeight: 34
            placeholderText: ""
            Text {
                anchors.fill: parent
                anchors.leftMargin: 12; anchors.rightMargin: 12
                verticalAlignment: Text.AlignVCenter
                text: "Find in PDF…"
                visible: searchField.text.length === 0
                color: backend.palette.muted
                font: searchField.font
                elide: Text.ElideRight
            }
            Accessible.name: "Find in PDF"
            Accessible.description: "Return moves to the next match; Shift Return moves to the previous match."
            font.family: Qt.application.font.family
            font.pixelSize: 13
            color: backend.palette.text
            selectionColor: backend.palette.selection
            selectedTextColor: "white"
            leftPadding: 12; rightPadding: 12
            selectByMouse: true
            clip: true
            background: Rectangle {
                radius: 8
                color: backend.palette.field
                border.width: searchField.activeFocus ? 2 : 1
                border.color: searchField.activeFocus ? backend.palette.focus : backend.palette.border
            }
            onTextEdited: searchDelay.restart()
            Keys.onReturnPressed: function(event) { root.move(event.modifiers & Qt.ShiftModifier ? -1 : 1); event.accepted = true; }
            Keys.onEnterPressed: function(event) { root.move(event.modifiers & Qt.ShiftModifier ? -1 : 1); event.accepted = true; }
            Keys.onEscapePressed: function(event) { root.reader.closeFind(); event.accepted = true; }
        }
        Label {
            objectName: "pdfFindStatus"
            Layout.minimumWidth: implicitWidth
            readonly property int count: root.reader.findResults.length
            text: root.query.length === 0 ? ""
                : count === 0 ? (root.reader.findSearching ? "Searching…" : "No matches")
                : (root.reader.findIndex >= 0 ? (root.reader.findIndex + 1) + " of " : "") + count + (root.reader.findSearching ? "…" : "")
            color: backend.palette.muted
            font.family: Qt.application.font.family
            font.pixelSize: 12
            Accessible.name: text
            Accessible.role: Accessible.StaticText
        }
        ToolbarGroup {
            Row {
                ToolbarButton {
                    objectName: "pdfFindPrevious"
                    iconName: "up"
                    grouped: true
                    hint: "Previous match"
                    iconColor: backend.palette.muted
                    enabled: root.reader.findResults.length > 0
                    onClicked: root.move(-1)
                }
                ToolbarButton {
                    objectName: "pdfFindNext"
                    iconName: "down"
                    grouped: true
                    hint: "Next match"
                    iconColor: backend.palette.muted
                    enabled: root.reader.findResults.length > 0
                    onClicked: root.move(1)
                }
            }
        }
        ToolbarButton {
            objectName: "pdfFindClose"
            iconName: "close"
            hint: "Close Find in PDF"
            iconColor: backend.palette.muted
            onClicked: root.reader.closeFind()
        }
    }
    // A query still waiting for its delay runs before the move.
    function move(direction) {
        if (searchDelay.running) { searchDelay.stop(); root.reader.runFind(); return; }
        root.reader.moveFind(direction);
    }
}
