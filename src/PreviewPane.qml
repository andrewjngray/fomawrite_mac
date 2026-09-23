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
    property int layoutMode: 1
    signal scrollFractionChanged(real fraction)
    function scrollToFraction(fraction) { previewScroll.contentY = Math.max(0, previewScroll.contentHeight - previewScroll.height) * fraction; }
    function jumpToAnchor(anchor) {
        var position = renderer.previewAnchorPosition(previewText.textDocument, decodeURIComponent(anchor));
        if (position < 0) return false;
        previewScroll.contentY = Math.max(0, Math.min(previewScroll.contentHeight - previewScroll.height, previewText.positionToRectangle(position).y));
        return true;
    }
    signal layoutRequested(int mode)
    signal linkRequested(url link)
    color: backend.palette.page
    property string renderedMarkdown: ""
    onMarkdownChanged: refreshTimer.restart()
    onTextSizeChanged: refreshTimer.restart()
    onTypefaceChanged: refreshTimer.restart()
    function refresh() {
        renderedMarkdown = renderer.previewMarkdown(markdown);
        Qt.callLater(function() { root.renderer.stylePreview(previewText.textDocument); });
    }
    function reload() {
        // Force a fresh Markdown parse even when the source has not changed.
        refreshTimer.stop();
        renderedMarkdown = "";
        Qt.callLater(refresh);
    }
    Component.onCompleted: refresh()

    Timer { id: refreshTimer; interval: 120; onTriggered: root.refresh() }
    Flickable {
        id: previewScroll
        objectName: "previewScroll"
        anchors.fill: parent
        anchors.bottomMargin: 34
        clip: true
        contentWidth: width
        contentHeight: Math.max(height, previewText.implicitHeight + 100)
        boundsBehavior: Flickable.StopAtBounds
        onContentYChanged: root.scrollFractionChanged(contentY / Math.max(1, contentHeight - height))
        ScrollBar.vertical: ScrollBar {}
        TextEdit {
            id: previewText
            objectName: "renderedPreview"
            x: Math.max(22, (previewScroll.width - 740) / 2)
            y: 10
            width: Math.max(100, Math.min(740, previewScroll.width - 44))
            height: implicitHeight
            readOnly: true
            selectByMouse: true
            textFormat: TextEdit.MarkdownText
            text: root.renderedMarkdown
            baseUrl: root.documentBaseUrl
            wrapMode: TextEdit.Wrap
            font.family: root.typeface
            font.pixelSize: root.textSize
            color: backend.palette.text
            selectionColor: backend.palette.selection
            selectedTextColor: "white"
            onLinkActivated: function(link) { if (String(link).charAt(0) === "#") root.jumpToAnchor(String(link).slice(1)); else root.linkRequested(link); }
            Accessible.name: "Rendered Markdown preview"
        }
        Label {
            anchors.centerIn: parent
            visible: root.markdown.length === 0
            text: "Your words, beautifully read.\nStart writing to see a live preview."
            horizontalAlignment: Text.AlignHCenter
            color: backend.palette.muted
            font.pixelSize: 15
            lineHeight: 1.5
        }
    }
    Rectangle {
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 34; color: backend.palette.panel
        Rectangle { width: parent.width; height: 1; color: backend.palette.border }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12
            Label { text: "Markdown"; font.pixelSize: 11; color: backend.palette.muted }
            Item { Layout.fillWidth: true }
            ChromeButton { text: "Split"; hint: "Split layout"; darkMode: root.darkMode; checked: root.layoutMode === 1; onClicked: root.layoutRequested(1) }
            ChromeButton { text: "Full"; hint: "Preview layout"; darkMode: root.darkMode; checked: root.layoutMode === 2; onClicked: root.layoutRequested(2) }
        }
    }

}
