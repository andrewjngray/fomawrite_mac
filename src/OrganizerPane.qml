import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

Rectangle {
    id: root
    objectName: "organizerPane"
    required property var commands
    required property var library
    property url currentFile
    property bool darkMode: false
    Settings {
        id: sectionSettings
        category: "organizerSections"
        property bool favoritesExpanded: true
        property bool recentsExpanded: true
        property bool tagsExpanded: false
    }
    signal searchRequested(string query, bool contents, url folder)
    signal openRequested(url file)
    color: backend.palette.organizer
    function chooseLocation() { locationDialog.open() }
    function activate(entry) {
        if (entry.directory) library.rootFolder = entry.url;
        else openRequested(entry.url);
    }
    LibraryContextMenu { id: contextMenu; library: root.library; commands: root.commands; darkMode: root.darkMode }
    component OrganizerSection: ChromeButton {
        id: sectionButton
        implicitHeight: 20
        font.pixelSize: 12
        font.weight: Font.Medium
        alignLeft: true
        darkMode: root.darkMode
        contentItem: Item {
            Text {
                anchors.fill: parent
                anchors.rightMargin: 16
                text: sectionButton.text
                font: sectionButton.font
                color: backend.palette.muted
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
            LineIcon {
                width: 12; height: 12
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                name: sectionButton.iconName
                ink: backend.palette.muted
                opacity: sectionButton.hovered || sectionButton.activeFocus ? 1 : 0
            }
        }
    }
    component OrganizerEntry: Item {
        id: shortcutRow
        required property var entryData
        property string shortcutKind: ""
        Layout.fillWidth: true
        implicitHeight: 30
        HoverHandler { id: rowHover }
        ChromeButton {
            id: shortcutButton
            anchors.fill: parent
            rightPadding: 30
            implicitHeight: 30
            text: shortcutRow.entryData.name
            iconName: shortcutRow.entryData.directory ? "folder" : "editor"
            iconColor: shortcutRow.entryData.available
                ? (shortcutRow.entryData.directory ? backend.palette.folder : backend.palette.muted)
                : backend.palette.muted
            alignLeft: true
            darkMode: root.darkMode
            checked: shortcutRow.entryData.available && (shortcutRow.entryData.directory
                ? shortcutRow.entryData.url.toString() === root.library.rootFolder.toString()
                : shortcutRow.entryData.url.toString() === root.currentFile.toString())
            opacity: shortcutRow.entryData.available ? 1 : 0.55
            Accessible.checkable: true
            Accessible.checked: checked
            hint: shortcutRow.entryData.url.toString() + (shortcutRow.entryData.available ? "" : " — unavailable")
            Accessible.name: text
            Accessible.description: shortcutRow.entryData.available
                ? (shortcutRow.entryData.directory ? "Folder shortcut" : "Document shortcut") : "Unavailable location"
            tooltipDelay: 2000
            onClicked: {
                if (!shortcutRow.entryData.available) return
                if (shortcutRow.shortcutKind === "location") root.library.rootFolder = shortcutRow.entryData.url
                else root.activate(shortcutRow.entryData)
            }
            Keys.onMenuPressed: contextMenu.showFor(this, shortcutRow.entryData, shortcutRow.shortcutKind === "location", Qt.point(0, height))
        }
        ChromeButton {
            visible: shortcutRow.shortcutKind === "location" || shortcutRow.shortcutKind === "favorite"
            width: 24
            height: 24
            anchors.right: parent.right
            anchors.rightMargin: 3
            anchors.verticalCenter: parent.verticalCenter
            iconName: "close"
            darkMode: root.darkMode
            opacity: rowHover.hovered || shortcutButton.activeFocus || activeFocus ? 1 : 0
            hint: (shortcutRow.shortcutKind === "location" ? "Remove location shortcut " : "Remove favorite ") + shortcutRow.entryData.name
            Accessible.name: hint
            onClicked: {
                if (shortcutRow.shortcutKind === "location") root.library.removeLocation(shortcutRow.entryData.url)
                else root.library.toggleFavorite(shortcutRow.entryData.url)
            }
        }
        TapHandler {
            acceptedButtons: Qt.RightButton
            onTapped: (eventPoint) => contextMenu.showFor(shortcutButton, shortcutRow.entryData, shortcutRow.shortcutKind === "location", eventPoint.position)
        }
    }
    ScrollView {
        id: organizerScroll
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        anchors.topMargin: 20
        anchors.bottomMargin: organizerFooter.height + 8
        contentWidth: availableWidth
        clip: true
        ColumnLayout {
            width: organizerScroll.availableWidth
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: 8
                Label {
                    text: "Library"
                    font.family: Qt.application.font.family
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    color: backend.palette.muted
                    Layout.leftMargin: 8
                    Layout.fillWidth: true
                }
                ChromeButton { darkMode: root.darkMode; implicitHeight: 20; implicitWidth: 24; iconName: "plus"; hint: "Add library location"; onClicked: locationDialog.open() }
            }
            Repeater {
                model: root.library.locations
                delegate: OrganizerEntry {
                    required property var modelData
                    entryData: modelData
                    shortcutKind: "location"
                }
            }
            RowLayout {
                Layout.topMargin: 20
                Layout.bottomMargin: 8
                Layout.fillWidth: true
                OrganizerSection {
                    objectName: "favoritesDisclosure"
                    Layout.fillWidth: true
                    text: "Favorites"
                    hint: (sectionSettings.favoritesExpanded ? "Collapse" : "Expand") + " Favorites"
                    iconName: sectionSettings.favoritesExpanded ? "down" : "right"
                    onClicked: sectionSettings.favoritesExpanded = !sectionSettings.favoritesExpanded
                }
                ChromeButton {
                    id: favoriteActions
                    implicitHeight: 20
                    implicitWidth: 24
                    iconName: "plus"
                    hint: "Add a favorite folder or document"
                    darkMode: root.darkMode
                    onClicked: favoriteMenu.open()
                }
            }
            ColumnLayout {
                objectName: "favoritesContents"
                visible: sectionSettings.favoritesExpanded
                Layout.fillWidth: true
                spacing: 2
                Repeater {
                    model: root.library.favorites
                    delegate: OrganizerEntry {
                        required property var modelData
                        entryData: modelData
                        shortcutKind: "favorite"
                    }
                }
            }
            Label {
                text: "Smart folders"
                color: backend.palette.muted
                font.family: Qt.application.font.family
                font.pixelSize: 12
                font.weight: Font.Medium
                Layout.leftMargin: 8
                Layout.topMargin: 20
                Layout.bottomMargin: 8
            }
            Repeater {
                model: root.library.savedSearches
                delegate: ChromeButton {
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: 30
                    alignLeft: true
                    iconName: "search"
                    darkMode: root.darkMode
                    text: modelData.query
                    hint: "Search saved files: " + modelData.query
                    onClicked: root.searchRequested(modelData.query, modelData.contents, modelData.root)
                }
            }
            Label {
                text: "Save queries in Quick Open to add smart folders."
                visible: root.library.savedSearches.length === 0
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                color: backend.palette.muted
                font.family: Qt.application.font.family
                font.pixelSize: 12
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.bottomMargin: 8
                OrganizerSection {
                    objectName: "tagsDisclosure"
                    text: "Tags"
                    Layout.fillWidth: true
                    hint: (sectionSettings.tagsExpanded ? "Collapse" : "Expand") + " Tags"
                    iconName: sectionSettings.tagsExpanded ? "down" : "right"
                    onClicked: sectionSettings.tagsExpanded = !sectionSettings.tagsExpanded
                }
                ChromeButton {
                    implicitHeight: 20
                    implicitWidth: 24
                    iconName: "refresh"
                    hint: "Scan saved files for tags"
                    darkMode: root.darkMode
                    onClicked: { sectionSettings.tagsExpanded = true; root.library.refreshTags() }
                }
            }
            ColumnLayout {
                visible: sectionSettings.tagsExpanded
                Layout.fillWidth: true
                spacing: 0
                Label {
                    text: root.library.tagStatus
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    Layout.rightMargin: 8
                    color: backend.palette.muted
                    font.family: Qt.application.font.family
                    font.pixelSize: 12
                }
                Repeater {
                    model: root.library.tagIndex
                    delegate: ChromeButton {
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: 30
                        alignLeft: true
                        iconName: "tag"
                        darkMode: root.darkMode
                        text: "#" + modelData.tag + " (" + modelData.count + ")"
                        hint: "Search saved files for #" + modelData.tag
                        onClicked: root.searchRequested("#" + modelData.tag, true, root.library.rootFolder)
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 20
                Layout.bottomMargin: 8
                OrganizerSection {
                    objectName: "recentsDisclosure"
                    Layout.fillWidth: true
                    text: "Recents"
                    hint: (sectionSettings.recentsExpanded ? "Collapse" : "Expand") + " Recents"
                    iconName: sectionSettings.recentsExpanded ? "down" : "right"
                    iconColor: backend.palette.muted
                    alignLeft: true
                    darkMode: root.darkMode
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    onClicked: sectionSettings.recentsExpanded = !sectionSettings.recentsExpanded
                }
                ChromeButton { id: recentActions; implicitHeight: 20; implicitWidth: 24; darkMode: root.darkMode; iconName: "more"; hint: "Recent file options"; onClicked: recentMenu.open() }
            }
            ColumnLayout {
                objectName: "recentsContents"
                visible: sectionSettings.recentsExpanded
                Layout.fillWidth: true
                spacing: 2
                Repeater {
                    model: root.library.recentFiles
                    delegate: OrganizerEntry {
                        required property var modelData
                        entryData: modelData
                    }
                }
            }
        }
    }
    CompactMenu {
        id: favoriteMenu
        parent: favoriteActions
        y: favoriteActions.height + 3
        darkMode: root.darkMode
        CompactMenuItem {
            text: "Favorite current folder"
            enabled: root.library.rootFolder.toString() !== ""
            onTriggered: root.library.toggleFavorite(root.library.rootFolder)
        }
        CompactMenuItem {
            text: "Favorite current document"
            enabled: root.currentFile.toString() !== ""
            onTriggered: root.library.toggleFavorite(root.currentFile)
        }
    }
    CompactMenu {
        id: recentMenu
        parent: recentActions
        y: recentActions.height + 3
        darkMode: root.darkMode
        CompactMenuItem { text: "Clear recent file shortcuts"; onTriggered: root.library.clearRecentFiles() }
    }
    Dialogs.FolderDialog {
        id: locationDialog
        title: "Add writing location"
        onAccepted: root.library.addLocation(selectedFolder)
    }
    Connections {
        target: root.library
        function onLocationRejected(url, message, canFavorite) {
            overlapDialog.canFavorite = canFavorite;
            overlapDialog.folder = url;
            overlapMessage.text = message;
            overlapDialog.open();
        }
    }
    Dialog {
        id: overlapDialog
        property url folder
        property bool canFavorite: false
        title: "Cannot add Location"
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(440, parent.width - 32)
        modal: true
        Label { id: overlapMessage; width: parent.width; wrapMode: Text.Wrap; textFormat: Text.PlainText }
        footer: DialogButtonBox {
            Button { text: "Cancel"; DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            Button {
                text: "Add to Favorites instead"
                visible: overlapDialog.canFavorite
                onClicked: {
                    if (root.library.addFavorite(overlapDialog.folder)) overlapDialog.close();
                }
            }
            onRejected: overlapDialog.close()
        }
    }

    WorkspaceFooter {
        id: organizerFooter
        objectName: "organizerFooter"
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        color: backend.palette.organizer
        Label {
            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
            text: backend.status
            font.family: Qt.application.font.family; font.pixelSize: 13
            color: backend.palette.muted
            elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
            Accessible.name: "Document status: " + backend.status
            ToolTip.visible: statusHover.hovered; ToolTip.text: backend.status
            HoverHandler { id: statusHover }
        }
    }

}
