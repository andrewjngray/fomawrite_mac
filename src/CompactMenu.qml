import QtQuick
import QtQuick.Controls

Menu {
    id: menu
    property bool darkMode: false
    delegate: CompactMenuItem {}
    width: 224
    padding: 6
    font.family: Qt.application.font.family
    font.pixelSize: 13
    popupType: Popup.Item
    // Respond immediately when a footer menu is reopened after a command.
    // Material’s exit fade can otherwise consume the next opening click.
    enter: Transition {}
    exit: Transition {}
    background: Rectangle {
        radius: 8
        color: backend.palette.popover || backend.palette.panel
        border.color: backend.palette.border
        border.width: 1
    }
}
