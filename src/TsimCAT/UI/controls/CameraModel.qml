import QtQuick
import QtQuick3D

Node {
    id: cameraRoot

    // Housing
    Model {
        source: "#Cube"
        scale: Qt.vector3d(0.8, 0.8, 1.2)
        materials: [
            PrincipledMaterial {
                baseColor: "#7f8c8d"
                metalness: 0.8
            }
        ]
    }

    // Lens
    Model {
        position: Qt.vector3d(0, 0, 60)
        eulerRotation.x: 90
        source: "#Cylinder"
        scale: Qt.vector3d(0.5, 0.1, 0.5)
        materials: [
            PrincipledMaterial {
                baseColor: "#000"
                metalness: 1.0
                roughness: 0.0
            }
        ]
    }
}
