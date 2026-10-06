import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: hub
    objectName: "exportHub"
    required property var backend
    required property var renderer
    required property string markdown
    required property url documentBaseUrl
    required property bool darkMode
    property string selectedFormat: "pdf"
    property string cssName: backend.outputCssName()
    property string cssFeedback: ""
    property int previewLayoutMode: 2
    readonly property real minimumBandsWidth: 220 + 260 + 360 + 28
    property real requestedWidth: 1100
    property real requestedHeight: 720
    property real requestedX: 16
    property real requestedY: 16
    property bool geometryInitialized: false
    signal destinationRequested(string format)
    Connections {
        target: hub.backend
        function onOutputCssChanged() { hub.cssName = hub.backend.outputCssName() }
    }
    FileDialog {
        id: cssPicker
        title: "Choose additional local publishing CSS"
        nameFilters: ["CSS files (*.css)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            if (hub.backend.loadOutputCss(selectedFile))
                hub.cssFeedback = "CSS applied to Web and PDF publishing output."
            else
                hub.cssFeedback = hub.backend.status
        }
    }
    FileDialog {
        id: themePicker
        objectName: "exportThemePicker"
        title: "Import Publishing Theme"
        nameFilters: ["CSS themes (*.css)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            hub.backend.importPublishingTheme(selectedFile)
            hub.cssFeedback = hub.backend.status
        }
    }
    component SecondaryAction: Button {
        id: action
        property bool selected: false
        font.pixelSize: 13
        font.weight: Font.DemiBold
        leftPadding: 14; rightPadding: 14
        topPadding: 7; bottomPadding: 7
        implicitHeight: 36
        implicitWidth: Math.max(96, contentItem.implicitWidth + leftPadding + rightPadding)
        background: Rectangle {
            radius: 7
            color: action.selected || action.down || action.hovered ? backend.palette.hover : backend.palette.field
            border.color: action.visualFocus ? backend.palette.focus
                : action.selected || action.hovered || action.down ? backend.themeAccent : backend.palette.border
            border.width: action.selected ? 2 : 1
        }
        contentItem: Label {
            text: action.text; font: action.font; color: action.enabled ? backend.palette.text : backend.palette.muted
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        }
    }

    title: "Export and share"
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    onAboutToShow: {
        if (!geometryInitialized) {
            requestedWidth = Math.max(0, Math.min(1100, parent.width - 32))
            requestedHeight = Math.max(0, Math.min(720, parent.height - 32))
            requestedX = (parent.width - requestedWidth) / 2
            requestedY = (parent.height - requestedHeight) / 2
            geometryInitialized = true
        }
    }
    // Popup positioning may adjust x/y while a window resizes. Keep size
    // independent of that adjustment so it cannot feed back into positioning.
    width: Math.max(0, Math.min(requestedWidth, parent.width - 32))
    height: Math.max(0, Math.min(requestedHeight, parent.height - 32))
    x: Math.max(16, Math.min(requestedX, parent.width - width - 16))
    y: Math.max(16, Math.min(requestedY, parent.height - height - 16))
    padding: 0
    standardButtons: Dialog.NoButton
    background: Rectangle { color: backend.palette.panel; border.color: backend.palette.border; radius: 12 }
    header: Rectangle {
        implicitHeight: 76; color: "transparent"
        Label {
            x: 22; y: 14; text: hub.title
            color: backend.palette.text; font.pixelSize: 20; font.weight: Font.DemiBold
            Accessible.role: Accessible.Heading
        }
        Label {
            x: 22; y: 44; width: parent.width - 44
            text: "Choose a format and style, then save or share."
            elide: Text.ElideRight
            color: backend.palette.muted; font.pixelSize: 13
        }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: backend.palette.border }
    }
    contentItem: ColumnLayout {
        spacing: 0
        Flickable {
            id: horizontalViewport
            objectName: "exportHorizontalViewport"
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 20
            clip: true
            interactive: false // Preserve mouse drags on the two band dividers.
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: Math.max(width, hub.minimumBandsWidth)
            contentHeight: height
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOff }
            ScrollBar.horizontal: ScrollBar {
                id: horizontalBar
                objectName: "exportHorizontalScrollBar"
                policy: horizontalViewport.contentWidth > horizontalViewport.width + 1
                    ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                height: 12
                background: Rectangle { radius: 6; color: backend.palette.field }
                contentItem: Rectangle { objectName: "exportHorizontalThumb"; radius: 6; color: backend.palette.muted }
            }
        SplitView {
            id: bands
            objectName: "exportBands"
            width: horizontalViewport.contentWidth
            height: horizontalViewport.height - (horizontalBar.policy === ScrollBar.AlwaysOn ? horizontalBar.height + 4 : 0)
            orientation: Qt.Horizontal
            handle: Item {
                id: splitterHandle
                objectName: "exportBandHandle"
                readonly property bool active: SplitHandle.hovered || SplitHandle.pressed
                implicitWidth: 14
                Rectangle {
                    anchors.centerIn: parent
                    width: splitterHandle.active ? 2 : 1
                    height: parent.height
                    color: splitterHandle.active ? backend.themeAccent : backend.palette.border
                }
                Column {
                    anchors.centerIn: parent
                    spacing: 4
                    Repeater {
                        model: 3
                        Rectangle {
                            width: 3; height: 3; radius: 2
                            color: splitterHandle.active ? backend.themeAccent : backend.palette.muted
                        }
                    }
                }
            }
            Item {
                id: optionsBand
                objectName: "exportOptionsBand"
                SplitView.minimumWidth: 220
                SplitView.preferredWidth: 280
                ScrollView {
                id: optionsScroll
                objectName: "exportOptionsScroll"
                anchors.fill: parent; anchors.rightMargin: 12
                clip: true; rightPadding: 24; contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical: ScrollBar {
                    id: optionsBar
                    objectName: "exportOptionsScrollBar"
                    parent: optionsScroll
                    x: optionsScroll.width - width - 3
                    y: 0
                    height: optionsScroll.height
                    policy: ScrollBar.AsNeeded
                    implicitWidth: 7
                    background: Item {}
                    contentItem: Rectangle { radius: 3; color: backend.palette.muted; opacity: optionsBar.active ? 0.55 : 0.25 }
                }
                ColumnLayout {
                width: optionsScroll.availableWidth; spacing: 12
                Label { text: "Output"; color: backend.palette.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                ComboBox {
                    objectName: "exportFormatChoice"; Layout.fillWidth: true
                    model: [{ label: "PDF document", value: "pdf" }, { label: "HTML document", value: "html" }]
                    textRole: "label"; valueRole: "value"; currentIndex: hub.selectedFormat === "html" ? 1 : 0
                    onActivated: hub.selectedFormat = currentValue; Accessible.name: "Export format"
                }
                Label { text: "Publishing theme"; color: backend.palette.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                Label { Layout.fillWidth: true; text: backend.publishingThemeName + (backend.publishingThemeId.length === 0 ? " · " + backend.outputFont + " · " + backend.outputPointSize + " pt" : ""); wrapMode: Text.Wrap; color: backend.palette.muted; font.pixelSize: 13; Accessible.name: "Current output style: " + text }
                ComboBox {
                    id: publishingThemeChoice
                    objectName: "exportPublishingThemeChoice"
                    Layout.fillWidth: true
                    model: {
                        var choices = [{ id: "", name: "Basic: " + backend.outputTemplateName }]
                        var themes = backend.publishingThemes
                        var selectedFound = backend.publishingThemeId.length === 0
                        for (var i = 0; i < themes.length; ++i) {
                            choices.push({ id: themes[i].id, name: themes[i].id === "claude-like" ? "Claude Like" : themes[i].name })
                            if (themes[i].id === backend.publishingThemeId) selectedFound = true
                        }
                        if (!selectedFound) choices.push({ id: backend.publishingThemeId, name: "Unavailable: " + backend.publishingThemeName })
                        return choices
                    }
                    textRole: "name"; valueRole: "id"
                    function acceptedThemeIndex() {
                        for (var i = 0; i < model.length; ++i)
                            if (model[i].id === backend.publishingThemeId) return i
                        return 0
                    }
                    currentIndex: acceptedThemeIndex()
                    onActivated: {
                        hub.cssFeedback = ""
                        if (currentValue.length === 0) backend.setOutputStyle(backend.outputStyle)
                        else if (!backend.selectPublishingTheme(currentValue)) hub.cssFeedback = backend.publishingThemeError || backend.status
                        currentIndex = Qt.binding(function() { return publishingThemeChoice.acceptedThemeIndex() })
                    }
                    Accessible.name: "Publishing theme for Web and PDF"
                }
                Label { objectName: "exportThemeNotice"; visible: backend.publishingThemeError.length > 0; text: backend.publishingThemeError; Layout.fillWidth: true; wrapMode: Text.Wrap; color: hub.darkMode ? "#fca5a5" : "#b42318"; font.pixelSize: 12; Accessible.name: text }
                Label { Layout.fillWidth: true; text: "CSS themes come from your Themes folder. Basic styles use the font and page settings in the next column."; wrapMode: Text.Wrap; color: backend.palette.muted; font.pixelSize: 11 }
                SecondaryAction { objectName: "exportGetMoreThemesButton"; Layout.fillWidth: true; text: "Get More Themes…"; onClicked: Qt.openUrlExternally("https://theme.typora.io/"); Accessible.name: "Browse the Typora theme gallery" }
                SecondaryAction { objectName: "exportImportThemeButton"; Layout.fillWidth: true; text: "Import Theme…"; onClicked: themePicker.open(); Accessible.name: "Import publishing CSS theme" }
                ColumnLayout {
                    visible: true
                    Layout.fillWidth: true; spacing: 4
                    Label { text: "Additional publishing CSS: " + hub.cssName; Layout.fillWidth: true; elide: Text.ElideMiddle; color: backend.palette.muted; font.pixelSize: 11; Accessible.name: text }
                    RowLayout {
                        Layout.fillWidth: true
                        SecondaryAction { text: "Choose CSS…"; onClicked: cssPicker.open(); Accessible.name: "Choose additional local publishing CSS" }
                        SecondaryAction { text: "Clear"; enabled: hub.cssName !== "None"; onClicked: { backend.clearOutputCss(); hub.cssFeedback = "" }
                            Accessible.name: "Clear additional publishing CSS" }
                    }
                    Label { visible: hub.cssFeedback.length > 0; text: hub.cssFeedback; Layout.fillWidth: true; wrapMode: Text.Wrap; color: backend.palette.muted; font.pixelSize: 11 }
                }
                Label { text: "Paper"; color: backend.palette.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                ComboBox {
                    objectName: "exportPaperChoice"; Layout.fillWidth: true
                    model: [{ label: "A4", value: "a4" }, { label: "US Letter", value: "letter" }, { label: "US Legal", value: "legal" }, { label: "A5", value: "a5" }]
                    textRole: "label"; valueRole: "value"
                    currentIndex: { for (var i = 0; i < model.length; ++i) if (model[i].value === backend.exportPaperSize()) return i; return 0; }
                    onActivated: backend.setExportPaperSize(currentValue); Accessible.name: "Paper size"
                }
                Label { text: "Orientation"; color: backend.palette.text; font.pixelSize: 14; font.weight: Font.DemiBold }
                RowLayout {
                    Layout.fillWidth: true
                    ButtonGroup { id: orientationGroup }
                    RadioButton { text: "Portrait"; ButtonGroup.group: orientationGroup; checked: backend.exportOrientation() === "portrait"; onClicked: backend.setExportOrientation("portrait"); Accessible.name: "Portrait orientation" }
                    RadioButton { text: "Landscape"; ButtonGroup.group: orientationGroup; checked: backend.exportOrientation() === "landscape"; onClicked: backend.setExportOrientation("landscape"); Accessible.name: "Landscape orientation" }
                }
                Label { Layout.fillWidth: true; text: selectedFormat === "html" ? "HTML uses the selected output style. Paper settings apply when printing it." : "PDF uses the selected style, paper size and orientation."; wrapMode: Text.Wrap; color: backend.palette.muted; font.pixelSize: 12 }
                SecondaryAction { visible: hub.width < 520; Layout.fillWidth: true; text: "Share Markdown…"; onClicked: backend.nativeWindowAction("share"); Accessible.name: "Share Markdown" }
                }
                }
            }
            Item {
                id: stylesBand
                objectName: "exportStylesBand"
                SplitView.minimumWidth: 260
                SplitView.preferredWidth: 310
                StyleGallery { objectName: "exportWideGallery"; anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; backend: hub.backend; onStyleChanged: hub.forceActiveFocus() }
            }
            Item {
                id: previewBand
                objectName: "exportPreviewBand"
                SplitView.minimumWidth: 360
                SplitView.fillWidth: true
                ColumnLayout {
                anchors.fill: parent; anchors.leftMargin: 12
                spacing: 10
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "Live output preview"; color: backend.palette.text; font.pixelSize: 14; font.weight: Font.DemiBold; Layout.fillWidth: true }
                    SecondaryAction { objectName: "exportSplitButton"; text: "Split"; selected: hub.previewLayoutMode === 1; onClicked: hub.previewLayoutMode = 1; Accessible.name: "Split export preview" }
                    SecondaryAction { objectName: "exportFullButton"; text: "Full"; selected: hub.previewLayoutMode === 2; onClicked: hub.previewLayoutMode = 2; Accessible.name: "Full export preview" }
                }
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true; color: backend.palette.page; border.color: backend.palette.border; radius: 5; clip: true
                    RowLayout {
                        anchors.fill: parent
                        spacing: 0
                        ScrollView {
                            id: sourcePreviewScroll
                            visible: hub.previewLayoutMode === 1
                            Layout.fillWidth: true; Layout.fillHeight: true
                            clip: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            TextArea {
                                objectName: "exportHubSourcePreview"
                                width: sourcePreviewScroll.availableWidth
                                text: hub.markdown
                                readOnly: true
                                wrapMode: TextEdit.Wrap
                                selectByMouse: true
                                font.family: "Menlo"
                                font.pixelSize: 13
                                color: backend.palette.text
                                background: Rectangle { color: backend.palette.panel }
                                Accessible.name: "Markdown source for export preview"
                            }
                        }
                        Rectangle { visible: hub.previewLayoutMode === 1; Layout.fillHeight: true; Layout.preferredWidth: 1; color: backend.palette.border }
                        PublishingPreview { objectName: "exportHubPreviewPane"; publishingMode: hub.selectedFormat === "pdf" ? "pdf" : "web"; showFooter: false; Layout.fillWidth: true; Layout.fillHeight: true; renderer: hub.renderer; markdown: hub.markdown; documentBaseUrl: hub.documentBaseUrl; darkMode: hub.darkMode; typeface: backend.outputFont; textSize: backend.outputPointSize; layoutMode: hub.previewLayoutMode; onLayoutRequested: function(mode) { hub.previewLayoutMode = mode }; onLinkRequested: function(link) {} }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Label { Layout.fillWidth: true; text: backend.publishingThemeName + (hub.selectedFormat === "pdf" ? " · PDF pages" : " · Web preview"); elide: Text.ElideRight; color: backend.palette.muted; font.pixelSize: 12 }
                    SecondaryAction { objectName: "exportPaginatedPreviewButton"; text: "PDF pages"; selected: hub.selectedFormat === "pdf"; onClicked: hub.selectedFormat = "pdf"; Accessible.name: "Show exported PDF pages"; ToolTip.text: "Shows exact PDF page breaks before saving"; ToolTip.visible: hovered }
                }
                }
            }
        }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: backend.palette.border }
        RowLayout {
            Layout.fillWidth: true; Layout.preferredHeight: 68
            Layout.leftMargin: 20; Layout.rightMargin: 8
            spacing: 8
            SecondaryAction { objectName: "exportShareMarkdownButton"; visible: hub.width >= 520; text: "Share Markdown…"; Layout.preferredWidth: 160; onClicked: backend.nativeWindowAction("share"); Accessible.name: "Share Markdown" }
            Item { Layout.fillWidth: true }
            SecondaryAction { objectName: "exportCancelButton"; text: "Cancel"; Layout.preferredWidth: 136; onClicked: hub.close(); Accessible.name: "Cancel export" }
            Button {
                id: saveAction
                objectName: "exportDestinationButton"
                text: selectedFormat === "pdf" ? "Save PDF…" : "Save HTML…"
                Layout.preferredWidth: 136; Layout.preferredHeight: 36
                implicitWidth: 136; implicitHeight: 36
                font.pixelSize: 13
                font.weight: Font.DemiBold
                leftPadding: 18; rightPadding: 18
                background: Rectangle {
                    radius: 7
                    color: saveAction.down ? "#204b7e" : saveAction.hovered ? "#3170b8" : "#285e9e"
                }
                contentItem: Text {
                    text: saveAction.text; font: saveAction.font; color: "white"
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: { hub.destinationRequested(hub.selectedFormat); hub.close(); }
                Accessible.name: text
            }
            Item {
                Layout.preferredWidth: 26; Layout.preferredHeight: 36
                Canvas {
                    anchors.right: parent.right; anchors.bottom: parent.bottom
                    width: 18; height: 18
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.strokeStyle = backend.palette.muted
                        ctx.lineWidth = 1.4
                        for (var i = 0; i < 3; ++i) {
                            ctx.beginPath()
                            ctx.moveTo(16 - i * 5, 4)
                            ctx.lineTo(4, 16 - i * 5)
                            ctx.stroke()
                        }
                    }
                }
                MouseArea {
                    id: resizeGrip
                    objectName: "exportResizeGrip"
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.SizeFDiagCursor
                    property real startWidth: 0
                    property real startHeight: 0
                    property point startPoint: Qt.point(0, 0)
                    onPressed: function(mouse) {
                        startWidth = hub.width
                        startHeight = hub.height
                        startPoint = mapToItem(hub.parent, mouse.x, mouse.y)
                    }
                    onPositionChanged: function(mouse) {
                        if (!pressed) return
                        var point = mapToItem(hub.parent, mouse.x, mouse.y)
                        var maxWidth = hub.parent.width - hub.x - 16
                        var maxHeight = hub.parent.height - hub.y - 16
                        hub.requestedWidth = Math.max(Math.min(380, maxWidth), Math.min(maxWidth, startWidth + point.x - startPoint.x))
                        hub.requestedHeight = Math.max(Math.min(460, maxHeight), Math.min(maxHeight, startHeight + point.y - startPoint.y))
                    }
                    Accessible.name: "Resize export dialog"
                }
            }
        }
    }
}
