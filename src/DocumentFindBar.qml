import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Find controls remain in place while the query and result count change.
Item {
    id: root
    objectName: "documentFindBar"
    property alias query: searchField.text
    property alias replacement: replaceField.text
    property bool replaceVisible: false
    property int matchCount: 0
    property int matchIndex: -1
    readonly property bool compact: width < 540
    signal queryEdited()
    signal moveRequested(int direction)
    signal replaceRequested(bool all)
    signal replaceToggled()
    signal closeRequested()
    implicitHeight: replaceVisible ? 118 : 76
    height: implicitHeight
    Accessible.name: "Find in document"
    function focusQuery() { searchField.forceActiveFocus(); searchField.selectAll(); }
    function focusReplacement() { replaceField.forceActiveFocus(); replaceField.selectAll(); }
    Keys.onEscapePressed: function(event) { root.closeRequested(); event.accepted = true; }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            TextField {
                id: searchField
                objectName: "searchField"
                Layout.fillWidth: true
                Layout.minimumWidth: 80
                Layout.preferredHeight: 34
                // Draw a stable inline prompt; Material's floating label crosses
                // the border of this compact workspace field.
                placeholderText: ""
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 12; anchors.rightMargin: 12
                    verticalAlignment: Text.AlignVCenter
                    text: "Find in document…"
                    visible: searchField.text.length === 0
                    color: backend.palette.muted
                    font: searchField.font
                    elide: Text.ElideRight
                }
                Accessible.name: "Find in document"
                Accessible.description: "Search Markdown source. Return moves to the next match; Shift Return moves to the previous match."
                font.family: Qt.application.font.family
                font.pixelSize: 13
                color: backend.palette.text
                placeholderTextColor: backend.palette.muted
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
                onTextChanged: root.queryEdited()
                Keys.onReturnPressed: function(event) { root.moveRequested(event.modifiers & Qt.ShiftModifier ? -1 : 1); event.accepted = true; }
                Keys.onEnterPressed: function(event) { root.moveRequested(event.modifiers & Qt.ShiftModifier ? -1 : 1); event.accepted = true; }
                Keys.onEscapePressed: function(event) { root.closeRequested(); event.accepted = true; }
            }
            ToolbarGroup {
                Row {
                    SearchIconButton {
                        objectName: "findPreviousButton"
                        iconName: "up"
                        grouped: true
                        enabled: root.matchCount > 0
                        onClicked: root.moveRequested(-1)
                    }
                    SearchIconButton {
                        objectName: "findNextButton"
                        iconName: "down"
                        grouped: true
                        enabled: root.matchCount > 0
                        onClicked: root.moveRequested(1)
                    }
                }
            }
            SearchIconButton {
                objectName: "findCloseButton"
                iconName: "close"
                onClicked: root.closeRequested()
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label {
                objectName: "searchMatchStatus"
                Layout.fillWidth: true
                text: root.query.length === 0 ? "Search this document"
                    : root.matchCount === 0 ? "No matches"
                    : (root.matchIndex + 1) + " of " + root.matchCount + (root.matchCount === 1 ? " match" : " matches")
                color: backend.palette.muted
                font.family: Qt.application.font.family
                font.pixelSize: 12
                elide: Text.ElideRight
                Accessible.name: text
                Accessible.role: Accessible.StaticText
            }
            ToolbarButton {
                objectName: "findReplaceToggle"
                text: "Replace"
                hint: root.replaceVisible ? "Hide replacement field" : "Show replacement field"
                checked: root.replaceVisible
                onClicked: root.replaceToggled()
            }
        }
        RowLayout {
            visible: root.replaceVisible
            Layout.fillWidth: true
            spacing: 6
            TextField {
                id: replaceField
                objectName: "replaceField"
                Layout.fillWidth: true
                Layout.minimumWidth: 80
                Layout.preferredHeight: 34
                // Draw a stable inline prompt; Material's floating label crosses
                // the border of this compact workspace field.
                placeholderText: ""
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 12; anchors.rightMargin: 12
                    verticalAlignment: Text.AlignVCenter
                    text: "Replace with…"
                    visible: replaceField.text.length === 0
                    color: backend.palette.muted
                    font: replaceField.font
                    elide: Text.ElideRight
                }
                Accessible.name: "Replace with"
                Accessible.description: "Return replaces the current match; Escape closes Find."
                font.family: Qt.application.font.family
                font.pixelSize: 13
                color: backend.palette.text
                placeholderTextColor: backend.palette.muted
                selectionColor: backend.palette.selection
                selectedTextColor: "white"
                leftPadding: 12; rightPadding: 12
                selectByMouse: true
                clip: true
                background: Rectangle {
                    radius: 8
                    color: backend.palette.field
                    border.width: replaceField.activeFocus ? 2 : 1
                    border.color: replaceField.activeFocus ? backend.palette.focus : backend.palette.border
                }
                Keys.onReturnPressed: function(event) { if (root.matchCount > 0) root.replaceRequested(false); event.accepted = true; }
                Keys.onEnterPressed: function(event) { if (root.matchCount > 0) root.replaceRequested(false); event.accepted = true; }
                Keys.onEscapePressed: function(event) { root.closeRequested(); event.accepted = true; }
            }
            ToolbarButton {
                objectName: "replaceCurrentButton"
                text: "Replace"
                hint: "Replace current match"
                enabled: root.matchCount > 0
                onClicked: root.replaceRequested(false)
            }
            ToolbarButton {
                objectName: "replaceAllButton"
                text: root.compact ? "All" : "Replace all"
                hint: "Replace all matches in this document"
                enabled: root.matchCount > 0
                onClicked: root.replaceRequested(true)
            }
        }
    }
}
