import QtQuick
import QtQuick.Controls

// The Format Bar: Word's Home-ribbon formatting controls, Markdown-only. Every control
// writes plain Markdown through the window's commands (tryWrapSelection, editMarkdown,
// tryInsertLink), in Source and in Live. What Markdown cannot store (font and size, colours,
// alignment, spacing, shading, borders, underline) is deliberately absent.
//
// Narrow widths fold the groups from the right-hand side into a "»" menu; Style, Bold, Italic
// and the formatting-marks toggle stay longest.
Rectangle {
    id: bar
    objectName: "formatBar"
    required property var window
    required property var settings
    // The current block's style name (Body, Heading 1-6, Quote, Code Block).
    property string blockStyle: "Body"
    // Usable whenever an editing surface is on screen; a click with no owner claims it (see
    // Main.qml claimFormattingSurface) and then applies at the surface's own caret/selection.
    readonly property bool formattingAllowed: window.formatSurfaceAvailable
    function wrap(before, after) { window.claimFormattingSurface(); window.tryWrapSelection(before, after); }
    function md(action) { window.claimFormattingSurface(); window.editMarkdown(action); }
    signal revealChrome()
    function link() { window.claimFormattingSurface(); window.tryInsertLink(); }
    // F6 lands here (WorkspaceHeader.focusWorkspaceControl).
    function focusFirstControl() { styleButton.forceActiveFocus(Qt.TabFocusReason); }
    readonly property alias styleMenu: styleMenu
    readonly property alias overflowMenu: overflowMenu

    height: 40
    color: backend.palette.editor

    // Group widths (a group that follows another carries its 13 px divider).
    readonly property real sideMargin: 16
    // The Style button is as wide as its longest label ("Code Block") plus the chevron.
    TextMetrics { id: styleMetrics; font.family: Qt.application.font.family; font.pixelSize: 13; text: "Code Block" }
    TextMetrics { id: styleMetrics2; font.family: Qt.application.font.family; font.pixelSize: 13; text: "Heading 6" }
    readonly property real styleWidth: Math.max(118, Math.ceil(Math.max(styleMetrics.advanceWidth, styleMetrics2.advanceWidth)) + 38)
    readonly property var groupWidths: ({ style: styleWidth, bi: 75, inline2: 96, link: 43, lists: 107, indent: 75, clear: 43, marks: 43 })
    readonly property real overflowWidth: 38
    // The first to fold is the first listed.
    readonly property var foldOrder: ["indent", "lists", "clear", "link", "inline2"]
    readonly property var plan: computePlan(width)
    function computePlan(width) {
        var shown = { style: true, bi: true, inline2: true, link: true, lists: true, indent: true, clear: true, marks: true };
        var available = Math.max(0, width - 2 * sideMargin);
        function total() {
            var sum = 0, folded = false;
            for (var key in shown) { if (shown[key]) sum += groupWidths[key]; else folded = true; }
            return sum + (folded ? overflowWidth : 0);
        }
        for (var i = 0; i < foldOrder.length && total() > available; ++i) shown[foldOrder[i]] = false;
        shown.styleWidth = Math.max(76, Math.min(groupWidths.style, groupWidths.style - (total() - available)));
        return shown;
    }
    readonly property bool anyFolded: !plan.inline2 || !plan.link || !plan.lists || !plan.indent || !plan.clear

    // A quiet, square-cornered tool: the ribbon is flat, unlike the header's capsules.
    component FormatButton: Button {
        id: tool
        property string iconName: ""
        property string hint: text
        property bool active: false
        property bool sourceFormattingControl: true
        implicitWidth: 30
        implicitHeight: 30
        padding: 0
        // Material pads buttons by 24 px a side whatever `padding` says; the ribbon's tools size themselves.
        leftPadding: 0; rightPadding: 0; topPadding: 0; bottomPadding: 0
        topInset: 0; bottomInset: 0; leftInset: 0; rightInset: 0
        focusPolicy: Qt.StrongFocus
        font.family: Qt.application.font.family
        font.pixelSize: 15
        Accessible.name: hint
        Accessible.checked: active
        ToolTip.visible: hovered && hint !== ""
        ToolTip.delay: 700
        ToolTip.text: hint
        contentItem: Item {
            opacity: tool.enabled ? 1 : 0.4
            LineIcon {
                visible: tool.iconName !== ""
                name: tool.iconName
                ink: backend.palette.toolbarInk || backend.palette.text
                width: 18; height: 18
                strokeWidth: 1.8
                anchors.centerIn: parent
            }
            Text {
                visible: tool.iconName === ""
                anchors.centerIn: parent
                text: tool.text
                font: tool.font
                color: backend.palette.toolbarInk || backend.palette.text
            }
        }
        background: Rectangle {
            radius: 6
            color: tool.down ? (backend.palette.controlPressed || backend.palette.selectedRow)
                : tool.active ? (backend.palette.controlSelected || backend.palette.selectedRow)
                : tool.hovered && tool.enabled ? (backend.palette.controlHover || backend.palette.hover) : "transparent"
            border.width: tool.visualFocus ? 2 : 0
            border.color: backend.palette.focus
        }
    }
    component Divider: Item {
        implicitWidth: 13
        height: 30
        Rectangle { anchors.centerIn: parent; width: 1; height: 18; color: backend.palette.border }
    }

    Row {
        id: strip
        x: bar.sideMargin
        anchors.verticalCenter: parent.verticalCenter
        spacing: 0

        // Style
        FormatButton {
            id: styleButton
            objectName: "formatStyleButton"
            width: bar.plan.styleWidth
            text: bar.blockStyle
            hint: "Paragraph style"
            enabled: bar.formattingAllowed
            font.pixelSize: 13
            contentItem: Item {
                opacity: styleButton.enabled ? 1 : 0.4
                Text {
                    anchors.left: parent.left; anchors.leftMargin: 10
                    anchors.right: chevron.left; anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    text: styleButton.text
                    font: styleButton.font
                    elide: Text.ElideRight
                    color: backend.palette.toolbarInk || backend.palette.text
                }
                LineIcon {
                    id: chevron
                    name: "down"
                    ink: backend.palette.toolbarInk || backend.palette.text
                    width: 14; height: 14
                    anchors.right: parent.right; anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            background: Rectangle {
                radius: 6
                color: styleButton.down ? (backend.palette.controlPressed || backend.palette.selectedRow)
                    : styleButton.hovered && styleButton.enabled ? (backend.palette.controlHover || backend.palette.hover) : (backend.palette.control || backend.palette.field)
                border.width: styleButton.visualFocus ? 2 : 1
                border.color: styleButton.visualFocus ? backend.palette.focus : (backend.palette.toolbarBorder || backend.palette.border)
            }
            onClicked: { bar.window.claimFormattingSurface(); bar.window.openAnchoredMenu(styleMenu, styleButton); }
        }

        // Bold, Italic
        Row {
            visible: bar.plan.bi
            Divider {}
            Row {
                spacing: 2
                FormatButton { objectName: "formatBold"; text: "B"; font.pixelSize: 16; font.weight: Font.Bold; hint: "Bold"; enabled: bar.formattingAllowed; onClicked: bar.wrap("**", "**") }
                FormatButton { objectName: "formatItalic"; text: "I"; font.family: "Times New Roman"; font.pixelSize: 18; font.italic: true; hint: "Italic"; enabled: bar.formattingAllowed; onClicked: bar.wrap("*", "*") }
            }
        }
        // Strikethrough, inline code, highlight
        Row {
            visible: bar.plan.inline2
            spacing: 2
            Item { width: 0; height: 30 }
            FormatButton { objectName: "formatStrike"; text: "S"; font.pixelSize: 15; font.strikeout: true; hint: "Strikethrough"; enabled: bar.formattingAllowed; onClicked: bar.wrap("~~", "~~") }
            FormatButton { objectName: "formatCode"; iconName: "code"; hint: "Inline code"; enabled: bar.formattingAllowed; onClicked: bar.wrap("`", "`") }
            FormatButton { objectName: "formatHighlight"; iconName: "highlight"; hint: "Highlight"; enabled: bar.formattingAllowed; onClicked: bar.wrap("==", "==") }
        }
        // Link
        Row {
            visible: bar.plan.link
            Divider {}
            FormatButton { objectName: "formatLink"; iconName: "link"; hint: "Insert or edit link"; enabled: bar.formattingAllowed; onClicked: bar.link() }
        }
        // Lists
        Row {
            visible: bar.plan.lists
            Divider {}
            Row {
                spacing: 2
                FormatButton { objectName: "formatBullet"; iconName: "list-bullet"; hint: "Bulleted list"; enabled: bar.formattingAllowed; onClicked: bar.md("bullet") }
                FormatButton { objectName: "formatOrdered"; iconName: "list-ordered"; hint: "Numbered list"; enabled: bar.formattingAllowed; onClicked: bar.md("ordered") }
                FormatButton { objectName: "formatTask"; iconName: "list-task"; hint: "Task list"; enabled: bar.formattingAllowed; onClicked: bar.md("task") }
            }
        }
        // Indent
        Row {
            visible: bar.plan.indent
            Divider {}
            Row {
                spacing: 2
                FormatButton { objectName: "formatOutdent"; iconName: "outdent"; hint: "Decrease indent"; enabled: bar.formattingAllowed; onClicked: bar.md("outdent") }
                FormatButton { objectName: "formatIndent"; iconName: "indent"; hint: "Increase indent"; enabled: bar.formattingAllowed; onClicked: bar.md("indent") }
            }
        }
        // Clear formatting
        Row {
            visible: bar.plan.clear
            Divider {}
            FormatButton { objectName: "formatClear"; iconName: "clear-format"; hint: "Clear formatting"; enabled: bar.formattingAllowed; onClicked: bar.md("clearInline") }
        }
        // Show formatting marks
        Row {
            visible: bar.plan.marks
            Divider {}
            FormatButton {
                objectName: "formatMarks"
                iconName: "paragraph"
                hint: "Show formatting marks"
                active: bar.settings.showFormattingMarks
                Accessible.role: Accessible.CheckBox
                onClicked: bar.window.toggleFormattingMarks()
            }
        }
        // Everything that does not fit
        Row {
            visible: bar.anyFolded
            Item { width: 8; height: 30 }
            FormatButton {
                id: overflowButton
                objectName: "formatOverflowButton"
                iconName: "chevrons-right"
                hint: "More formatting"
                onClicked: { bar.window.claimFormattingSurface(); bar.window.openAnchoredMenu(overflowMenu, overflowButton); }
            }
        }
    }

    CompactMenu {
        id: styleMenu
        objectName: "formatStyleMenu"
        width: 170
        CompactMenuItem { objectName: "formatStyleBody"; text: "Body"; onTriggered: bar.md("body") }
        CompactMenuItem { text: "Heading 1"; onTriggered: bar.md("heading1") }
        CompactMenuItem { text: "Heading 2"; onTriggered: bar.md("heading2") }
        CompactMenuItem { text: "Heading 3"; onTriggered: bar.md("heading3") }
        CompactMenuItem { text: "Heading 4"; onTriggered: bar.md("heading4") }
        CompactMenuItem { text: "Heading 5"; onTriggered: bar.md("heading5") }
        CompactMenuItem { text: "Heading 6"; onTriggered: bar.md("heading6") }
        MenuSeparator {}
        CompactMenuItem { text: "Quote"; onTriggered: bar.md("quote") }
        CompactMenuItem { text: "Code Block"; onTriggered: bar.md("codeBlock") }
    }

    // Folded controls, in the order they fold.
    CompactMenu {
        id: overflowMenu
        objectName: "formatOverflowMenu"
        width: 200
        CompactMenuItem { visible: !bar.plan.inline2; height: visible ? implicitHeight : 0; enabled: bar.formattingAllowed; text: "Strikethrough"; onTriggered: bar.wrap("~~", "~~") }
        CompactMenuItem { visible: !bar.plan.inline2; height: visible ? implicitHeight : 0; enabled: bar.formattingAllowed; text: "Inline Code"; onTriggered: bar.wrap("`", "`") }
        CompactMenuItem { visible: !bar.plan.inline2; height: visible ? implicitHeight : 0; enabled: bar.formattingAllowed; text: "Highlight"; onTriggered: bar.wrap("==", "==") }
        CompactMenuItem { visible: !bar.plan.link; height: visible ? implicitHeight : 0; enabled: bar.formattingAllowed; text: "Link…"; onTriggered: bar.link() }
        CompactMenuItem { visible: !bar.plan.clear; height: visible ? implicitHeight : 0; enabled: bar.formattingAllowed; text: "Clear Formatting"; onTriggered: bar.md("clearInline") }
        CompactMenuItem { visible: !bar.plan.lists; height: visible ? implicitHeight : 0; enabled: bar.formattingAllowed; text: "Bulleted List"; onTriggered: bar.md("bullet") }
        CompactMenuItem { visible: !bar.plan.lists; height: visible ? implicitHeight : 0; enabled: bar.formattingAllowed; text: "Numbered List"; onTriggered: bar.md("ordered") }
        CompactMenuItem { visible: !bar.plan.lists; height: visible ? implicitHeight : 0; enabled: bar.formattingAllowed; text: "Task List"; onTriggered: bar.md("task") }
        CompactMenuItem { visible: !bar.plan.indent; height: visible ? implicitHeight : 0; enabled: bar.formattingAllowed; text: "Decrease Indent"; onTriggered: bar.md("outdent") }
        CompactMenuItem { visible: !bar.plan.indent; height: visible ? implicitHeight : 0; enabled: bar.formattingAllowed; text: "Increase Indent"; onTriggered: bar.md("indent") }
    }

    // Pointing at the bar keeps the header chrome revealed, as pointing at the header does.
    HoverHandler { onHoveredChanged: if (hovered && bar.settings.autoHideChrome) bar.revealChrome() }

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: backend.palette.border }
}
