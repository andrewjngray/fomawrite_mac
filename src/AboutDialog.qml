import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    objectName: "aboutDialog"
    title: "About Fomawrite"
    modal: true
    anchors.centerIn: parent
    width: Math.min(540, parent.width - 40)
    standardButtons: Dialog.Close
    contentItem: ColumnLayout {
        spacing: 16
        Label {
            text: "Fomawrite " + Qt.application.version
            font.pixelSize: 20; font.bold: true
            Layout.fillWidth: true; wrapMode: Text.Wrap
        }
        Label {
            text: "A calm place to write plain Markdown."
            Layout.fillWidth: true; wrapMode: Text.Wrap
        }
        Label { text: "Running app"; font.bold: true }
        TextArea {
            objectName: "runningApplicationPath"
            text: backend.applicationPath
            readOnly: true; selectByMouse: true
            wrapMode: TextEdit.WrapAnywhere
            Layout.fillWidth: true
            Accessible.name: "Running application path"
        }
    }
}
