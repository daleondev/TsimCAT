import QtQuick
import QtQuick3D

Node {
    id: stationRoot
    property real width: 600
    property real depth: 600
    property real height: 800

    Model {
        position: Qt.vector3d(0, stationRoot.height - 25, 0)
        source: "#Cube"
        scale: Qt.vector3d(stationRoot.width / 100, 0.5, stationRoot.depth / 100)
        materials: [
            PrincipledMaterial {
                baseColor: "#3f6d7a"
                metalness: 0.28
                roughness: 0.36
            }
        ]
    }

    Model {
        position: Qt.vector3d(0, (stationRoot.height - 50) / 2, 0)
        source: "#Cylinder"
        scale: Qt.vector3d(stationRoot.width / 200, (stationRoot.height - 50) / 100, stationRoot.depth / 200)
        materials: [
            PrincipledMaterial {
                baseColor: '#8d7f7f'
                metalness: 0.9
            }
        ]
    }
}