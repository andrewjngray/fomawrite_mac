import QtQuick
import QtQuick.Controls

CompactMenu {
    id: menu
    required property var library
    property var target: ({})
    property bool location: false
    property bool renamePending: false
    function showFor(item, entry, isLocation, point) {
        target = entry;
        location = isLocation;
        popup(item, point.x, point.y);
    }
    CompactMenuItem {
        text: "Rename in Locations…"
        visible: menu.location
        height: visible ? implicitHeight : 0
        onTriggered: menu.renamePending = true
    }
    CompactMenuItem {
        text: "Remove from Locations"
        visible: menu.location
        height: visible ? implicitHeight : 0
        onTriggered: menu.library.removeLocation(menu.target.url)
    }
    MenuSeparator { visible: menu.location; height: visible ? implicitHeight : 0 }
    CompactMenuItem {
        text: menu.location ? "Copy Library Path" : "Copy Path"
        onTriggered: menu.library.copyPath(menu.target.url)
    }
    MenuSeparator {}
    CompactMenuItem {
        text: Qt.platform.os === "osx" ? "Show in Finder" : "Show in File Manager"
        enabled: !!menu.target.available
        onTriggered: menu.library.showInFileManager(menu.target.url)
    }
    onClosed: if (renamePending) { renamePending = false; renameDialog.open(); }
    Dialog {
        id: renameDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        title: "Rename in Locations"
        modal: true
        width: 320
        standardButtons: Dialog.Ok | Dialog.Cancel
        onOpened: { labelField.text = menu.target.name; labelField.forceActiveFocus(); labelField.selectAll(); }
        onAccepted: menu.library.renameLocation(menu.target.url, labelField.text)
        TextField {
            id: labelField
            width: parent.width
            maximumLength: 200
            Accessible.name: "Location label"
            onTextChanged: renameDialog.standardButton(Dialog.Ok).enabled = text.trim().length > 0
            onAccepted: if (text.trim().length > 0) renameDialog.accept()
        }
    }
}
