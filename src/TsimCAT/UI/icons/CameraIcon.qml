import QtQuick
import QtQuick.Shapes

Shape {
    id: iconRoot
    property color color: "white"
    width: 24
    height: 24
    anchors.centerIn: parent
    antialiasing: true
    ShapePath {
        fillColor: iconRoot.color
        strokeColor: "transparent"
        PathMove { x: 4; y: 6 }
        PathLine { x: 16; y: 6 }
        PathLine { x: 16; y: 18 }
        PathLine { x: 4; y: 18 }
        PathLine { x: 4; y: 6 }
        PathMove { x: 18; y: 8 }
        PathLine { x: 22; y: 6 }
        PathLine { x: 22; y: 18 }
        PathLine { x: 18; y: 16 }
        PathLine { x: 18; y: 8 }
    }
}
