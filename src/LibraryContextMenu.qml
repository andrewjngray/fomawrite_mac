import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

CompactMenu {
    id: menu
    delegate: CompactMenuItem {
        visible: !subMenu || subMenu.contextAllowed
        height: visible ? implicitHeight : 0
    }
    required property var library
    required property var commands
    property var target: ({})
    property bool location: false
    property string pending: ""
    readonly property bool folder: !!target.directory
    height: Math.min(implicitHeight, (Overlay.overlay ? Overlay.overlay.height : 900) - 24)
    function showFor(item, entry, isLocation, point) {
        target = backend.libraryItemInfo(entry.url);
        if (target.error) target = entry;
        location = isLocation;
        popup(item, point.x, point.y);
    }
    function later(action) { pending = action; close(); Qt.callLater(performPending); }
    function run(action, argument) {
        if (!backend.libraryItemAction(target.url, action, argument || "")) {
            message.text = backend.status;
            messageDialog.open();
            return false;
        }
        return true;
    }
    onClosed: Qt.callLater(performPending)
    function performPending() {
        const action = pending; pending = "";
        if (!action) return;
        if (["rename", "duplicate", "newFile", "newFolder"].indexOf(action) >= 0) {
            nameDialog.action = action;
            nameDialog.title = ({rename:"Rename",duplicate:"Duplicate",newFile:"New File",newFolder:"New Folder"})[action];
            nameField.text = action === "newFile" ? "Untitled.md" : action === "newFolder" ? "New Folder" : target.name;
            if (action === "duplicate") {
                const dot = folder ? -1 : target.name.lastIndexOf(".");
                nameField.text = dot > 0 ? target.name.slice(0,dot) + " copy" + target.name.slice(dot) : target.name + " copy";
            }
            nameError.text = ""; nameDialog.open(); nameField.forceActiveFocus(); nameField.selectAll();
        } else if (action === "info") {
            messageDialog.title = "Get Info";
            message.text = target.name + "\n\n" + target.path + "\n\nKind: " + (folder ? "Folder" : "Document")
                + (folder ? "" : "\nSize: " + target.bytes + " bytes") + "\nCreated: " + target.created + "\nModified: " + target.modified
                + "\nWritable: " + (target.writable ? "Yes" : "No");
            messageDialog.open();
        } else if (action === "trash") trashDialog.open();
        else if (action === "export") exportChoice.open();
        else if (action === "tab") backend.openInNewTab(target.url);
        else if (action === "window") backend.openInNewWindow(target.url);
        else if (action === "open") library.rootFolder = target.url;
        else run(action);
    }
    CompactMenuItem { text: "Rename Folder…"; visible: menu.location; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("rename") }
    CompactMenuItem { text: "Remove from Locations"; visible: menu.location; height: visible ? implicitHeight : 0; onTriggered: menu.library.removeLocation(menu.target.url) }
    CompactMenuItem { text: "Open"; visible: !menu.location && menu.folder; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("open") }
    CompactMenuItem { text: "Open in New Tab"; visible: !menu.location && !menu.folder; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("tab") }
    CompactMenuItem { text: "Open in New Window"; visible: !menu.location && !menu.folder; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("window") }
    MenuSeparator {}
    CompactMenuItem { text: "Get Info"; visible: !menu.location; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("info") }
    CompactMenuItem { text: menu.target.favorite ? "Remove Favorite" : "Favorite"; visible: !menu.location; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("favorite") }
    CompactMenuItem { text: "Duplicate…"; visible: !menu.location; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("duplicate") }
    CompactMenuItem { text: "Rename…"; visible: !menu.location; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("rename") }
    CompactMenuItem { text: "Move to Trash…"; visible: !menu.location; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("trash") }
    MenuSeparator { visible: !menu.location; height: visible ? implicitHeight : 0 }
    CompactMenuItem { text: Qt.platform.os === "osx" ? "Show in Finder" : "Show in File Manager"; enabled: !!menu.target.available; onTriggered: menu.library.showInFileManager(menu.target.url) }
    MenuSeparator {}
    CompactMenuItem { text: "Share…"; visible: !menu.location && Qt.platform.os === "osx"; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("share") }
    CompactMenuItem { text: "Export…"; visible: !menu.location && !menu.folder; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("export") }
    CompactMenu {
        title: "Print"; property bool contextAllowed: !menu.location && !menu.folder; enabled: !!menu.target.available
        CompactMenuItem { text: "Print Rendered Document…"; onTriggered: menu.later("print") }
        CompactMenuItem { text: "Print Markdown Source…"; onTriggered: menu.later("printSource") }
        CompactMenuItem { text: "Paginated Preview…"; onTriggered: menu.later("printPreview") }
    }
    CompactMenu {
        title: "Copy"; property bool contextAllowed: !menu.location && !menu.folder
        CompactMenuItem { text: "Path"; onTriggered: menu.library.copyPath(menu.target.url) }
        CompactMenuItem { text: "Markdown"; enabled: !!menu.target.available; onTriggered: menu.later("copyMarkdown") }
        CompactMenuItem { text: "Plain Text"; enabled: !!menu.target.available; onTriggered: menu.later("copyText") }
        CompactMenuItem { text: "HTML"; enabled: !!menu.target.available; onTriggered: menu.later("copyHtml") }
    }
    CompactMenuItem { text: "Copy Library Path"; visible: menu.location || menu.folder; height: visible ? implicitHeight : 0; onTriggered: menu.library.copyPath(menu.target.url) }
    MenuSeparator { visible: !menu.location; height: visible ? implicitHeight : 0 }
    CompactMenuItem { text: "New File…"; visible: !menu.location; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("newFile") }
    CompactMenuItem { text: "New Folder…"; visible: !menu.location; height: visible ? implicitHeight : 0; enabled: !!menu.target.available; onTriggered: menu.later("newFolder") }
    MenuSeparator { visible: !menu.location; height: visible ? implicitHeight : 0 }
    CompactMenu {
        title: "Sort By"; property bool contextAllowed: !menu.location
        CompactMenuItem { text: "Name"; checkable: true; checked: menu.library.sortMode === 0; onTriggered: menu.library.sortMode = 0 }
        CompactMenuItem { text: "Date Modified"; checkable: true; checked: menu.library.sortMode === 1; onTriggered: menu.library.sortMode = 1 }
        CompactMenuItem { text: "Date Created"; checkable: true; checked: menu.library.sortMode === 2; onTriggered: menu.library.sortMode = 2 }
        CompactMenuItem { text: "Extension"; checkable: true; checked: menu.library.sortMode === 3; onTriggered: menu.library.sortMode = 3 }
        MenuSeparator {}
        CompactMenuItem { text: "A to Z"; checkable: true; checked: menu.library.ascending; onTriggered: menu.library.ascending = true }
        CompactMenuItem { text: "Z to A"; checkable: true; checked: !menu.library.ascending; onTriggered: menu.library.ascending = false }
        CompactMenuItem { text: "Pin Folders to Top"; checkable: true; checked: menu.library.foldersFirst; onTriggered: menu.library.foldersFirst = !menu.library.foldersFirst }
    }
    CompactMenu {
        title: "View Options"; property bool contextAllowed: !menu.location
        CompactMenuItem { text: "Show Date"; checkable: true; checked: menu.commands.libraryPane.showDates; onTriggered: menu.commands.run("dates") }
        CompactMenuItem { text: "Show Text Excerpts"; checkable: true; checked: menu.commands.libraryPane.showExcerpts; onTriggered: menu.commands.run("excerpts") }
        CompactMenuItem { text: "Show Sort Bar"; checkable: true; checked: menu.commands.libraryPane.showSortBar; onTriggered: menu.commands.run("sortBar") }
        CompactMenuItem { text: "Show Filter Bar"; checkable: true; checked: menu.commands.libraryPane.showFilterBar; onTriggered: menu.commands.run("filterBar") }
    }
    Dialog {
        id: nameDialog
        property string action
        parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 360
        ColumnLayout {
            width: parent.width
            TextField { id: nameField; Layout.fillWidth: true; maximumLength: 200; Accessible.name: "Item name"; onAccepted: nameDialog.submit() }
            Label { id: nameError; Layout.fillWidth: true; wrapMode: Text.Wrap; visible: text !== "" }
        }
        function submit() {
            const ok = backend.libraryItemAction(menu.target.url, action, nameField.text);
            if (ok) close(); else nameError.text = backend.status;
        }
        footer: DialogButtonBox {
            Button { text: "Cancel"; DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
            Button { text: "OK"; enabled: nameField.text.trim().length > 0; onClicked: nameDialog.submit() }
            onRejected: nameDialog.close()
        }
    }
    Dialog {
        id: messageDialog
        parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 420
        title: "Library"; standardButtons: Dialog.Ok
        Label { id: message; width: parent.width; wrapMode: Text.Wrap; textFormat: Text.PlainText }
    }
    Dialog {
        id: trashDialog
        parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 380
        title: "Move to Trash?"; standardButtons: Dialog.Ok | Dialog.Cancel
        Label { width: parent.width; wrapMode: Text.Wrap; text: "Move “" + menu.target.name + "”" + (menu.folder ? " and its contents" : "") + " to Trash? You can restore it using Finder."; textFormat: Text.PlainText }
        onAccepted: menu.run("trash")
    }
    Dialog {
        id: exportChoice
        parent: Overlay.overlay; anchors.centerIn: parent; modal: true; width: 280
        title: "Export Document"; standardButtons: Dialog.Cancel
        Column {
            Button { text: "HTML…"; onClicked: { exportFile.format="exportHtml"; exportFile.nameFilters=["HTML (*.html)"]; exportChoice.close(); exportFile.open(); } }
            Button { text: "PDF…"; onClicked: { exportFile.format="exportPdf"; exportFile.nameFilters=["PDF (*.pdf)"]; exportChoice.close(); exportFile.open(); } }
        }
    }
    Dialogs.FileDialog {
        id: exportFile
        property string format
        title: "Export Document"; fileMode: Dialogs.FileDialog.SaveFile
        onAccepted: menu.run(format, selectedFile.toString())
    }
}
