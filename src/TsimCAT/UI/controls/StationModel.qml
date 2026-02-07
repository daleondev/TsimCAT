import QtQuick
import QtQuick3D

Node {
    id: stationRoot
    property real width: 600
    property real depth: 600
    property real height: 800
    property color color: "#34495e"

    // Table Top
    Model {
        position: Qt.vector3d(0, stationRoot.height - 25, 0)
        source: "#Cube"
        scale: Qt.vector3d(stationRoot.width / 100, 0.5, stationRoot.depth / 100)
        materials: [ PrincipledMaterial { baseColor: stationRoot.color; metalness: 0.3 } ]
    }

    // Single central pillar for a cleaner "station" look
    Model {
        position: Qt.vector3d(0, (stationRoot.height - 50) / 2, 0)
        source: "#Cylinder"
        scale: Qt.vector3d(stationRoot.width / 200, (stationRoot.height - 50) / 100, stationRoot.depth / 200)
        materials: [ PrincipledMaterial { baseColor: "#7f8c8d"; metalness: 0.9 } ]
    }

    // Base plate
    Model {
        position: Qt.vector3d(0, 10, 0)
        source: "#Cube"
        scale: Qt.vector3d(stationRoot.width / 100 + 1, 0.2, stationRoot.depth / 100 + 1)
        materials: [ PrincipledMaterial { baseColor: "#2c3e50"; metalness: 0.5 } ]
    }
}
