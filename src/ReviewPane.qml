import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// The Spelling and Grammar pane: every finding the checker has in the document,
// one row each, with the fixes at hand. The list is Backend::reviewIssues; the
// window decides how a jump or a fix reaches the surface in use.
Rectangle {
    id: root
    objectName: "reviewPane"
    property bool darkMode: false
    property var issues: backend.reviewIssues
    property bool truncated: backend.reviewIssuesTruncated
    property string filter: "all" // all | spelling | grammar | style
    property int selectedStart: -1
    readonly property var checker: backend.spellCheck
    readonly property int spellingCount: countOf("Spelling")
    readonly property int grammarCount: countOf("Grammar")
    readonly property int styleCount: countOf("Style")
    readonly property bool harperActive: checker.effectiveEngine === "harper"
    readonly property var shown: issues.filter(function(issue) {
        return filter === "all" || issue.category.toLowerCase() === filter
    })
    readonly property color spellingInk: darkMode ? "#ff8a80" : "#d93025"
    readonly property color grammarInk: darkMode ? "#8ab4f8" : "#1a5fd0"
    readonly property color styleInk: darkMode ? "#a0a6b0" : "#5f6670"
    signal jumpRequested(int start, int end)
    signal fixRequested(var issue, string replacement)
    signal closeRequested()

    color: backend.palette.panel
    clip: true

    function countOf(category) {
        var n = 0
        for (var i = 0; i < issues.length; ++i) if (issues[i].category === category) ++n
        return n
    }
    function escaped(text) {
        return String(text).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    }
    function inkFor(category) { return category === "Spelling" ? spellingInk : category === "Style" ? styleInk : grammarInk }
    // Harper's own name for the finding ("Agreement", "WordChoice") is the tag
    // for grammar and style rows when there is one; spelling stays "Spelling".
    function tagFor(issue) {
        if (issue.category === "Spelling") return "Spelling"
        var kind = issue.kind ? String(issue.kind) : ""
        return kind !== "" ? kind.replace(/([a-z])([A-Z])/g, "$1 $2") : issue.category
    }
    function engineStatus() {
        if (harperActive) return "Harper " + checker.harperVersion
        return checker.engine === "harper" ? "macOS checker (Harper is loading)" : "macOS checker"
    }
    function emphasised(issue) {
        var context = String(issue.context), from = issue.wordOffset, to = from + String(issue.word).length
        var ink = inkFor(issue.category)
        return escaped(context.substring(0, from)) + "<b><font color=\"" + ink + "\">"
            + escaped(context.substring(from, to)) + "</font></b>" + escaped(context.substring(to))
    }
    function summary() {
        if (!checker.available) return "Not available here"
        if (!checker.enabled) return "Spelling check is off"
        var parts = []
        if (spellingCount > 0) parts.push(spellingCount + " spelling")
        if (grammarCount > 0) parts.push(grammarCount + " grammar")
        if (styleCount > 0) parts.push(styleCount + " style")
        return parts.length ? parts.join(" · ") + (truncated ? " (first " + issues.length + ")" : "") : "No issues"
    }
    // Rows are replaced whenever the document is re-checked; keep the writer's place in the list.
    onShownChanged: {
        var y = list.contentY
        Qt.callLater(function() { list.contentY = y; list.returnToBounds() })
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 16
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.bottomMargin: 8
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 2
                Label {
                    text: "Review"
                    color: backend.palette.text
                    font.family: Qt.application.font.family
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                }
                Label {
                    objectName: "reviewCounts"
                    Layout.fillWidth: true
                    text: root.summary()
                    elide: Text.ElideRight
                    color: backend.palette.muted
                    font.family: Qt.application.font.family
                    font.pixelSize: 12
                }
                Label {
                    objectName: "reviewEngine"
                    Layout.fillWidth: true
                    text: root.engineStatus()
                    elide: Text.ElideRight
                    color: backend.palette.muted
                    font.family: Qt.application.font.family
                    font.pixelSize: 11
                    opacity: 0.8
                }
            }
            ChromeButton {
                id: languageButton
                objectName: "reviewLanguage"
                Layout.minimumWidth: 28
                Layout.maximumWidth: 120
                text: root.harperActive ? root.checker.dialect
                    : root.checker.language === "" ? "System language" : root.checker.language
                hint: root.harperActive ? "Dialect" : "Spelling language"
                darkMode: root.darkMode
                enabled: root.checker.available
                iconName: "down"
                alignLeft: true
                onClicked: (root.harperActive ? dialectMenu : languageMenu).popup(languageButton, 0, languageButton.height + 3)
            }
            ToolbarButton {
                objectName: "reviewClose"
                iconName: "close"
                hint: "Close Spelling and Grammar"
                implicitWidth: 34
                onClicked: root.closeRequested()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: [{ id: "all", text: "All", name: "reviewFilterAll" },
                        { id: "spelling", text: "Spelling", name: "reviewFilterSpelling" },
                        { id: "grammar", text: "Grammar", name: "reviewFilterGrammar" },
                        { id: "style", text: "Style", name: "reviewFilterStyle" }]
                ChromeButton {
                    required property var modelData
                    objectName: modelData.name
                    text: modelData.text
                    hint: "Show " + (modelData.id === "all" ? "all findings" : modelData.id + " findings only")
                    darkMode: root.darkMode
                    checkable: true
                    checked: root.filter === modelData.id
                    tonal: checked
                    onClicked: root.filter = modelData.id
                }
            }
        }

        ListView {
            id: list
            objectName: "reviewList"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            model: root.shown
            keyNavigationEnabled: true
            activeFocusOnTab: true
            Accessible.name: "Spelling and grammar findings"
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            Keys.onReturnPressed: function(event) {
                if (currentIndex >= 0 && currentIndex < root.shown.length) rowActivated(root.shown[currentIndex])
                event.accepted = true
            }
            Keys.onEscapePressed: function(event) { root.closeRequested(); event.accepted = true }
            function rowActivated(issue) {
                root.selectedStart = issue.start
                root.jumpRequested(issue.start, issue.end)
            }
            delegate: Rectangle {
                id: row
                required property var modelData
                required property int index
                readonly property bool spelling: modelData.category === "Spelling"
                readonly property color ink: root.inkFor(modelData.category)
                readonly property bool selected: root.selectedStart === modelData.start
                objectName: "reviewRow_" + index
                width: ListView.view.width - 10
                implicitHeight: body.implicitHeight + 20
                height: implicitHeight
                radius: 8
                color: selected ? backend.palette.selectedRow : rowHover.hovered ? backend.palette.hover : backend.palette.field
                border.width: ListView.isCurrentItem && list.activeFocus ? 2 : 1
                border.color: ListView.isCurrentItem && list.activeFocus ? backend.palette.focus : backend.palette.border
                Accessible.role: Accessible.ListItem
                Accessible.name: modelData.category + ": " + modelData.word + (modelData.message ? ". " + modelData.message : "")

                HoverHandler { id: rowHover }
                MouseArea {
                    objectName: "reviewRowArea"
                    anchors.fill: parent
                    onClicked: { list.currentIndex = row.index; list.rowActivated(row.modelData) }
                }
                ColumnLayout {
                    id: body
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 10
                    spacing: 6
                    Rectangle {
                        objectName: "reviewTag"
                        implicitWidth: tagLabel.implicitWidth + 12
                        implicitHeight: 16
                        radius: 4
                        color: Qt.rgba(tagLabel.color.r, tagLabel.color.g, tagLabel.color.b, 0.14)
                        Label {
                            id: tagLabel
                            anchors.centerIn: parent
                            text: root.tagFor(row.modelData)
                            color: row.ink
                            font.family: Qt.application.font.family
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 0.4
                        }
                    }
                    Text {
                        objectName: "reviewContext"
                        Layout.fillWidth: true
                        text: root.emphasised(row.modelData)
                        textFormat: Text.StyledText
                        wrapMode: Text.Wrap
                        maximumLineCount: 4
                        elide: Text.ElideRight
                        color: backend.palette.text
                        font.family: Qt.application.font.family
                        font.pixelSize: 13
                    }
                    Text {
                        objectName: "reviewMessage"
                        Layout.fillWidth: true
                        visible: !row.spelling && row.modelData.message !== ""
                        text: row.modelData.message
                        wrapMode: Text.Wrap
                        color: backend.palette.muted
                        font.family: Qt.application.font.family
                        font.pixelSize: 12
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 4
                        visible: row.modelData.suggestions.length > 0
                        Repeater {
                            model: row.modelData.suggestions.slice(0, 5)
                            ChromeButton {
                                required property string modelData
                                required property int index
                                objectName: "reviewSuggestion_" + index
                                text: modelData
                                hint: "Replace with " + modelData
                                darkMode: root.darkMode
                                tonal: true
                                width: Math.min(implicitWidth, row.width - 20)
                                onClicked: root.fixRequested(row.modelData, modelData)
                            }
                        }
                    }
                    RowLayout {
                        spacing: 4
                        ChromeButton {
                            objectName: "reviewIgnore"
                            text: "Ignore"
                            hint: row.spelling ? "Ignore this word until the app quits" : "Ignore this finding until the app quits"
                            darkMode: root.darkMode
                            font.pixelSize: 12
                            implicitHeight: 24
                            onClicked: row.spelling ? root.checker.ignoreWord(row.modelData.word)
                                                    : root.checker.ignoreGrammar(row.modelData.word, row.modelData.message)
                        }
                        ChromeButton {
                            objectName: "reviewLearn"
                            visible: row.spelling
                            text: "Learn"
                            hint: "Add to the system dictionary"
                            darkMode: root.darkMode
                            font.pixelSize: 12
                            implicitHeight: 24
                            onClicked: root.checker.learnWord(row.modelData.word)
                        }
                    }
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.max(0, parent.width - 24)
                visible: root.shown.length === 0
                spacing: 10
                Label {
                    objectName: "reviewEmpty"
                    Layout.fillWidth: true
                    text: !root.checker.available ? "Spelling check is not available on this platform"
                        : !root.checker.enabled ? "Spelling check is off"
                        : root.filter === "grammar" && !root.checker.grammarEnabled ? "Grammar check is off"
                        : root.filter === "style" && !root.harperActive ? "Style suggestions come from Harper"
                        : root.issues.length > 0 ? "No " + root.filter + " issues"
                        : root.harperActive ? "No spelling, grammar or style issues" : "No spelling or grammar issues"
                    color: backend.palette.muted
                    font.family: Qt.application.font.family
                    font.pixelSize: 13
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                }
                ChromeButton {
                    objectName: "reviewTurnOn"
                    Layout.alignment: Qt.AlignHCenter
                    visible: root.checker.available && (!root.checker.enabled || (root.filter === "grammar" && !root.checker.grammarEnabled))
                    text: root.checker.enabled ? "Turn On Grammar Check" : "Turn On Spelling Check"
                    tonal: true
                    darkMode: root.darkMode
                    onClicked: { if (!root.checker.enabled) root.checker.enabled = true; else root.checker.grammarEnabled = true }
                }
            }
        }
    }

    CompactMenu {
        id: languageMenu
        objectName: "reviewLanguageMenu"
        darkMode: root.darkMode
        width: 200
        height: Math.min(implicitHeight, 360)
        CompactMenuItem {
            objectName: "reviewLanguage_system"
            text: "System language"
            checkable: true
            checked: root.checker.language === ""
            onTriggered: root.checker.language = ""
        }
        Repeater {
            model: root.checker.languages()
            CompactMenuItem {
                required property string modelData
                objectName: "reviewLanguage_" + modelData
                text: modelData
                checkable: true
                checked: root.checker.language === modelData
                onTriggered: root.checker.language = modelData
            }
        }
    }

    // Harper's dialects, where the macOS checker has languages.
    CompactMenu {
        id: dialectMenu
        objectName: "reviewDialectMenu"
        darkMode: root.darkMode
        width: 200
        Repeater {
            model: root.checker.dialects
            CompactMenuItem {
                required property string modelData
                objectName: "reviewDialect_" + modelData
                text: modelData
                checkable: true
                checked: root.checker.dialect === modelData
                onTriggered: root.checker.dialect = modelData
            }
        }
    }
}
