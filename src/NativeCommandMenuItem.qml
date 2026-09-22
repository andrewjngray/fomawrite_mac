import QtQuick
import Qt.labs.platform as Platform

Platform.MenuItem {
    required property var commands
    required property string commandId
    objectName: "native_" + commandId
    text: commands.label(commandId)
    enabled: commands.isEnabled(commandId)
    checkable: !!commands.entry(commandId).toggle
    checked: commands.isChecked(commandId)
    onTriggered: commands.run(commandId)
}
