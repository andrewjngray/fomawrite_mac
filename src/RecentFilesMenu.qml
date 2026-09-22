import QtQuick
import Qt.labs.platform as Platform

Platform.Menu {
    id: root
    required property var library
    signal openRequested(url file)
    Instantiator {
        model: root.library.recentFiles
        delegate: Platform.MenuItem {
            required property var modelData
            // Paths distinguish documents with the same basename.
            text: modelData.name + " — " + modelData.url.toString().replace(/^file:\/\//, "")
            onTriggered: root.openRequested(modelData.url)
        }
        onObjectAdded: function(index, object) { root.insertItem(index, object); }
        onObjectRemoved: function(index, object) { root.removeItem(object); }
    }
    Platform.MenuItem { text: "No Recent Files"; enabled: false; visible: root.library.recentFiles.length === 0 }
    Platform.MenuSeparator { visible: root.library.recentFiles.length > 0 }
    Platform.MenuItem { text: "Clear Recent Files"; enabled: root.library.recentFiles.length > 0; onTriggered: root.library.clearRecentFiles() }
}
