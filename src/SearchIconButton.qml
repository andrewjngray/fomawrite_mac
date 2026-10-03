import QtQuick
import QtQuick.Controls

ToolbarButton {
    hint: iconName === "close" ? "Close Find in document"
        : iconName === "up" ? "Previous match in document" : "Next match in document"
    iconColor: backend.palette.muted
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()
}
