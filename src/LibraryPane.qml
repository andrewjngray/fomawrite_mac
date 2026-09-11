import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

Rectangle {
    id: root
    required property var library
    property url currentFile
    property bool darkMode: false
    Settings {
        id: displaySettings
        category: "libraryDisplay"
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
    color: darkMode ? "#202124" : "#fbfbfc"
    objectName: "libraryPane"
    function chooseFolder() { folderDialog.open(); }
    function newDocument() { newFileDialog.open(); }
    function showOptions() { folderMenu.open(); }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4
        RowLayout {
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
                    Text { id: sortLabel; text: sortButton.text; font.family: "Helvetica Neue"; font.pixelSize: 12; color: root.darkMode ? "#c3c7ce" : "#777c83" }
                    LineIcon { name: "down"; ink: root.darkMode ? "#c3c7ce" : "#777c83"; Layout.preferredWidth: 12; Layout.preferredHeight: 12 }
                }
                background: Rectangle { radius: height / 2; color: root.darkMode ? "#303238" : "#eff0f2"; border.width: sortButton.activeFocus ? 1 : 0; border.color: "#426da7" }
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
                onClicked: displaySettings.showExcerpts = checked
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
                height: modelData.directory || !displaySettings.showExcerpts ? 30 : 54
                leftPadding: 8 + modelData.depth * 16
                highlighted: !modelData.directory && modelData.url.toString() === root.currentFile.toString()
                Accessible.name: modelData.name
                Accessible.description: modelData.directory ? "Folder" : "Markdown or text document"
                onClicked: {
                    if (modelData.directory) root.library.toggleFolder(modelData.url)
                    else root.openRequested(modelData.url)
                }
                background: Rectangle {
                    radius: 0
                    color: entry.highlighted ? (root.darkMode ? "#293c57" : "#f1f4f9")
                        : entry.hovered ? (root.darkMode ? "#2b2d31" : "#e9ecf0") : "transparent"
                }
                Rectangle { anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom; width: 2; visible: entry.highlighted; color: root.darkMode ? "#8eb5f0" : "#244f88" }
                Rectangle { anchors.bottom: parent.bottom; x: 24; width: parent.width - 24; height: 1; color: root.darkMode ? "#303237" : "#ededee" }
                contentItem: RowLayout {
                    spacing: 7
                    LineIcon {
                        name: entry.modelData.directory ? "folder" : "editor"
                        ink: entry.highlighted ? (root.darkMode ? "#8eb5f0" : "#244f88") : (root.darkMode ? "#9ba2ae" : "#737b87")
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                    }
                    ColumnLayout {
                        spacing: 3
                        Layout.fillWidth: true
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: entry.modelData.name; font.pixelSize: 12; color: root.darkMode ? "#e3e5e9" : "#30343b"; elide: Text.ElideMiddle; Layout.fillWidth: true }
                            Label { visible: !entry.modelData.directory && displaySettings.showDates; text: entry.modelData.modified; font.pixelSize: 10; color: "#92969e" }
                        }
                        Label { visible: !entry.modelData.directory && displaySettings.showExcerpts; text: visible ? root.library.excerpt(entry.modelData.url) : ""; elide: Text.ElideRight; font.pixelSize: 11; color: "#92969e"; Layout.fillWidth: true }
                    }
                    LineIcon { visible: entry.modelData.directory; name: entry.modelData.expanded ? "down" : "right"; ink: "#92969e"; Layout.preferredWidth: 14; Layout.preferredHeight: 14 }
                }
                ToolTip.delay: 700
                ToolTip.visible: hovered
                ToolTip.text: modelData.url.toString()
            }
            Label {
                anchors.centerIn: parent
                width: parent.width - 20
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                visible: fileList.count === 0
                text: root.library.rootFolder.toString() === "" ? "Choose a folder to begin.\nYour files stay on your Mac." : "No Markdown or text files here.\nExpand a folder or create a document."
                color: root.darkMode ? "#929aa6" : "#78808c"
                font.pixelSize: 13
            }
        }
        TextField {
            id: filterField
            objectName: "libraryFilter"
            Layout.fillWidth: true
            implicitHeight: 26
            placeholderText: "Filter files"
            leftPadding: 8
            font.pixelSize: 12
            text: root.library.filter
            onTextEdited: root.library.filter = text
            Accessible.name: "Filter visible library files"
            background: Rectangle { radius: 13; color: root.darkMode ? "#292c31" : "#f0f1f3"; border.width: filterField.activeFocus ? 1 : 0; border.color: "#426da7" }
            Keys.onEscapePressed: { text = ""; root.library.filter = ""; }
        }
    }

    CompactMenu {
        id: sortMenu
        objectName: "librarySortMenu"
        ButtonGroup { id: sortFieldGroup }
        ButtonGroup { id: sortDirectionGroup }
        parent: sortButton
        x: 0
        y: sortButton.height + 3
        darkMode: root.darkMode
        CompactMenuItem { text: "Date Modified"; ButtonGroup.group: sortFieldGroup; checkable: true; checked: root.library.sortMode === 1; onTriggered: root.library.sortMode = 1 }
        CompactMenuItem { text: "Date Created"; ButtonGroup.group: sortFieldGroup; checkable: true; checked: root.library.sortMode === 2; onTriggered: root.library.sortMode = 2 }
        CompactMenuItem { text: "Name"; ButtonGroup.group: sortFieldGroup; checkable: true; checked: root.library.sortMode === 0; onTriggered: root.library.sortMode = 0 }
        CompactMenuItem { text: "Extension"; ButtonGroup.group: sortFieldGroup; checkable: true; checked: root.library.sortMode === 3; onTriggered: root.library.sortMode = 3 }
        MenuSeparator { padding: 4; implicitHeight: 9; contentItem: Rectangle { implicitHeight: 1; color: root.darkMode ? "#45484e" : "#dedfe2" } }
        CompactMenuItem { text: "A to Z"; ButtonGroup.group: sortDirectionGroup; checkable: true; checked: root.library.ascending; onTriggered: root.library.ascending = true }
        CompactMenuItem { text: "Z to A"; ButtonGroup.group: sortDirectionGroup; checkable: true; checked: !root.library.ascending; onTriggered: root.library.ascending = false }
        MenuSeparator { padding: 4; implicitHeight: 9; contentItem: Rectangle { implicitHeight: 1; color: root.darkMode ? "#45484e" : "#dedfe2" } }
        CompactMenuItem { text: "Pin Folders to Top"; checkable: true; checked: root.library.foldersFirst; onTriggered: root.library.foldersFirst = !root.library.foldersFirst }
        MenuSeparator { padding: 4; implicitHeight: 9; contentItem: Rectangle { implicitHeight: 1; color: root.darkMode ? "#45484e" : "#dedfe2" } }
        CompactMenuItem { text: "Show Date"; checkable: true; checked: displaySettings.showDates; onTriggered: displaySettings.showDates = !displaySettings.showDates }
        CompactMenuItem { text: "Show Text Excerpts"; checkable: true; checked: displaySettings.showExcerpts; onTriggered: displaySettings.showExcerpts = !displaySettings.showExcerpts }
    }
    CompactMenu {
        id: folderMenu
        darkMode: root.darkMode
        CompactMenuItem { text: "Choose Folder…"; onTriggered: folderDialog.open() }
        CompactMenuItem { text: "New Folder…"; enabled: root.library.rootFolder.toString() !== ""; onTriggered: newFolderDialog.open() }
        CompactMenuItem { text: "Refresh"; onTriggered: root.library.refresh() }
    }
    Dialogs.FolderDialog {
        id: folderDialog
        title: "Choose your writing folder"
        onAccepted: root.library.rootFolder = selectedFolder
    }
    Dialog {
        id: newFileDialog
        title: "New document"
        anchors.centerIn: parent
        modal: true
        width: 290
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: { newFileName.text = "Untitled.md"; newFileName.forceActiveFocus(); newFileName.selectAll(); }
        onAccepted: {
            var file = root.library.createDocument(newFileName.text)
            if (file.toString() !== "") root.openRequested(file)
        }
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
