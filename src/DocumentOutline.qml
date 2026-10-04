import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Drawer {
    id: root
    objectName: "documentOutline"
    property var headings: []
    property int currentPosition: -1
    property var previousFocusItem: null
    property int pendingPosition: -1
    readonly property string searchQuery: outlineSearch.text.trim().toLocaleLowerCase()
    readonly property var filteredHeadings: {
        var terms = searchQuery.split(/\s+/);
        return headings.filter(function(heading) {
            var title = String(heading.title).toLocaleLowerCase();
            return terms.every(function(term) { return title.indexOf(term) !== -1; });
        });
    }
    readonly property int currentHeadingPosition: {
        var position = -1;
        for (var i = 0; i < headings.length; ++i) {
            if (headings[i].position > currentPosition) break;
            position = headings[i].position;
        }
        return position;
    }
    readonly property int minimumHeadingLevel: {
        var level = 6;
        for (var i = 0; i < headings.length; ++i) level = Math.min(level, headings[i].level);
        return level;
    }
    signal jumpRequested(int position)
    signal focusRestoreRequested()

    function selectCurrentHeading() {
        if (!headingList) return;
        var selected = filteredHeadings.length > 0 ? 0 : -1;
        for (var i = 0; i < filteredHeadings.length; ++i)
            if (filteredHeadings[i].position === currentHeadingPosition) selected = i;
        headingList.currentIndex = selected;
        if (selected >= 0) headingList.positionViewAtIndex(selected, ListView.Contain);
    }
    function showFor(position, returnFocusItem) {
        currentPosition = position;
        previousFocusItem = returnFocusItem;
        pendingPosition = -1;
        outlineSearch.text = "";
        selectCurrentHeading();
        open();
    }
    function moveSelection(offset) {
        if (filteredHeadings.length === 0) return;
        headingList.currentIndex = Math.max(0, Math.min(filteredHeadings.length - 1, headingList.currentIndex + offset));
        headingList.positionViewAtIndex(headingList.currentIndex, ListView.Contain);
    }
    function activateSelection() {
        if (headingList.currentIndex < 0 || headingList.currentIndex >= filteredHeadings.length) return;
        // Wait for the drawer to close before handing focus to the editor.
        pendingPosition = filteredHeadings[headingList.currentIndex].position;
        close();
    }
    onFilteredHeadingsChanged: selectCurrentHeading()
    width: Math.min(360, parent.width * 0.8)
    height: parent.height
    edge: Qt.LeftEdge
    focus: true
    background: Rectangle {
        color: backend.palette.panel
        border.color: backend.palette.divider
        border.width: 1
    }
    onOpened: {
        selectCurrentHeading();
        outlineSearch.forceActiveFocus();
    }
    onClosed: {
        var position = pendingPosition;
        pendingPosition = -1;
        if (position >= 0) jumpRequested(position);
        else if (previousFocusItem && previousFocusItem.visible && previousFocusItem.enabled)
            previousFocusItem.forceActiveFocus();
        else focusRestoreRequested();
        previousFocusItem = null;
    }
    ColumnLayout {
        Accessible.name: "Document outline"
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label {
                text: "Document outline"
                color: backend.palette.text
                font.family: Qt.application.font.family
                font.pixelSize: 14
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            ToolbarButton {
                id: outlineCloseButton
                objectName: "documentOutlineCloseButton"
                iconName: "close"
                hint: "Close document outline"
                implicitWidth: 34
                onClicked: root.close()
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            TextField {
                id: outlineSearch
                objectName: "documentOutlineSearch"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredHeight: 34
                // Draw a stable inline prompt; Material's floating label crosses
                // the border of this compact workspace field.
                placeholderText: ""
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 12; anchors.rightMargin: 12
                    verticalAlignment: Text.AlignVCenter
                    text: "Find a heading…"
                    visible: outlineSearch.text.length === 0
                    color: backend.palette.muted
                    font: outlineSearch.font
                    elide: Text.ElideRight
                }
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
                    border.width: outlineSearch.activeFocus ? 2 : 1
                    border.color: outlineSearch.activeFocus ? backend.palette.focus : backend.palette.border
                }
                Accessible.name: "Find a heading"
                Accessible.description: "Filter document headings. Use arrow keys to choose a result and Return to jump."
                onAccepted: root.activateSelection()
                Keys.onDownPressed: function(event) { root.moveSelection(1); event.accepted = true; }
                Keys.onUpPressed: function(event) { root.moveSelection(-1); event.accepted = true; }
                Keys.onEscapePressed: function(event) { root.close(); event.accepted = true; }
            }
            ToolbarButton {
                objectName: "documentOutlineClearSearch"
                iconName: "close"
                hint: "Clear heading search"
                implicitWidth: 34
                enabled: outlineSearch.text.length > 0
                onClicked: { outlineSearch.clear(); outlineSearch.forceActiveFocus(); }
            }
        }
        Label {
            objectName: "documentOutlineCount"
            text: root.searchQuery.length > 0
                ? root.filteredHeadings.length + " of " + root.headings.length + " headings"
                : root.headings.length + (root.headings.length === 1 ? " heading" : " headings")
            color: backend.palette.muted
            font.family: Qt.application.font.family
            font.pixelSize: 12
            Layout.fillWidth: true
        }
        ListView {
            id: headingList
            objectName: "documentOutlineList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.filteredHeadings
            spacing: 2
            activeFocusOnTab: true
            keyNavigationEnabled: false
            Accessible.name: "Document headings"
            Keys.onDownPressed: function(event) { root.moveSelection(1); event.accepted = true; }
            Keys.onUpPressed: function(event) { root.moveSelection(-1); event.accepted = true; }
            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Home) {
                    currentIndex = count > 0 ? 0 : -1;
                    positionViewAtBeginning();
                    event.accepted = true;
                } else if (event.key === Qt.Key_End) {
                    currentIndex = count - 1;
                    positionViewAtEnd();
                    event.accepted = true;
                }
            }
            Keys.onReturnPressed: function(event) { root.activateSelection(); event.accepted = true; }
            Keys.onEnterPressed: function(event) { root.activateSelection(); event.accepted = true; }
            Keys.onEscapePressed: function(event) { root.close(); event.accepted = true; }
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            delegate: ItemDelegate {
                id: headingRow
                required property var modelData
                required property int index
                readonly property bool currentSection: modelData.position === root.currentHeadingPosition
                objectName: "documentOutlineHeading_" + index
                width: ListView.view.width - 12
                implicitHeight: 38
                text: modelData.title
                leftPadding: 10 + (modelData.level - root.minimumHeadingLevel) * 12
                rightPadding: 10
                font.family: Qt.application.font.family
                font.pixelSize: 13
                font.weight: modelData.level === root.minimumHeadingLevel ? Font.DemiBold : Font.Normal
                focusPolicy: Qt.NoFocus
                highlighted: headingList.currentIndex === index
                Accessible.name: "Heading level " + modelData.level + ": " + modelData.title + (currentSection ? ", current section" : "")
                Accessible.selected: highlighted
                contentItem: RowLayout {
                    spacing: 8
                    Label {
                        text: "H" + headingRow.modelData.level
                        color: backend.palette.muted
                        font.family: Qt.application.font.family
                        font.pixelSize: 10
                        Layout.preferredWidth: 19
                    }
                    Text {
                        text: headingRow.text
                        font: headingRow.font
                        color: backend.palette.text
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Rectangle {
                        visible: headingRow.currentSection
                        width: 5; height: 5; radius: 2.5
                        color: backend.palette.focus
                    }
                }
                background: Rectangle {
                    radius: 6
                    color: headingRow.down ? backend.palette.controlPressed
                        : headingRow.highlighted ? backend.palette.selectedRow
                        : headingRow.hovered ? backend.palette.controlHover : "transparent"
                    border.width: headingRow.highlighted && headingList.activeFocus ? 2 : 0
                    border.color: backend.palette.focus
                }
                onClicked: { headingList.currentIndex = index; root.activateSelection(); }
                ToolTip.visible: hovered
                ToolTip.delay: 700
                ToolTip.text: modelData.title + (currentSection ? " — current section" : "")
            }
            Label {
                objectName: "documentOutlineEmptyState"
                anchors.centerIn: parent
                width: Math.max(0, parent.width - 16)
                visible: root.filteredHeadings.length === 0
                text: root.headings.length === 0 ? "Add Markdown headings to navigate your document."
                    : "No headings match your search."
                color: backend.palette.muted
                font.family: Qt.application.font.family
                font.pixelSize: 13
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
