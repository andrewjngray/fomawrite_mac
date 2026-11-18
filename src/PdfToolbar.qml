import QtQuick
import QtQuick.Controls

// The PDF pane's toolbar, in Preview's order: thumbnails · zoom out, actual
// size, zoom in · fit width · page · Inspector · find. Narrow panes fold actual
// size, then fit width, then thumbnails, then the Inspector into a "more" menu.
Rectangle {
    id: bar
    objectName: "pdfToolbar"
    required property var reader
    readonly property bool controlsEnabled: reader.ready
    readonly property Item inspectorAnchor: plan.inspector ? inspectorButton : overflowButton
    function focusPageField() {
        if (!pageField.enabled) return;
        pageField.forceActiveFocus(Qt.ShortcutFocusReason);
        pageField.selectAll();
    }
    height: 44
    color: backend.palette.page

    readonly property real sideMargin: 12
    readonly property real spacing: 6
    readonly property var foldOrder: ["actual", "fit", "thumbnails", "inspector"]
    readonly property var foldWidths: ({ actual: 54, fit: 34 + spacing, thumbnails: 34 + spacing, inspector: 34 + spacing })
    readonly property real baseWidth: 2 * sideMargin + 66 + spacing + pageCluster.implicitWidth + 12 + 34
    readonly property var plan: {
        var shown = { actual: true, fit: true, thumbnails: true, inspector: true };
        var folded = false;
        function total() {
            var sum = bar.baseWidth;
            for (var key in shown) if (shown[key]) sum += bar.foldWidths[key];
            return sum + (folded ? 34 + bar.spacing : 0);
        }
        for (var i = 0; i < foldOrder.length && total() > width; ++i) { shown[foldOrder[i]] = false; folded = true; }
        shown.folded = folded;
        return shown;
    }

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: backend.palette.border }

    Row {
        id: leading
        x: bar.sideMargin
        anchors.verticalCenter: parent.verticalCenter
        spacing: bar.spacing
        ToolbarButton {
            objectName: "pdfThumbnailsToggle"
            visible: bar.plan.thumbnails
            iconName: "thumbnails"
            hint: bar.reader.thumbnailsShown ? "Hide thumbnails" : "Show thumbnails"
            checked: bar.reader.thumbnailsShown
            enabled: bar.controlsEnabled
            focusPolicy: Qt.TabFocus
            onClicked: bar.reader.toggleThumbnails()
        }
        ToolbarGroup {
            objectName: "pdfZoomControls"
            ToolbarButton {
                objectName: "pdfZoomOut"
                grouped: true
                focusPolicy: Qt.TabFocus
                iconName: "minus"
                hint: "Zoom out"
                enabled: bar.controlsEnabled && bar.reader.zoom > bar.reader.minimumZoom + 0.001
                onClicked: bar.reader.zoomOut()
            }
            ToolbarButton {
                objectName: "pdfActualSize"
                visible: bar.plan.actual
                grouped: true
                showDivider: true
                focusPolicy: Qt.TabFocus
                implicitWidth: 54
                text: bar.reader.zoomPercent + "%"
                hint: "Actual size — " + bar.reader.zoomPercent + "%"
                enabled: bar.controlsEnabled
                onClicked: bar.reader.actualSize()
            }
            ToolbarButton {
                objectName: "pdfZoomIn"
                grouped: true
                showDivider: true
                focusPolicy: Qt.TabFocus
                iconName: "plus"
                hint: "Zoom in"
                enabled: bar.controlsEnabled && bar.reader.zoom < bar.reader.maximumZoom - 0.001
                onClicked: bar.reader.zoomIn()
            }
        }
        ToolbarButton {
            objectName: "pdfFitWidth"
            visible: bar.plan.fit
            iconName: "fit-width"
            hint: "Zoom to fit width"
            checked: bar.reader.zoomMode === "fit"
            enabled: bar.controlsEnabled
            focusPolicy: Qt.TabFocus
            onClicked: bar.reader.fitWidth()
        }
        Row {
            id: pageCluster
            objectName: "pdfPageIndicator"
            spacing: 6
            anchors.verticalCenter: parent.verticalCenter
            TextField {
                id: pageField
                objectName: "pdfPageField"
                readonly property string shownText: bar.reader.currentLabel
                width: 46
                height: 30
                anchors.verticalCenter: parent.verticalCenter
                enabled: bar.controlsEnabled
                text: shownText
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                font.family: Qt.application.font.family
                font.pixelSize: 13
                color: backend.palette.text
                selectionColor: backend.palette.selection
                selectedTextColor: "white"
                leftPadding: 4; rightPadding: 4; topPadding: 0; bottomPadding: 0
                selectByMouse: true
                Accessible.name: "Page"
                Accessible.description: "Type a page number or label and press Return to go there."
                background: Rectangle {
                    radius: 6
                    color: backend.palette.field
                    border.width: pageField.activeFocus ? 2 : 1
                    border.color: pageField.activeFocus ? backend.palette.focus : backend.palette.border
                }
                onShownTextChanged: if (!activeFocus) text = shownText
                onActiveFocusChanged: { if (activeFocus) selectAll(); else text = shownText; }
                onAccepted: {
                    bar.reader.goToPageText(text);
                    text = shownText;
                    bar.reader.focusPages();
                }
                Keys.onEscapePressed: function(event) { text = shownText; bar.reader.focusPages(); event.accepted = true; }
            }
            Label {
                objectName: "pdfPageCount"
                anchors.verticalCenter: parent.verticalCenter
                readonly property string number: String(bar.reader.currentPage + 1)
                text: !bar.reader.ready ? "" : bar.reader.currentLabel !== number
                    ? "(" + number + " of " + bar.reader.pageCount + ")" : "of " + bar.reader.pageCount
                color: backend.palette.muted
                font.family: Qt.application.font.family
                font.pixelSize: 13
            }
        }
    }

    Row {
        id: trailing
        anchors.right: parent.right
        anchors.rightMargin: bar.sideMargin
        anchors.verticalCenter: parent.verticalCenter
        spacing: bar.spacing
        ToolbarButton {
            id: inspectorButton
            objectName: "pdfInspectorButton"
            visible: bar.plan.inspector
            iconName: "info"
            hint: "Show Inspector"
            enabled: bar.controlsEnabled
            focusPolicy: Qt.TabFocus
            onClicked: bar.reader.showInspector()
        }
        ToolbarButton {
            objectName: "pdfFindButton"
            iconName: "search"
            hint: "Find in PDF"
            checked: bar.reader.findOpen
            enabled: bar.controlsEnabled
            focusPolicy: Qt.TabFocus
            onClicked: bar.reader.findOpen ? bar.reader.closeFind() : bar.reader.openFind(false)
        }
        ToolbarButton {
            id: overflowButton
            objectName: "pdfToolbarOverflow"
            visible: bar.plan.folded
            iconName: "more"
            hint: "More PDF controls"
            enabled: bar.controlsEnabled
            focusPolicy: Qt.TabFocus
            onClicked: overflowMenu.popup(overflowButton, overflowButton.width - overflowMenu.width, overflowButton.height + 4)
        }
    }

    CompactMenu {
        id: overflowMenu
        objectName: "pdfToolbarOverflowMenu"
        CompactMenuItem { objectName: "pdfOverflowActualSize"; text: "Actual Size"; visible: !bar.plan.actual; height: visible ? implicitHeight : 0; onTriggered: bar.reader.actualSize() }
        CompactMenuItem { objectName: "pdfOverflowFitWidth"; text: "Zoom to Fit Width"; visible: !bar.plan.fit; height: visible ? implicitHeight : 0; onTriggered: bar.reader.fitWidth() }
        CompactMenuItem { objectName: "pdfOverflowThumbnails"; text: bar.reader.thumbnailsShown ? "Hide Thumbnails" : "Show Thumbnails"; visible: !bar.plan.thumbnails; height: visible ? implicitHeight : 0; onTriggered: bar.reader.toggleThumbnails() }
        CompactMenuItem { objectName: "pdfOverflowInspector"; text: "Show Inspector"; visible: !bar.plan.inspector; height: visible ? implicitHeight : 0; onTriggered: bar.reader.showInspector() }
    }
}
