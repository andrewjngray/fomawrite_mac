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
    background: Rectangle {
        radius: 8
        color: backend.palette.popover || backend.palette.panel
        border.color: backend.palette.border
        border.width: 1
    }
}
