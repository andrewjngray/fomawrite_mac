import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

Rectangle {
    id: root
    required property var library
    property url currentFile
    property bool darkMode: false
    signal openRequested(url file)
    color: darkMode ? "#202124" : "#fbfbfc"
    objectName: "libraryPane"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4
        RowLayout {
            ChromeButton { text: root.library.rootName || "Choose folder…"; hint: "Choose library folder"; darkMode: root.darkMode; Layout.fillWidth: true; onClicked: folderDialog.open() }
            ChromeButton { iconName: "plus"; hint: "New document"; darkMode: root.darkMode; enabled: root.library.rootFolder.toString() !== ""; onClicked: newFileDialog.open() }
            ChromeButton { iconName: "more"; hint: "Library options and sorting"; darkMode: root.darkMode; onClicked: folderMenu.open() }
        }
        ChromeButton {
            objectName: "librarySort"
            text: "Sort by " + ["name", "modified", "created", "extension"][root.library.sortMode] + (root.library.ascending ? " ↑" : " ↓")
            darkMode: root.darkMode
            onClicked: folderMenu.open()
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
                height: 32
                leftPadding: 8 + modelData.depth * 16
                highlighted: !modelData.directory && modelData.url.toString() === root.currentFile.toString()
                Accessible.name: modelData.name
                Accessible.description: modelData.directory ? "Folder" : "Markdown or text document"
                onClicked: {
                    if (modelData.directory) root.library.toggleFolder(modelData.url)
                    else root.openRequested(modelData.url)
                }
                background: Rectangle {
                    radius: 4
                    color: entry.highlighted ? (root.darkMode ? "#293c57" : "#e5ecf7")
                        : entry.hovered ? (root.darkMode ? "#2b2d31" : "#e9ecf0") : "transparent"
                }
                contentItem: RowLayout {
                    spacing: 7
                    LineIcon {
                        name: entry.modelData.directory ? (entry.modelData.expanded ? "down" : "right") : "editor"
                        ink: entry.highlighted ? (root.darkMode ? "#8eb5f0" : "#244f88") : (root.darkMode ? "#9ba2ae" : "#737b87")
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                    }
                    Label { text: entry.modelData.name; font.pixelSize: 12; color: root.darkMode ? "#e3e5e9" : "#30343b"; elide: Text.ElideMiddle; Layout.fillWidth: true }
                }
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
            implicitHeight: 30
            placeholderText: "Filter files"
            leftPadding: 8
            font.pixelSize: 12
            text: root.library.filter
            onTextEdited: root.library.filter = text
            Accessible.name: "Filter visible library files"
            background: Rectangle { radius: 5; color: root.darkMode ? "#292c31" : "#f0f1f3"; border.width: filterField.activeFocus ? 1 : 0; border.color: "#426da7" }
            Keys.onEscapePressed: { text = ""; root.library.filter = ""; }
        }
    }

    Menu {
        id: folderMenu
        MenuItem { text: "Choose Folder…"; onTriggered: folderDialog.open() }
        MenuItem { text: "New Folder…"; enabled: root.library.rootFolder.toString() !== ""; onTriggered: newFolderDialog.open() }
        MenuItem { text: "Sort by name"; onTriggered: root.library.sortMode = 0 }
        MenuItem { text: "Sort by modified date"; onTriggered: root.library.sortMode = 1 }
        MenuItem { text: "Sort by created date"; onTriggered: root.library.sortMode = 2 }
        MenuItem { text: "Sort by extension"; onTriggered: root.library.sortMode = 3 }
        MenuItem { text: "Ascending order"; checkable: true; checked: root.library.ascending; onTriggered: root.library.ascending = !root.library.ascending }
        MenuItem { text: "Pin folders to top"; checkable: true; checked: root.library.foldersFirst; onTriggered: root.library.foldersFirst = !root.library.foldersFirst }
        MenuItem { text: "Refresh"; onTriggered: root.library.refresh() }
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
