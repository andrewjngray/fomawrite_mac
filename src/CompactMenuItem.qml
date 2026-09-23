import QtQuick
import QtQuick.Controls

MenuItem {
    id: item
    property bool darkMode: menu ? menu.darkMode : false
    property string iconName: subMenu ? (subMenu.title === "Sort By" ? "sort" : "outline") : ""
    implicitHeight: 28
    leftPadding: 24
    rightPadding: 10
    topPadding: 0
    bottomPadding: 0
    font.family: Qt.platform.os === "osx" ? Qt.application.font.family : "Helvetica Neue"
    font.pixelSize: 13
    LineIcon {
        x: 5; width: 16; height: 16
        anchors.verticalCenter: parent.verticalCenter
        name: item.iconName
        visible: item.iconName !== "" && !item.checkable
        ink: item.highlighted ? "#ffffff" : backend.palette.text
    }
    arrow: LineIcon {
        x: item.width - width - 8
        anchors.verticalCenter: parent.verticalCenter
        name: "right"; visible: item.subMenu !== null
        ink: item.highlighted ? "#ffffff" : backend.palette.text
    }
    indicator: Text {
        x: 7
        anchors.verticalCenter: parent.verticalCenter
        text: item.checked ? "✓" : ""
        font.pixelSize: 13
        color: item.highlighted ? "#ffffff" : backend.palette.text
    }
    contentItem: Text {
        text: item.text
        font: item.font
        verticalAlignment: Text.AlignVCenter
        color: !item.enabled ? "#92969e" : item.highlighted ? "#ffffff" : backend.palette.text
    }
    background: Rectangle {
        radius: 5
        color: item.highlighted ? "#244f88" : "transparent"
    }
}
