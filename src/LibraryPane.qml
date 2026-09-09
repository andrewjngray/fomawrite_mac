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
    color: darkMode ? "#202124" : "#f4f5f7"
    objectName: "libraryPane"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8
        RowLayout {
            Label { text: "LIBRARY"; font.pixelSize: 11; font.bold: true; color: root.darkMode ? "#9ca3af" : "#737b87"; Layout.fillWidth: true }
            ToolButton { text: "+"; font.pixelSize: 22; enabled: root.library.rootFolder.toString() !== ""; Accessible.name: "New document"; onClicked: newFileDialog.open(); ToolTip.visible: hovered; ToolTip.text: "New Markdown document" }
            ToolButton { text: "…"; font.pixelSize: 22; Accessible.name: "Library actions"; onClicked: folderMenu.open() }
        }
        Button {
            Layout.fillWidth: true
            text: root.library.rootName || "Choose Folder…"
            Accessible.name: "Choose library folder"
            onClicked: folderDialog.open()
            ToolTip.visible: hovered
            ToolTip.text: root.library.rootFolder.toString()
        }
        TextField {
            id: filterField
            objectName: "libraryFilter"
            Layout.fillWidth: true
            placeholderText: "Filter visible files"
            font.pixelSize: 13
            text: root.library.filter
            onTextEdited: root.library.filter = text
            Accessible.name: "Filter library files"
            Keys.onEscapePressed: { text = ""; root.library.filter = ""; }
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
                height: 36
                leftPadding: 8 + modelData.depth * 16
                highlighted: !modelData.directory && modelData.url.toString() === root.currentFile.toString()
                Accessible.name: modelData.name
                Accessible.description: modelData.directory ? "Folder" : "Markdown or text document"
                onClicked: {
                    if (modelData.directory) root.library.toggleFolder(modelData.url)
                    else root.openRequested(modelData.url)
                }
                background: Rectangle {
                    radius: 5
                    color: entry.highlighted ? (root.darkMode ? "#244354" : "#dceef8")
                        : entry.hovered ? (root.darkMode ? "#2b2d31" : "#e9ecf0") : "transparent"
                }
                contentItem: RowLayout {
                    spacing: 7
                    Label {
                        text: entry.modelData.directory ? (entry.modelData.expanded ? "▾" : "▸") : "≡"
                        color: entry.modelData.directory ? "#2896c4" : (root.darkMode ? "#9ba2ae" : "#8a93a0")
                        font.pixelSize: 16
                        Layout.preferredWidth: 14
                    }
                    Label { text: entry.modelData.name; font.pixelSize: 13; color: root.darkMode ? "#e3e5e9" : "#30343b"; elide: Text.ElideMiddle; Layout.fillWidth: true }
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
        Label { text: "Markdown & text · A–Z"; font.pixelSize: 11; color: root.darkMode ? "#929aa6" : "#78808c" }
    }

    Menu {
        id: folderMenu
        MenuItem { text: "Choose Folder…"; onTriggered: folderDialog.open() }
        MenuItem { text: "New Folder…"; enabled: root.library.rootFolder.toString() !== ""; onTriggered: newFolderDialog.open() }
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
