import QtQuick
import QtQuick3D

Node {
    id: gantryRoot

    // Positions in mm
    property real yPos: 0
    property real zPos: 0

    readonly property color frameColor: "#2c3e50"
    readonly property color axisColor: "#f67828"

    // Two vertical pillars
    Model {
        position: Qt.vector3d(0, 1000, -1000)
        source: "#Cube"
        scale: Qt.vector3d(1, 20, 1)
        materials: [ PrincipledMaterial { baseColor: frameColor; metalness: 0.8 } ]
    }
    Model {
        position: Qt.vector3d(0, 1000, 1000)
        source: "#Cube"
        scale: Qt.vector3d(1, 20, 1)
        materials: [ PrincipledMaterial { baseColor: frameColor; metalness: 0.8 } ]
    }

    // Horizontal Rail (Y-axis)
    Model {
        position: Qt.vector3d(0, 2000, 0)
        source: "#Cube"
        scale: Qt.vector3d(0.8, 0.8, 20)
        materials: [ PrincipledMaterial { baseColor: frameColor; metalness: 0.9 } ]
    }

    // Y Carriage
    Node {
        id: carriage
        position: Qt.vector3d(0, 2000, gantryRoot.yPos)

        Model {
            source: "#Cube"
            scale: Qt.vector3d(1.2, 1.2, 1.2)
            materials: [ PrincipledMaterial { baseColor: axisColor } ]
        }

        // Z Axis
        Model {
            position: Qt.vector3d(0, -gantryRoot.zPos / 2, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.4, 10, 0.4)
            materials: [ PrincipledMaterial { baseColor: "#bdc3c7"; metalness: 1.0 } ]
        }

        // Simple Gripper placeholder
        Model {
            position: Qt.vector3d(0, -gantryRoot.zPos, 0)
            source: "#Cube"
            scale: Qt.vector3d(1, 0.2, 1)
            materials: [ PrincipledMaterial { baseColor: "#333" } ]
        }
    }
}