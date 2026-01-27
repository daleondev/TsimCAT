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
        PathMove { x: 12; y: 2 }
        PathLine { x: 12; y: 22 }
        PathMove { x: 2; y: 12 }
        PathLine { x: 22; y: 12 }
        PathMove { x: 12; y: 12 }
        PathArc { x: 12.1; y: 12; radiusX: 2; radiusY: 2 }
    }
}
