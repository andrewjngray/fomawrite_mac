pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as Dialogs

CompactMenu {
    id: menu
    objectName: "previewLinkMenu"
    required property var renderer
    property url target
    property int targetPage: -1
    property point targetPoint
    property bool fromWeb: false
    property bool internal: false
    property var info: ({})
    readonly property var applications: info.applications || []
    signal openRequested(url link, int page, point location, bool fromWeb)

    delegate: CompactMenuItem {
        visible: !subMenu || menu.applications.length > 0
        height: visible ? implicitHeight : 0
    }

    function showFor(anchor, point, link, page, location, web, inDocument) {
        target = inDocument ? link : renderer.resolveDocumentLink(link);
        targetPage = page;
        targetPoint = location;
        fromWeb = web;
        internal = inDocument;
        info = internal ? ({}) : renderer.previewLinkInfo(target);
        if (internal || info.local || info.web)
            Qt.callLater(function() { if (anchor.visible) menu.popup(anchor, point.x, point.y); });
    }

    CompactMenuItem {
        objectName: "previewLinkOpen"
        text: menu.internal ? "Go to Link"
            : menu.info.local ? (menu.info.file ? "Open File" : "Open Linked Item")
            : String(menu.target).startsWith("mailto:") ? "Open in Mail App" : "Open in Browser"
        enabled: menu.internal || menu.info.web || !!menu.info.available
        onTriggered: menu.openRequested(menu.target, menu.targetPage, menu.targetPoint, menu.fromWeb)
    }
    CompactMenuItem {
        objectName: "previewLinkOpenWithPicker"
        text: "Open With…"
        visible: Qt.platform.os === "windows" && !!menu.info.file
        height: visible ? implicitHeight : 0
        onTriggered: menu.renderer.previewLinkAction(menu.target, "openWith")
    }
    CompactMenu {
        id: applicationsMenu
        title: "Open With"
        Instantiator {
            model: menu.applications
            delegate: CompactMenuItem {
                required property var modelData
                text: modelData.name
                onTriggered: menu.renderer.previewLinkAction(menu.target, "openWith", modelData.url)
            }
            onObjectAdded: function(index, item) { applicationsMenu.insertItem(index, item); }
            onObjectRemoved: function(index, item) { applicationsMenu.removeItem(item); }
        }
    }
    MenuSeparator { visible: !menu.internal; height: visible ? implicitHeight : 0 }
    CompactMenuItem {
        objectName: "previewLinkSaveAs"
        text: "Save As…"
        visible: !!menu.info.file
        height: visible ? implicitHeight : 0
        onTriggered: {
            saveDialog.source = menu.target;
            saveDialog.selectedFile = menu.info.suggestedCopy;
            saveDialog.open();
        }
    }
    CompactMenuItem {
        objectName: "previewLinkCopyPath"
        text: menu.info.local ? "Copy Path" : "Copy Link Address"
        visible: !menu.internal
        height: visible ? implicitHeight : 0
        onTriggered: menu.renderer.previewLinkAction(menu.target, menu.info.local ? "copyPath" : "copyAddress")
    }
    CompactMenuItem {
        objectName: "previewLinkCopyContents"
        text: "Copy File Contents"
        visible: !!menu.info.text
        height: visible ? implicitHeight : 0
        onTriggered: menu.renderer.previewLinkAction(menu.target, "copyContents")
    }
    CompactMenuItem {
        objectName: "previewLinkReveal"
        text: Qt.platform.os === "osx" ? "Reveal in Finder" : "Show in File Manager"
        visible: !!menu.info.local
        height: visible ? implicitHeight : 0
        enabled: !!menu.info.available
        onTriggered: menu.renderer.previewLinkAction(menu.target, "reveal")
    }

    Dialogs.FileDialog {
        id: saveDialog
        objectName: "previewLinkSaveDialog"
        property url source
        title: "Save Linked File As"
        fileMode: Dialogs.FileDialog.SaveFile
        onAccepted: menu.renderer.previewLinkAction(source, "save", selectedFile)
    }
}
