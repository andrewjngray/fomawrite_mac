import QtQuick
import QtQuick.Controls
import Fomawrite 1.0

// The PDF in the preview pane's slot (Cycle 160: pages only; the toolbar,
// password prompt and the rest of the pane arrive in Cycle 161).
Rectangle {
    id: root
    objectName: "pdfReader"
    property var session: null
    property int bottomInset: 0
    property bool darkMode: false
    readonly property int pageGap: 12
    readonly property int sideMargin: 16
    readonly property string pdfState: session ? session.state : "none"
    color: darkMode ? "#2a2c30" : "#e4e4e2"

    function showRequestedPage() {
        if (!session || pages.count === 0) return;
        pages.positionViewAtIndex(Math.max(0, Math.min(session.requestedPage, pages.count - 1)), ListView.Beginning);
    }

    ListView {
        id: pages
        objectName: "pdfPageList"
        anchors.fill: parent
        anchors.bottomMargin: root.bottomInset
        clip: true
        visible: root.pdfState === "ready"
        model: root.session && root.pdfState === "ready" ? root.session.pageCount : 0
        spacing: root.pageGap
        topMargin: root.pageGap
        bottomMargin: root.pageGap
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}
        onCountChanged: if (count > 0) Qt.callLater(root.showRequestedPage)
        delegate: Item {
            id: pageSlot
            required property int index
            // Re-read after a reload: the revision changes when the file does.
            readonly property size points: root.session && root.session.revision >= 0 ? root.session.pageSize(index) : Qt.size(612, 792)
            width: pages.width
            height: page.height
            PdfPage {
                id: page
                objectName: "pdfPage"
                session: root.session
                page: pageSlot.index
                x: root.sideMargin
                width: Math.max(1, pages.width - 2 * root.sideMargin)
                height: Math.max(1, width * pageSlot.points.height / Math.max(1, pageSlot.points.width))
            }
        }
    }

    Connections {
        target: root.session
        function onRequestedPageChanged() { root.showRequestedPage(); }
    }

    Label {
        objectName: "pdfReaderMessage"
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 420)
        visible: root.pdfState === "password" || root.pdfState === "error"
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        textFormat: Text.PlainText
        color: root.darkMode ? "#d8d8d8" : "#444444"
        text: !root.session ? ""
            : root.pdfState === "password" ? root.session.fileName + " is protected by a password."
            : root.session.errorText
    }
}
