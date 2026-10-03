import QtQuick
import QtQuick.Window

// Original line drawings, independent of platform symbol fonts.
Item {
    id: icon
    property string name: ""
    property color ink: "#40444b"
    implicitWidth: 18
    implicitHeight: 18
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
            c.scale(width / 20, height / 20)
            c.strokeStyle = icon.ink
            c.lineWidth = 1.5
            c.lineCap = "round"
            c.lineJoin = "round"
            function line(x, y, a, b) { c.moveTo(x,y); c.lineTo(a,b) }
            c.beginPath()
            switch (icon.name) {
            case "folder-filled":
                c.lineWidth = 0.7
                c.fillStyle = "#5cbce7"; c.strokeStyle = "#399aca"
                c.moveTo(2,5); c.quadraticCurveTo(2,3.5,3.5,3.5); c.lineTo(8,3.5); c.lineTo(10,5.5); c.lineTo(17,5.5); c.quadraticCurveTo(18,5.5,18,7); c.lineTo(18,16); c.lineTo(2,16); c.closePath(); c.fill(); c.stroke()
                c.beginPath()
                var blue = c.createLinearGradient(0,7,0,17)
                blue.addColorStop(0,"#8cdbf7"); blue.addColorStop(1,"#38a8d7")
                c.fillStyle = blue
                c.roundedRect(2,7,16,10,1.5,1.5); c.fill(); c.stroke(); return
            case "document":
                c.lineWidth = 0.7; c.strokeStyle = "#b5bac1"; c.fillStyle = "#ffffff"
                c.moveTo(4,2); c.lineTo(12,2); c.lineTo(16,6); c.lineTo(16,18); c.lineTo(4,18); c.closePath(); c.fill(); c.stroke()
                c.beginPath(); c.fillStyle = "#e5e8ec"; c.moveTo(12,2); c.lineTo(12,6); c.lineTo(16,6); c.closePath(); c.fill(); c.stroke()
                c.beginPath(); c.strokeStyle = "#bfc4cb"; for(let y=8;y<=15;y+=2) line(6,y,y===14?11:14,y); c.stroke(); return

            case "sort": line(6,3,6,17); line(3,6,6,3); line(6,3,9,6); line(14,3,14,17); line(11,14,14,17); line(14,17,17,14); break
            case "filter": line(3,5,17,5); line(5,10,15,10); line(7,15,13,15); break
            case "search": c.arc(8.5,8.5,5,0,Math.PI*2); line(12.5,12.5,17,17); break
            case "folder": line(4,3.5,15,3.5); c.moveTo(2.5,7); c.lineTo(17.5,7); c.lineTo(15.5,16.5); c.lineTo(4.5,16.5); c.closePath(); break
            case "library": c.rect(2.5,3.5,15,13); line(7,4,7,16); break
            case "workspace":
            case "organizer": c.rect(2.5,3.5,15,13); line(6,4,6,16); line(10,4,10,16); break
            case "outline": for (let y=5;y<=15;y+=5) { line(3,y,4,y); line(8,y,17,y) } break
            case "editor": c.rect(3.5,2.5,13,15); line(7,7,13,7); line(7,10,13,10); line(7,13,11,13); break
            case "split": c.rect(2.5,3.5,15,13); line(10,4,10,16); break
            case "preview": c.moveTo(7,3.5); c.lineTo(16,10); c.lineTo(7,16.5); c.closePath(); break
            case "plus": line(10,4,10,16); line(4,10,16,10); break
            case "close": line(6,6,14,14); line(14,6,6,14); break
            case "more": for(let x=4;x<=16;x+=6) { c.moveTo(x+0.65,10); c.arc(x,10,0.65,0,Math.PI*2) } break
            case "back":
            case "left": c.moveTo(12,6); c.lineTo(8,10); c.lineTo(12,14); break
            case "forward": c.moveTo(8,6); c.lineTo(12,10); c.lineTo(8,14); break
            case "up": c.moveTo(6,12); c.lineTo(10,8); c.lineTo(14,12); break
            case "check": c.moveTo(4,10); c.lineTo(8,14); c.lineTo(16,6); break
            case "link":
                c.moveTo(8,7); c.lineTo(10,5); c.bezierCurveTo(14,1,19,6,15,10); c.lineTo(13,12)
                c.moveTo(12,13); c.lineTo(10,15); c.bezierCurveTo(6,19,1,14,5,10); c.lineTo(7,8)
                line(7,13,13,7); break
            case "paragraph":
                c.moveTo(11,17); c.lineTo(11,3); c.lineTo(8,3); c.bezierCurveTo(2,3,2,10,8,10); c.lineTo(11,10)
                line(11,3,16,3); line(15,3,15,17); break
            case "export":
            case "share":
                c.moveTo(4,9); c.lineTo(4,17); c.lineTo(16,17); c.lineTo(16,9)
                line(10,12,10,2); c.moveTo(6,6); c.lineTo(10,2); c.lineTo(14,6); break
            case "tag":
                c.moveTo(3,3); c.lineTo(10,3); c.lineTo(17,10); c.lineTo(10,17); c.lineTo(3,10); c.closePath()
                c.moveTo(7.5,6.5); c.arc(6.5,6.5,1,0,Math.PI*2); break
            case "format": line(4,4,16,4); line(10,4,10,16); line(7,16,13,16); break
            case "appearance":
                c.arc(10,10,4,0,Math.PI*2)
                for(let a=0;a<8;a++) { const t=a*Math.PI/4; line(10+Math.cos(t)*6.5,10+Math.sin(t)*6.5,10+Math.cos(t)*8.5,10+Math.sin(t)*8.5) } break
            case "refresh":
                c.arc(10,10,6,Math.PI*0.2,Math.PI*1.8); c.moveTo(16,3); c.lineTo(16,7); c.lineTo(12,7); break
            case "down": c.moveTo(6,8); c.lineTo(10,12); c.lineTo(14,8); break
            case "right": c.moveTo(8,6); c.lineTo(12,10); c.lineTo(8,14); break
            }
            c.stroke()
        }
    }
}
