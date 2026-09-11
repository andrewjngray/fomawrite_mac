import QtQuick
import QtQuick.Controls

Menu {
    id: menu
    property bool darkMode: false
    delegate: CompactMenuItem {}
    width: 224
    padding: 5
    font.family: "Helvetica Neue"
    font.pixelSize: 13
    popupType: Popup.Item
    background: Rectangle {
        radius: 10
        color: menu.darkMode ? "#292b30" : "#fafafa"
        border.color: menu.darkMode ? "#484b52" : "#d5d7da"
        border.width: 1
    }
}
