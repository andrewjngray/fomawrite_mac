import QtQuick
import QtQuick.Window

// One 24-unit drawing grid and stroke weight for the workspace icon family.
// Geometry is drawn locally so the appearance does not depend on symbol fonts.
Item {
    id: icon
    property string name: ""
    property color ink: "#34363a"
    implicitWidth: 16
    implicitHeight: 16
    readonly property real devicePixelRatio: Math.max(1, Screen.devicePixelRatio)
    onNameChanged: { if (drawing) drawing.requestPaint() }
    onInkChanged: { if (drawing) drawing.requestPaint() }
    Canvas {
        id: drawing
        anchors.centerIn: parent
        width: icon.width * icon.devicePixelRatio
        height: icon.height * icon.devicePixelRatio
        scale: 1 / icon.devicePixelRatio
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const c = getContext("2d")
            c.reset()
            c.scale(width / 24, height / 24)
            c.strokeStyle = icon.ink
            c.lineWidth = 1.7
            c.lineCap = "round"
            c.lineJoin = "round"
            function line(x, y, a, b) { c.moveTo(x, y); c.lineTo(a, b) }
            function panel() { c.roundedRect(3, 3, 18, 18, 2, 2) }
            c.beginPath()
            switch (icon.name) {
            case "folder-filled":
                c.fillStyle = "#5cbce7"
                c.strokeStyle = "#399aca"
                c.lineWidth = 1
                c.moveTo(3,4); c.lineTo(9,4); c.lineTo(12,7); c.lineTo(20,7)
                c.quadraticCurveTo(22,7,22,9); c.lineTo(22,18); c.quadraticCurveTo(22,20,20,20)
                c.lineTo(4,20); c.quadraticCurveTo(2,20,2,18); c.lineTo(2,6); c.quadraticCurveTo(2,4,3,4)
                c.closePath(); c.fill(); break
            case "folder":
                c.moveTo(4,4); c.lineTo(9,4); c.lineTo(12,7); c.lineTo(20,7)
                c.quadraticCurveTo(22,7,22,9); c.lineTo(22,18); c.quadraticCurveTo(22,20,20,20)
                c.lineTo(4,20); c.quadraticCurveTo(2,20,2,18); c.lineTo(2,6); c.quadraticCurveTo(2,4,4,4)
                c.closePath(); break
            case "document":
            case "editor":
                c.moveTo(14,2); c.lineTo(6,2); c.quadraticCurveTo(4,2,4,4); c.lineTo(4,20)
                c.quadraticCurveTo(4,22,6,22); c.lineTo(18,22); c.quadraticCurveTo(20,22,20,20)
                c.lineTo(20,8); c.lineTo(14,2); c.lineTo(14,8); c.lineTo(20,8)
                line(8,12,16,12); line(8,16,16,16); break
            case "compose":
                c.moveTo(12,3); c.lineTo(5,3); c.quadraticCurveTo(3,3,3,5); c.lineTo(3,19)
                c.quadraticCurveTo(3,21,5,21); c.lineTo(19,21); c.quadraticCurveTo(21,21,21,19); c.lineTo(21,12)
                c.moveTo(18,2); c.lineTo(22,6); c.lineTo(12,16); c.lineTo(7,17); c.lineTo(8,12); c.closePath()
                line(15,5,19,9); break
            case "sort": line(7,3,7,21); line(3,7,7,3); line(7,3,11,7); line(17,3,17,21); line(13,17,17,21); line(17,21,21,17); break
            case "filter": line(4,6,20,6); line(7,12,17,12); line(10,18,14,18); break
            case "search": c.arc(11,11,8,0,Math.PI*2); line(17,17,21,21); break
            case "clock": c.arc(12,12,9,0,Math.PI*2); c.moveTo(12,7); c.lineTo(12,12); c.lineTo(16,12); break
            case "library": panel(); line(9,3,9,21); break
            case "organizer": panel(); line(8,3,8,21); line(12,3,12,21); break
            case "panel-left-close":
            case "panel-left-open":
                panel(); line(9,3,9,21)
                if (icon.name === "panel-left-close") { c.moveTo(17,9); c.lineTo(14,12); c.lineTo(17,15) }
                else { c.moveTo(14,9); c.lineTo(17,12); c.lineTo(14,15) }
                break
            case "workspace": panel(); line(3,9,21,9); line(9,9,9,21); break
            case "outline": for (let y=6;y<=18;y+=6) { line(3,y,4,y); line(8,y,21,y) } break
            case "split": panel(); line(12,3,12,21); break
            case "preview": c.moveTo(8,4); c.lineTo(20,12); c.lineTo(8,20); c.closePath(); break
            case "minus": line(5,12,19,12); break
            case "plus": line(12,5,12,19); line(5,12,19,12); break
            case "close": line(6,6,18,18); line(18,6,6,18); break
            case "more":
                c.fillStyle = icon.ink
                for (let x=5;x<=19;x+=7) { c.moveTo(x+1,12); c.arc(x,12,1,0,Math.PI*2) }
                c.fill(); return
            case "back":
            case "left": c.moveTo(15,6); c.lineTo(9,12); c.lineTo(15,18); break
            case "forward":
            case "right": c.moveTo(9,6); c.lineTo(15,12); c.lineTo(9,18); break
            case "up": c.moveTo(6,15); c.lineTo(12,9); c.lineTo(18,15); break
            case "down": c.moveTo(6,9); c.lineTo(12,15); c.lineTo(18,9); break
            case "check": c.moveTo(4,12); c.lineTo(9,17); c.lineTo(20,6); break
            case "bold":
                c.moveTo(6,12); c.lineTo(14,12); c.bezierCurveTo(21,12,21,20,14,20); c.lineTo(6,20); c.closePath()
                c.moveTo(6,12); c.lineTo(6,4); c.lineTo(13,4); c.bezierCurveTo(20,4,20,12,13,12); break
            case "italic": line(10,4,20,4); line(4,20,14,20); line(15,4,9,20); break
            case "link":
                c.moveTo(10,13); c.bezierCurveTo(12,16,15,16,18,13); c.lineTo(21,10); c.bezierCurveTo(26,4,19,-1,15,3); c.lineTo(13,5)
                c.moveTo(14,11); c.bezierCurveTo(12,8,9,8,6,11); c.lineTo(3,14); c.bezierCurveTo(-2,20,5,25,9,21); c.lineTo(11,19); break
            case "paragraph":
                c.moveTo(13,20); c.lineTo(13,4); c.lineTo(9,4); c.bezierCurveTo(2,4,2,12,9,12); c.lineTo(13,12)
                line(13,4,20,4); line(18,4,18,20); break
            case "export":
            case "share":
                c.moveTo(5,10); c.lineTo(5,19); c.quadraticCurveTo(5,21,7,21); c.lineTo(17,21); c.quadraticCurveTo(19,21,19,19); c.lineTo(19,10)
                line(12,15,12,3); c.moveTo(7,8); c.lineTo(12,3); c.lineTo(17,8); break
            case "tag":
                c.moveTo(3,3); c.lineTo(12,3); c.lineTo(21,12); c.lineTo(12,21); c.lineTo(3,12); c.closePath()
                c.moveTo(8,7); c.arc(7,7,1,0,Math.PI*2); break
            case "format": line(4,4,20,4); line(12,4,12,20); line(8,20,16,20); break
            case "appearance":
                c.arc(12,12,4,0,Math.PI*2)
                for (let a=0;a<8;a++) { const t=a*Math.PI/4; line(12+Math.cos(t)*8,12+Math.sin(t)*8,12+Math.cos(t)*10,12+Math.sin(t)*10) }
                break
            case "refresh":
                c.arc(12,12,8,Math.PI*0.15,Math.PI*1.85); c.moveTo(20,3); c.lineTo(20,9); c.lineTo(14,9); break
            }
            c.stroke()
        }
    }
}
