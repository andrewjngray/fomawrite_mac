import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    objectName: "previewPane"
    required property var renderer
    property string markdown: ""
    property url documentBaseUrl
    property bool darkMode: false
    property string typeface: "Helvetica Neue"
    property int textSize: 17
    signal linkRequested(url link)
    color: darkMode ? "#191b1e" : "#ffffff"
    property string renderedMarkdown: ""
    onMarkdownChanged: refreshTimer.restart()
    onTextSizeChanged: refreshTimer.restart()
    onTypefaceChanged: refreshTimer.restart()
    function refresh() {
        renderedMarkdown = markdown;
        Qt.callLater(function() { root.renderer.stylePreview(previewText.textDocument); });
    }
    Component.onCompleted: refresh()

    Timer { id: refreshTimer; interval: 120; onTriggered: root.refresh() }
    Flickable {
        id: previewScroll
        objectName: "previewScroll"
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: Math.max(height, previewText.implicitHeight + 100)
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}
        TextEdit {
            id: previewText
            objectName: "renderedPreview"
            x: Math.max(24, (previewScroll.width - 740) / 2)
            y: 24
            width: Math.max(100, Math.min(740, previewScroll.width - 48))
            height: implicitHeight
            readOnly: true
            selectByMouse: true
            textFormat: TextEdit.MarkdownText
            text: root.renderedMarkdown
            baseUrl: root.documentBaseUrl
            wrapMode: TextEdit.Wrap
            font.family: root.typeface
            font.pixelSize: root.textSize
            color: root.darkMode ? "#e6e8ec" : "#242831"
            selectionColor: "#c9d9f0"
            onLinkActivated: function(link) { root.linkRequested(link) }
            Accessible.name: "Rendered Markdown preview"
        }
        Label {
            anchors.centerIn: parent
            visible: root.markdown.length === 0
            text: "Your words, beautifully read.\nStart writing to see a live preview."
            horizontalAlignment: Text.AlignHCenter
            color: root.darkMode ? "#929aa6" : "#8b919b"
            font.pixelSize: 15
            lineHeight: 1.5
        }
    }
}
