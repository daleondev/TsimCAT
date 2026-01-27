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
        fillColor: "transparent"
        strokeColor: iconRoot.color
        strokeWidth: 2
        PathMove { x: 4; y: 20 }
        PathLine { x: 8; y: 12 }
        PathLine { x: 12; y: 16 }
        PathLine { x: 16; y: 6 }
        PathLine { x: 20; y: 10 }
    }
}
