import QtQuick
import QtQuick.Controls

Menu {
    id: menu
    property bool darkMode: false
    delegate: CompactMenuItem {}
    width: 224
    padding: 5
    font.family: Qt.platform.os === "osx" ? Qt.application.font.family : "Helvetica Neue"
    font.pixelSize: 13
    popupType: Popup.Item
    background: Rectangle {
        radius: 10
        color: backend.palette.panel
        border.color: backend.palette.border
        border.width: 1
    }
}
