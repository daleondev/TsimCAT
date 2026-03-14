import QtQuick
import QtQuick3D

Node {
    id: laserRoot

    property bool laserOn: false
    property color beamColor: "red"

    Model {
        source: "#Cube"
        scale: Qt.vector3d(0.8, 1.2, 0.8)
        materials: [
            PrincipledMaterial {
                baseColor: "#2c3e50"
                metalness: 0.9
                roughness: 0.1
            }
        ]
    }

    Model {
        position: Qt.vector3d(0, -60, 0)
        source: "#Cylinder"
        scale: Qt.vector3d(0.4, 0.2, 0.4)
        materials: [
            PrincipledMaterial {
                baseColor: "#111111"
                metalness: 1.0
            }
        ]
    }

    Model {
        visible: laserRoot.laserOn
        position: Qt.vector3d(0, -420, 0)
        source: "#Cylinder"
        scale: Qt.vector3d(0.03, 7.8, 0.03)
        materials: [
            DefaultMaterial {
                diffuseColor: laserRoot.beamColor
                emissiveFactor: Qt.vector3d(1, 0, 0)
                opacity: 0.7
            }
        ]
    }
}