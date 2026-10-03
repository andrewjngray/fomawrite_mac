import QtQuick
import QtQuick.Controls

MenuItem {
    id: item
    property bool darkMode: menu ? menu.darkMode : false
    property string iconName: subMenu ? (subMenu.title === "Sort By" ? "sort" : "outline") : ""
    implicitHeight: 28
    leftPadding: 28
    rightPadding: subMenu ? 28 : 10
    topPadding: 0
    bottomPadding: 0
    font.family: Qt.application.font.family
    font.pixelSize: 13
    Accessible.name: text
    LineIcon {
        x: 7
        width: 16
        height: 16
        anchors.verticalCenter: parent.verticalCenter
        name: item.iconName
        visible: item.iconName !== "" && !item.checkable
        ink: backend.palette.text
        opacity: item.enabled ? 1 : 0.45
    }
    arrow: LineIcon {
        x: item.width - width - 8
        anchors.verticalCenter: parent.verticalCenter
        name: "right"
        visible: item.subMenu !== null
        ink: backend.palette.text
        opacity: item.enabled ? 1 : 0.45
    }
    indicator: LineIcon {
        x: 7
        width: 16
        height: 16
        anchors.verticalCenter: parent.verticalCenter
        name: "check"
        visible: item.checkable && item.checked
        ink: backend.palette.text
        opacity: item.enabled ? 1 : 0.45
    }
    contentItem: Text {
        text: item.text
        font: item.font
        verticalAlignment: Text.AlignVCenter
        color: backend.palette.text
        opacity: item.enabled ? 1 : 0.45
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 5
        color: item.down ? (backend.palette.controlPressed || backend.palette.selectedRow)
            : item.highlighted ? (backend.palette.controlHover || backend.palette.hover) : "transparent"
        border.width: item.activeFocus ? 2 : 0
        border.color: backend.palette.focus
    }
}
