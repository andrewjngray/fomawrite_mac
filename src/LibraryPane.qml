import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Controls.Basic as Basic
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

Rectangle {
    id: root
    required property var library
    required property var commands
    property alias showSortBar: displaySettings.showSortBar
    property alias showFilterBar: displaySettings.showFilterBar
    property alias showDates: displaySettings.showDates
    property alias showExcerpts: displaySettings.showExcerpts
    property url currentFile
    property bool darkMode: false
    Settings {
        id: displaySettings
        category: "libraryDisplay"
        property bool showSortBar: true
        property bool showFilterBar: true
        property bool showDates: false
        property bool showExcerpts: false
        property int compactListRevision: 0
        Component.onCompleted: {
            if (compactListRevision < 1) {
                showDates = false
                showExcerpts = false
                compactListRevision = 1
            }
        }
    }
    signal openRequested(url file)
    signal createRequested(string name, bool inNewWindow)
    color: backend.palette.panel
    objectName: "libraryPane"
    function chooseFolder() { folderDialog.open(); }
    function newDocument(inNewWindow) { newFileDialog.inNewWindow = !!inNewWindow; newFileDialog.open(); }
    function newFolder() { newFolderDialog.open(); }
    function showCurrentFile() {
        var index = library.showFile(currentFile);
        if (index >= 0) Qt.callLater(function() {
            fileList.currentIndex = index;
            fileList.positionViewAtIndex(index, ListView.Contain);
        });
        return index >= 0;
    }
    function showOptions(anchor) { folderMenu.parent = anchor; folderMenu.x = anchor.width - folderMenu.width; folderMenu.y = anchor.height + 3; folderMenu.open(); }

    LibraryContextMenu { id: contextMenu; library: root.library; commands: root.commands; darkMode: root.darkMode }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4
        RowLayout {
            objectName: "librarySortBar"
            visible: displaySettings.showSortBar
            Layout.fillWidth: true
            ChromeButton {
                id: sortButton
                objectName: "librarySort"
                implicitHeight: 24
                implicitWidth: sortLabel.implicitWidth + 34
                text: "Sort by " + ["Name", "Date Modified", "Date Created", "Extension"][root.library.sortMode]
                darkMode: root.darkMode
                onClicked: sortMenu.open()
                contentItem: RowLayout {
                    spacing: 3
                    Text { id: sortLabel; text: sortButton.text; font.family: Qt.platform.os === "osx" ? Qt.application.font.family : "Helvetica Neue"; font.pixelSize: 12; color: backend.palette.muted }
                    LineIcon { name: "down"; ink: backend.palette.muted; Layout.preferredWidth: 12; Layout.preferredHeight: 12 }
                }
                background: Rectangle { radius: height / 2; color: backend.palette.field; border.width: sortButton.activeFocus ? 1 : 0; border.color: backend.palette.focus }
            }
            Item { Layout.fillWidth: true }
            ChromeButton {
                objectName: "libraryPreviewToggle"
                text: "Previews"
                hint: "Show or hide file text previews"
                implicitHeight: 24
                darkMode: root.darkMode
                checkable: true
                checked: displaySettings.showExcerpts
                onClicked: root.commands.run("excerpts")
            }
        }
        Label {
            visible: root.library.error !== ""
            text: root.library.error
            wrapMode: Text.Wrap
            color: root.darkMode ? "#fca5a5" : "#b42318"
            Layout.fillWidth: true
            font.pixelSize: 12
        }
        ListView {
            id: fileList
            objectName: "libraryFiles"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.library.entries
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate {
                id: entry
                required property var modelData
                width: fileList.width
                height: modelData.directory || !displaySettings.showExcerpts ? 28 : 50
                leftPadding: 8 + modelData.depth * 16
                highlighted: !modelData.directory && modelData.url.toString() === root.currentFile.toString()
                Accessible.name: modelData.name
                Accessible.description: modelData.directory ? "Folder" : "Markdown or text document"
                onClicked: {
                    if (modelData.directory) root.library.toggleFolder(modelData.url)
                    else root.openRequested(modelData.url)
                }
                background: Rectangle {
                    radius: 7
                    color: entry.highlighted ? (backend.palette.hover)
                        : entry.hovered ? (backend.palette.hover) : "transparent"
                }
                contentItem: RowLayout {
                    spacing: 7
                    LineIcon {
                        name: entry.modelData.directory ? "folder" : "editor"
                        ink: entry.modelData.directory ? backend.palette.folder : backend.palette.muted
                        Layout.preferredWidth: 18
                        Layout.preferredHeight: 18
                    }
                    ColumnLayout {
                        spacing: 3
                        Layout.fillWidth: true
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: entry.modelData.name; font.pixelSize: 14; color: backend.palette.text; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            Label { visible: !entry.modelData.directory && displaySettings.showDates; text: entry.modelData.modified; font.pixelSize: 10; color: backend.palette.muted }
                        }
                        Label { visible: !entry.modelData.directory && displaySettings.showExcerpts; text: visible ? root.library.excerpt(entry.modelData.url) : ""; elide: Text.ElideRight; font.pixelSize: 11; color: backend.palette.muted; Layout.fillWidth: true }
                    }
                    LineIcon { visible: entry.modelData.directory; name: entry.modelData.expanded ? "down" : "right"; ink: "#92969e"; Layout.preferredWidth: 14; Layout.preferredHeight: 14 }
                }
                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: (eventPoint) => contextMenu.showFor(entry, {url: entry.modelData.url, name: entry.modelData.name, directory: entry.modelData.directory, available: true}, false, eventPoint.position)
                }
                Keys.onMenuPressed: contextMenu.showFor(entry, {url: modelData.url, name: modelData.name, directory: modelData.directory, available: true}, false, Qt.point(0, height))
                property bool tooltipReady: false
                onHoveredChanged: { tooltipReady=false; if (hovered) pathHoverDelay.restart(); else pathHoverDelay.stop(); }
                Timer { id: pathHoverDelay; interval: 2000; onTriggered: entry.tooltipReady=entry.hovered }
                ToolTip.delay: 0
                ToolTip.visible: hovered && tooltipReady
                ToolTip.text: modelData.url.toString()
            }
            Label {
                anchors.centerIn: parent
                width: parent.width - 20
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                visible: fileList.count === 0
                text: root.library.rootFolder.toString() === "" ? "Choose a folder to begin.\nYour files stay on your Mac." : "No Markdown or text files here.\nExpand a folder or create a document."
                color: backend.palette.muted
                font.pixelSize: 13
            }
        }
        Basic.TextField {
            id: filterField
            objectName: "libraryFilter"
            visible: displaySettings.showFilterBar
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.maximumWidth: Math.max(0, root.width - 16)
            implicitWidth: 1
            implicitHeight: 26
            placeholderText: "Filter files"
            padding: 4
            leftPadding: 10
            rightPadding: 10
            leftInset: 0
            rightInset: 0
            topInset: 0
            bottomInset: 0
            color: backend.palette.text
            placeholderTextColor: backend.palette.muted
            selectionColor: "#426da7"
            selectedTextColor: "white"
            font.pixelSize: 12
            text: root.library.filter
            onTextEdited: root.library.filter = text
            Accessible.name: "Filter visible library files"
            background: Rectangle { radius: height / 2; color: backend.palette.field; border.width: filterField.activeFocus ? 1 : 0; border.color: backend.palette.focus }
            Keys.onEscapePressed: { text = ""; root.library.filter = ""; }
        }
    }

    component LibrarySortMenu: CompactMenu {
        ButtonGroup { id: sortFieldGroup }
        ButtonGroup { id: sortDirectionGroup }
        darkMode: root.darkMode
        CompactMenuItem { text: "Date Modified"; ButtonGroup.group: sortFieldGroup; checkable: true; checked: root.library.sortMode === 1; onTriggered: root.commands.run("sortModified") }
        CompactMenuItem { text: "Date Created"; ButtonGroup.group: sortFieldGroup; checkable: true; checked: root.library.sortMode === 2; onTriggered: root.commands.run("sortCreated") }
        CompactMenuItem { text: "Name"; ButtonGroup.group: sortFieldGroup; checkable: true; checked: root.library.sortMode === 0; onTriggered: root.commands.run("sortName") }
        CompactMenuItem { text: "Extension"; ButtonGroup.group: sortFieldGroup; checkable: true; checked: root.library.sortMode === 3; onTriggered: root.commands.run("sortExtension") }
        MenuSeparator { padding: 4; implicitHeight: 9; contentItem: Rectangle { implicitHeight: 1; color: backend.palette.border } }
        CompactMenuItem { text: "A to Z"; ButtonGroup.group: sortDirectionGroup; checkable: true; checked: root.library.ascending; onTriggered: root.commands.run("ascending") }
        CompactMenuItem { text: "Z to A"; ButtonGroup.group: sortDirectionGroup; checkable: true; checked: !root.library.ascending; onTriggered: root.commands.run("descending") }
        MenuSeparator { padding: 4; implicitHeight: 9; contentItem: Rectangle { implicitHeight: 1; color: backend.palette.border } }
        CompactMenuItem { text: "Pin Folders to Top"; checkable: true; checked: root.library.foldersFirst; onTriggered: root.commands.run("foldersFirst") }
        MenuSeparator { padding: 4; implicitHeight: 9; contentItem: Rectangle { implicitHeight: 1; color: backend.palette.border } }
        CompactMenuItem { text: "Show Date"; checkable: true; checked: displaySettings.showDates; onTriggered: root.commands.run("dates") }
        CompactMenuItem { text: "Show Text Excerpts"; checkable: true; checked: displaySettings.showExcerpts; onTriggered: root.commands.run("excerpts") }
    }
    LibrarySortMenu {
        id: sortMenu
        objectName: "librarySortMenu"
        parent: sortButton
        y: sortButton.height + 3
    }
    CompactMenu {
        id: folderMenu
        objectName: "libraryOptionsMenu"
        darkMode: root.darkMode
        CompactMenuItem { text: "New File"; iconName: "plus"; enabled: root.library.rootFolder.toString() !== ""; onTriggered: root.newDocument(false) }
        CompactMenuItem { text: "New Folder"; iconName: "folder"; enabled: root.library.rootFolder.toString() !== ""; onTriggered: root.newFolder() }
        MenuSeparator {}
        LibrarySortMenu { title: "Sort By"; objectName: "libraryOptionsSortMenu" }
        CompactMenu {
            title: "View Options"
            darkMode: root.darkMode
            CompactMenuItem { text: "Show Date"; checkable: true; checked: displaySettings.showDates; onTriggered: root.commands.run("dates") }
            CompactMenuItem { text: "Show Text Excerpts"; checkable: true; checked: displaySettings.showExcerpts; onTriggered: root.commands.run("excerpts") }
        }
        MenuSeparator {}
        CompactMenuItem { objectName: "toggleSortBar"; iconName: "sort"; text: displaySettings.showSortBar ? "Hide Sort Bar" : "Show Sort Bar"; onTriggered: root.commands.run("sortBar") }
        CompactMenuItem { objectName: "toggleFilterBar"; iconName: "filter"; text: displaySettings.showFilterBar ? "Hide Filter Bar" : "Show Filter Bar"; onTriggered: root.commands.run("filterBar") }
        MenuSeparator {}
        CompactMenuItem { text: "Choose Folder…"; onTriggered: folderDialog.open() }
        CompactMenuItem { text: "Refresh"; onTriggered: root.library.refresh() }
    }
    Dialogs.FolderDialog {
        id: folderDialog
        title: "Choose your writing folder"
        onAccepted: root.library.rootFolder = selectedFolder
    }
    Dialog {
        id: newFileDialog
        objectName: "newLibraryFileDialog"
        property bool inNewWindow: false
        title: "New document"
        anchors.centerIn: parent
        modal: true
        width: 290
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: { newFileName.text = "Untitled.md"; newFileName.forceActiveFocus(); newFileName.selectAll(); }
        onAccepted: root.createRequested(newFileName.text, inNewWindow)
        TextField { id: newFileName; width: parent.width; placeholderText: "Document.md"; onAccepted: newFileDialog.accept() }
    }
    Dialog {
        id: newFolderDialog
        title: "New folder"
        anchors.centerIn: parent
        modal: true
        width: 290
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: { newFolderName.text = ""; newFolderName.forceActiveFocus(); }
        onAccepted: root.library.createFolder(newFolderName.text)
        TextField { id: newFolderName; width: parent.width; placeholderText: "Folder name"; onAccepted: newFolderDialog.accept() }
    }
}
