import QtQuick
import QtQuick3D

Node {
    id: frameRoot
    property real width: 800
    property real depth: 800
    property real height: 2000
    
    readonly property color profileColor: "#2c3e50"

    // Left Pillar
    Model {
        position: Qt.vector3d(-width/2 + 50, height/2, -depth/2 + 50)
        source: "#Cube"
        scale: Qt.vector3d(0.4, height/100, 0.4)
        materials: [ PrincipledMaterial { baseColor: profileColor; metalness: 0.8 } ]
    }

    // Right Pillar
    Model {
        position: Qt.vector3d(width/2 - 50, height/2, -depth/2 + 50)
        source: "#Cube"
        scale: Qt.vector3d(0.4, height/100, 0.4)
        materials: [ PrincipledMaterial { baseColor: profileColor; metalness: 0.8 } ]
    }

    // Top Crossbar
    Model {
        position: Qt.vector3d(0, height - 50, -depth/2 + 50)
        source: "#Cube"
        scale: Qt.vector3d(width/100, 0.4, 0.4)
        materials: [ PrincipledMaterial { baseColor: profileColor; metalness: 0.8 } ]
    }

    // Tool mounting arm (Forward reaching)
    Model {
        id: toolArm
        position: Qt.vector3d(0, height - 50, 0)
        source: "#Cube"
        scale: Qt.vector3d(0.3, 0.3, depth/100)
        materials: [ PrincipledMaterial { baseColor: "#7f8c8d"; metalness: 0.9 } ]
    }
}
