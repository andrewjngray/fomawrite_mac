import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

Rectangle {
    id: root
    objectName: "organizerPane"
    required property var library
    property url currentFile
    property bool darkMode: false
    signal openRequested(url file)
    color: darkMode ? "#1c1e22" : "#f5f5f7"
    function activate(entry) {
        if (entry.directory) library.rootFolder = entry.url;
        else openRequested(entry.url);
    }
    ScrollView {
        anchors.fill: parent
        id: organizerScroll
        anchors.margins: 12
        contentWidth: availableWidth
        ColumnLayout {
            width: organizerScroll.availableWidth
            spacing: 4
            RowLayout {
                Label { text: "Locations"; font.pixelSize: 11; font.bold: false; color: root.darkMode ? "#92969e" : "#777c83"; Layout.fillWidth: true }
                ChromeButton { darkMode: root.darkMode; iconName: "plus"; Accessible.name: "Add library location"; onClicked: locationDialog.open() }
            }
            Repeater {
                model: root.library.locations
                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    ChromeButton {
                        text: modelData.name
                        iconName: modelData.directory ? "folder" : "editor"
                        alignLeft: true
                        darkMode: root.darkMode
                        enabled: modelData.available
                        checked: modelData.url.toString() === root.library.rootFolder.toString()
                        Layout.fillWidth: true
                        onClicked: root.library.rootFolder = modelData.url
                        ToolTip.visible: hovered
                        ToolTip.text: modelData.url.toString()
                    }
                    ChromeButton { darkMode: root.darkMode; iconName: "close"; Accessible.name: "Remove location shortcut " + modelData.name; onClicked: root.library.removeLocation(modelData.url) }
                }
            }
            Label { text: "Favorites"; font.pixelSize: 11; font.bold: false; color: root.darkMode ? "#92969e" : "#777c83"; Layout.topMargin: 14 }
            Repeater {
                model: root.library.favorites
                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    ChromeButton {
                        text: modelData.name
                        iconName: modelData.directory ? "folder" : "editor"
                        alignLeft: true
                        darkMode: root.darkMode
                        Layout.fillWidth: true
                        enabled: modelData.available
                        onClicked: root.activate(modelData)
                        ToolTip.visible: hovered
                        ToolTip.text: modelData.url.toString()
                    }
                    ChromeButton { darkMode: root.darkMode; iconName: "close"; Accessible.name: "Remove favorite " + modelData.name; onClicked: root.library.toggleFavorite(modelData.url) }
                }
            }
            ChromeButton { text: "+ Favorite folder"; darkMode: root.darkMode; Layout.fillWidth: true; onClicked: root.library.toggleFavorite(root.library.rootFolder) }
            ChromeButton { text: "+ Favorite document"; darkMode: root.darkMode; Layout.fillWidth: true; enabled: root.currentFile.toString() !== ""; onClicked: root.library.toggleFavorite(root.currentFile) }
            RowLayout {
                Layout.topMargin: 14
                Label { text: "Recents"; font.pixelSize: 11; font.bold: false; color: root.darkMode ? "#92969e" : "#777c83"; Layout.fillWidth: true }
                ChromeButton { darkMode: root.darkMode; iconName: "close"; Accessible.name: "Clear recent file shortcuts"; onClicked: root.library.clearRecentFiles() }
            }
            Repeater {
                model: root.library.recentFiles
                delegate: ChromeButton {
                    darkMode: root.darkMode
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: 30
                    font.pixelSize: 12
                    font.bold: false
                    text: modelData.name
                        iconName: modelData.directory ? "folder" : "editor"
                        alignLeft: true
                    enabled: modelData.available
                    onClicked: root.openRequested(modelData.url)
                    ToolTip.visible: hovered
                    ToolTip.text: modelData.url.toString()
                }
            }
        }
    }
    Dialogs.FolderDialog {
        id: locationDialog
        title: "Add writing location"
        onAccepted: root.library.rootFolder = selectedFolder
    }
}
