import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

Rectangle {
    id: root
    objectName: "organizerPane"
    required property var library
    property url currentFile
    property bool darkMode: false
    Settings {
        id: sectionSettings
        category: "organizerSections"
        property bool favoritesExpanded: true
        property bool recentsExpanded: true
    }
    signal searchRequested(string query, bool contents, url folder)
    signal openRequested(url file)
    color: backend.palette.panel
    function activate(entry) {
        if (entry.directory) library.rootFolder = entry.url;
        else openRequested(entry.url);
    }
    LibraryContextMenu { id: contextMenu; library: root.library; darkMode: root.darkMode }
    ScrollView {
        anchors.fill: parent
        id: organizerScroll
        anchors.margins: 12
        contentWidth: availableWidth
        ColumnLayout {
            width: organizerScroll.availableWidth
            spacing: 2
            RowLayout {
                Label { text: "Locations"; font.pixelSize: 12; font.bold: false; color: backend.palette.muted; Layout.fillWidth: true }
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
                        enabled: true
                        checked: modelData.url.toString() === root.library.rootFolder.toString()
                        Layout.fillWidth: true
                        onClicked: if (modelData.available) root.library.rootFolder = modelData.url
                        hint: modelData.url.toString()
                        Accessible.name: text
                        tooltipDelay: 2000
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: (eventPoint) => contextMenu.showFor(parent, modelData, true, eventPoint.position)
                        }
                        Keys.onMenuPressed: contextMenu.showFor(this, modelData, true, Qt.point(0, height))
                    }
                    ChromeButton { darkMode: root.darkMode; iconName: "close"; Accessible.name: "Remove location shortcut " + modelData.name; onClicked: root.library.removeLocation(modelData.url) }
                }
            }
            ChromeButton {
                objectName: "favoritesDisclosure"
                Layout.topMargin: 14
                Layout.fillWidth: true
                text: "Favorites"
                hint: (sectionSettings.favoritesExpanded ? "Collapse" : "Expand") + " Favorites"
                iconName: sectionSettings.favoritesExpanded ? "down" : "right"
                alignLeft: true
                darkMode: root.darkMode
                font.pixelSize: 12
                onClicked: sectionSettings.favoritesExpanded = !sectionSettings.favoritesExpanded
            }
            ColumnLayout {
                objectName: "favoritesContents"
                visible: sectionSettings.favoritesExpanded
                Layout.fillWidth: true
                spacing: 2
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
                            enabled: true
                            onClicked: if (modelData.available) root.activate(modelData)
                            hint: modelData.url.toString()
                            Accessible.name: text
                            tooltipDelay: 2000
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: (eventPoint) => contextMenu.showFor(parent, modelData, false, eventPoint.position)
                        }
                        Keys.onMenuPressed: contextMenu.showFor(this, modelData, false, Qt.point(0, height))
                        }
                        ChromeButton { darkMode: root.darkMode; iconName: "close"; Accessible.name: "Remove favorite " + modelData.name; onClicked: root.library.toggleFavorite(modelData.url) }
                    }
                }
                ChromeButton { text: "Favorite folder"; iconName: "plus"; alignLeft: true; darkMode: root.darkMode; Layout.fillWidth: true; onClicked: root.library.toggleFavorite(root.library.rootFolder) }
                ChromeButton { text: "Favorite document"; iconName: "plus"; alignLeft: true; darkMode: root.darkMode; Layout.fillWidth: true; enabled: root.currentFile.toString() !== ""; onClicked: root.library.toggleFavorite(root.currentFile) }
            }
            Label { text: "Smart folders"; color: backend.palette.muted; Layout.topMargin: 14 }
            Repeater {
                model: root.library.savedSearches
                delegate: ChromeButton {
                    required property var modelData
                    Layout.fillWidth: true; alignLeft: true; iconName: "search"; darkMode: root.darkMode
                    text: modelData.query; hint: "Search saved files: " + modelData.query
                    onClicked: root.searchRequested(modelData.query, modelData.contents, modelData.root)
                }
            }
            Label { text: "Save queries in Quick Open to add smart folders."; visible: root.library.savedSearches.length === 0; wrapMode: Text.Wrap; Layout.fillWidth: true; color: backend.palette.muted; font.pixelSize: 11 }
            RowLayout {
                Label { text: "Tags"; color: backend.palette.muted; Layout.fillWidth: true }
                ChromeButton { text: "Refresh"; hint: "Scan saved files for tags"; darkMode: root.darkMode; onClicked: root.library.refreshTags() }
            }
            Label { text: root.library.tagStatus; wrapMode: Text.Wrap; Layout.fillWidth: true; color: backend.palette.muted; font.pixelSize: 10 }
            Repeater {
                model: root.library.tagIndex
                delegate: ChromeButton {
                    required property var modelData
                    Layout.fillWidth: true; alignLeft: true; darkMode: root.darkMode
                    text: "#" + modelData.tag + " (" + modelData.count + ")"
                    onClicked: root.searchRequested("#" + modelData.tag,true,root.library.rootFolder)
                }
            }
            RowLayout {
                Layout.topMargin: 14
                ChromeButton {
                    objectName: "recentsDisclosure"
                    Layout.fillWidth: true
                    text: "Recents"
                    hint: (sectionSettings.recentsExpanded ? "Collapse" : "Expand") + " Recents"
                    iconName: sectionSettings.recentsExpanded ? "down" : "right"
                    alignLeft: true
                    darkMode: root.darkMode
                    font.pixelSize: 12
                    onClicked: sectionSettings.recentsExpanded = !sectionSettings.recentsExpanded
                }
                ChromeButton { darkMode: root.darkMode; iconName: "close"; Accessible.name: "Clear recent file shortcuts"; onClicked: root.library.clearRecentFiles() }
            }
            ColumnLayout {
                objectName: "recentsContents"
                visible: sectionSettings.recentsExpanded
                Layout.fillWidth: true
                spacing: 2
                Repeater {
                    model: root.library.recentFiles
                    delegate: ChromeButton {
                        darkMode: root.darkMode
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: 30
                        font.pixelSize: 14
                        font.bold: false
                        text: modelData.name
                        iconName: modelData.directory ? "folder" : "editor"
                        alignLeft: true
                        enabled: true
                        onClicked: if (modelData.available) root.openRequested(modelData.url)
                        hint: modelData.url.toString()
                        Accessible.name: text
                        tooltipDelay: 2000
                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: (eventPoint) => contextMenu.showFor(parent, modelData, false, eventPoint.position)
                        }
                        Keys.onMenuPressed: contextMenu.showFor(this, modelData, false, Qt.point(0, height))
                    }
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
