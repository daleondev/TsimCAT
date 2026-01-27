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
        PathLine { x: 20; y: 20 }
        PathMove { x: 4; y: 20 }
        PathLine { x: 4; y: 4 }
        PathMove { x: 4; y: 20 }
        PathLine { x: 12; y: 12 }
    }
}
