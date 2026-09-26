import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// This component exposes only the local, bounded style controls owned by Backend.
ScrollView {
    id: gallery
    required property var backend
    property var builtIns: []
    property var userStyles: []
    property string editingId: ""
    property string feedback: ""
    signal styleChanged()

    clip: true
    // Reserve a gutter: the vertical thumb must never sit on top of a card.
    leftPadding: 3
    rightPadding: 16
    contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AsNeeded
        implicitWidth: 7
        contentItem: Rectangle { radius: 3; color: backend.palette.muted }
    }

    function refresh() {
        builtIns = backend.builtInOutputStyles()
        userStyles = backend.userOutputStyles()
        if (editingId.length > 0)
            loadEditor(editingId)
    }
    function selectBuiltIn(style) {
        editingId = ""
        backend.setOutputStyle(style.id)
        feedback = style.name + " selected."
        styleChanged()
    }
    function selectUser(style) {
        if (backend.selectUserOutputStyle(style.id)) {
            editingId = style.id
            loadEditor(style.id)
            feedback = style.name + " selected."
            styleChanged()
        } else
            feedback = backend.status
        refresh()
    }
    function loadEditor(id) {
        var style = null
        for (var i = 0; i < userStyles.length; ++i) {
            if (userStyles[i].id === id) { style = userStyles[i]; break }
        }
        if (!style) { editingId = ""; return }
        editorName.text = style.name
        editorFont.text = style.fontFamily
        editorSize.value = style.pointSize
        editorHeader.text = style.header
        editorFooter.text = style.footer
        editorTitlePage.checked = style.titlePage
        editorPageFurniture.checked = style.pageFurniture
    }
    function createCopy() {
        var name = copyName.text.trim()
        if (name.length === 0) { feedback = "Give the copy a name."; return }
        var created = backend.createUserOutputStyleFromCurrent(name)
        if (created.error) { feedback = created.error; return }
        copyName.text = ""
        if (!backend.selectUserOutputStyle(created.id)) {
            feedback = backend.status
            refresh()
            return
        }
        editingId = created.id
        refresh()
        loadEditor(created.id)
        feedback = "Created " + created.name + "."
        styleChanged()
    }
    function saveEditor() {
        if (editingId.length === 0) return
        var changes = { name: editorName.text.trim(), fontFamily: editorFont.text.trim(),
            pointSize: Math.round(editorSize.value), header: editorHeader.text,
            footer: editorFooter.text, titlePage: editorTitlePage.checked,
            pageFurniture: editorPageFurniture.checked }
        if (backend.updateUserOutputStyle(editingId, changes)) {
            feedback = "Style changes applied to the live preview."
            styleChanged()
            refresh()
        } else
            feedback = backend.status
    }
    function removeEditor() {
        if (editingId.length === 0) return
        var removedName = editorName.text
        if (backend.deleteUserOutputStyle(editingId)) {
            editingId = ""
            feedback = "Deleted " + removedName + "."
            refresh()
            styleChanged()
        } else
            feedback = backend.status
    }

    Connections {
        target: gallery.backend
        function onOutputStyleChanged() { gallery.refresh() }
    }

    ColumnLayout {
        objectName: "styleGalleryColumn"
        width: gallery.availableWidth
        spacing: 10
        Label { text: "Built-in styles"; color: backend.palette.muted; font.pixelSize: 12; font.weight: Font.DemiBold; Accessible.role: Accessible.Heading }
        Repeater {
            model: gallery.builtIns
            delegate: Button {
                required property var modelData
                Layout.fillWidth: true; Layout.preferredHeight: 58; checkable: true
                checked: backend.outputStyle === modelData.id
                onClicked: gallery.selectBuiltIn(modelData)
                Accessible.name: modelData.name + ", " + modelData.font
                background: Rectangle {
                    radius: 7
                    color: parent.checked ? backend.palette.field : parent.hovered ? backend.palette.hover : "transparent"
                    border.color: parent.checked ? backend.themeAccent : backend.palette.border
                    border.width: parent.checked ? 2 : 1
                }
                contentItem: Item {
                    Column {
                        x: 13; anchors.verticalCenter: parent.verticalCenter; spacing: 3
                        Label { text: modelData.name; color: backend.palette.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                        Label { text: modelData.font; color: backend.palette.muted; font.pixelSize: 12 }
                    }
                }
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: backend.palette.border; Layout.topMargin: 4; Layout.bottomMargin: 2 }
        Label { text: "Your styles"; color: backend.palette.muted; font.pixelSize: 12; font.weight: Font.DemiBold; Accessible.role: Accessible.Heading }
        Repeater {
            model: gallery.userStyles
            delegate: Button {
                required property var modelData
                Layout.fillWidth: true; Layout.preferredHeight: 58; checkable: true
                checked: backend.outputStyle === 3 && backend.selectedUserOutputStyleId() === modelData.id
                onClicked: gallery.selectUser(modelData)
                Accessible.name: modelData.name + ", user style, " + modelData.fontFamily
                background: Rectangle {
                    radius: 7
                    color: parent.checked ? backend.palette.field : parent.hovered ? backend.palette.hover : "transparent"
                    border.color: parent.checked ? backend.themeAccent : backend.palette.border
                    border.width: parent.checked ? 2 : 1
                }
                contentItem: Item {
                    Column {
                        x: 13; anchors.verticalCenter: parent.verticalCenter; spacing: 3
                        Label { text: modelData.name; color: backend.palette.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                        Label { text: modelData.fontFamily + " · " + modelData.pointSize + " pt"; color: backend.palette.muted; font.pixelSize: 12 }
                    }
                }
            }
        }
        Label { visible: gallery.userStyles.length === 0; text: "No saved styles yet."; color: backend.palette.muted; font.pixelSize: 12 }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 6
            TextField { id: copyName; Layout.fillWidth: true; placeholderText: "Name this copy"; maximumLength: 80; Accessible.name: "Name for style copy"; onAccepted: gallery.createCopy() }
            Button {
                Layout.fillWidth: true; text: "Duplicate current"; flat: true
                background: Rectangle { radius: 6; color: parent.hovered ? backend.palette.hover : backend.palette.panel; border.color: backend.palette.border }
                onClicked: gallery.createCopy(); Accessible.name: "Duplicate current style"
            }
        }
        ColumnLayout {
            visible: gallery.editingId.length > 0; Layout.fillWidth: true; spacing: 7
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: backend.palette.border; Layout.topMargin: 5 }
            Label { text: "Edit selected style"; color: backend.palette.muted; font.pixelSize: 12; font.weight: Font.DemiBold; Accessible.role: Accessible.Heading }
            Label { text: "Changes are limited to current output controls and never change Markdown."; wrapMode: Text.Wrap; Layout.fillWidth: true; color: backend.palette.muted; font.pixelSize: 11 }
            Label { text: "Name"; color: backend.palette.text; font.pixelSize: 11 }
            TextField { id: editorName; Layout.fillWidth: true; maximumLength: 80; Accessible.name: "Style name" }
            Label { text: "Font family"; color: backend.palette.text; font.pixelSize: 11 }
            TextField { id: editorFont; Layout.fillWidth: true; maximumLength: 128; Accessible.name: "Style font family" }
            Label { text: "Point size"; color: backend.palette.text; font.pixelSize: 11 }
            SpinBox { id: editorSize; from: 8; to: 32; value: 12; editable: true; Accessible.name: "Style point size" }
            Label { text: "Header (optional; {title}, {page}, {pages})"; color: backend.palette.text; font.pixelSize: 11; wrapMode: Text.Wrap; Layout.fillWidth: true }
            TextField { id: editorHeader; Layout.fillWidth: true; maximumLength: 200; Accessible.name: "Style header" }
            Label { text: "Footer (optional; {title}, {page}, {pages})"; color: backend.palette.text; font.pixelSize: 11; wrapMode: Text.Wrap; Layout.fillWidth: true }
            TextField { id: editorFooter; Layout.fillWidth: true; maximumLength: 200; Accessible.name: "Style footer" }
            CheckBox { id: editorPageFurniture; text: "PDF header and footer"; Accessible.name: "Show header and footer in PDF" }
            CheckBox { id: editorTitlePage; text: "PDF title page"; Accessible.name: "Add a title page to PDF" }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 6
                Button {
                    Layout.fillWidth: true; text: "Apply to preview"; flat: true
                    background: Rectangle { radius: 6; color: parent.hovered ? backend.palette.hover : backend.palette.field; border.color: backend.palette.border }
                    onClicked: gallery.saveEditor(); Accessible.name: "Apply style changes to live preview"
                }
                Button {
                    Layout.fillWidth: true; text: "Delete style"; flat: true
                    background: Rectangle { radius: 6; color: parent.hovered ? backend.palette.hover : backend.palette.panel; border.color: backend.palette.border }
                    onClicked: gallery.removeEditor(); Accessible.name: "Delete selected user style"
                }
            }
        }
        Label { visible: gallery.feedback.length > 0; text: gallery.feedback; wrapMode: Text.Wrap; Layout.fillWidth: true; color: backend.palette.muted; font.pixelSize: 12 }
        Item { Layout.preferredHeight: 2 }
    }
    Component.onCompleted: refresh()
}
