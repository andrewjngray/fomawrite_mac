pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Window

// Both document panes use the same fixed-width, view-only zoom controls.
Item {
    id: root
    required property var zoomController
    required property string pane
    property bool controlsActive: true
    readonly property int zoomPercent: pane === "source" ? zoomController.sourceZoom : zoomController.previewZoom
    readonly property string paneName: pane === "source" ? "Editor" : "Preview"
    readonly property string controlPrefix: controlsActive ? pane + "Zoom" : "inactive" + paneName + "Zoom"
    property var previousFocusItem: null
    objectName: controlPrefix + "Controls"
    implicitWidth: 120
    implicitHeight: 34

    function openMenu() {
        var owner = root.Window.window;
        previousFocusItem = owner ? owner.activeFocusItem : null;
        if (owner) {
            var point = root.mapToItem(owner.contentItem, 0, root.height + 6);
            zoomMenu.parent = owner.contentItem;
            zoomMenu.x = Math.max(8, Math.min(owner.width - zoomMenu.width - 8, point.x));
            zoomMenu.y = point.y;
        }
        zoomMenu.open();
    }

    ToolbarGroup {
        anchors.fill: parent
        ToolbarButton {
            objectName: root.controlPrefix + "OutButton"
            grouped: true
            focusPolicy: Qt.TabFocus
            iconName: "minus"
            hint: "Decrease " + root.paneName.toLowerCase() + " zoom"
            enabled: root.zoomPercent > 75
            onClicked: root.zoomController.adjustZoom(root.pane, -10)
        }
        ToolbarButton {
            objectName: root.controlPrefix + "MenuButton"
            grouped: true
            showDivider: true
            focusPolicy: Qt.TabFocus
            implicitWidth: 54
            text: root.zoomPercent + "%"
            hint: root.paneName + " zoom — " + root.zoomPercent + "%"
            onClicked: root.openMenu()
        }
        ToolbarButton {
            objectName: root.controlPrefix + "InButton"
            grouped: true
            showDivider: true
            focusPolicy: Qt.TabFocus
            iconName: "plus"
            hint: "Increase " + root.paneName.toLowerCase() + " zoom"
            enabled: root.zoomPercent < 200
            onClicked: root.zoomController.adjustZoom(root.pane, 10)
        }
    }
    CompactMenu {
        id: zoomMenu
        objectName: root.controlPrefix + "Menu"
        onClosed: {
            var previous = root.previousFocusItem;
            root.previousFocusItem = null;
            if (previous && previous.visible && previous.enabled)
                previous.forceActiveFocus(Qt.OtherFocusReason);
        }
        Instantiator {
            model: [75, 90, 100, 110, 125, 150, 175, 200]
            onObjectAdded: (index, item) => zoomMenu.insertItem(index, item)
            onObjectRemoved: (index, item) => zoomMenu.removeItem(item)
            delegate: CompactMenuItem {
                required property int modelData
                objectName: root.controlPrefix + "Preset" + modelData
                text: modelData + "%"
                checkable: true
                checked: root.zoomPercent === modelData
                onTriggered: {
                    root.zoomController.setZoom(root.pane, modelData);
                    // A checkable item can remove its binding when reselected.
                    checked = Qt.binding(function() { return root.zoomPercent === modelData; });
                }
            }
        }
        MenuSeparator {}
        CompactMenuItem {
            objectName: root.controlPrefix + "Reset"
            text: "Reset to 100%"
            onTriggered: root.zoomController.resetZoom(root.pane)
        }
        MenuSeparator {}
        CompactMenuItem {
            objectName: root.controlPrefix + "Link"
            text: "Link zoom"
            checkable: true
            checked: root.zoomController.linked
            onTriggered: {
                root.zoomController.setLinked(checked, root.pane);
                checked = Qt.binding(function() { return root.zoomController.linked; });
            }
        }
    }
}
