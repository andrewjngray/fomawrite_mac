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
    // Qt's native (Cocoa) menu items can show a stale tick when the bound
    // value was already set before the item existed in its menu. Re-assert the
    // value when the owning menu is about to show, then restore the binding.
    function resync() {
        if (!checkable) return;
        var value = !!commands.isChecked(commandId);
        checked = !value;
        checked = Qt.binding(function() { return commands.isChecked(commandId); });
    }
}
