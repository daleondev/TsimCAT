import QtQuick
import QtQuick3D

Node {
    id: conveyorRoot
    property real length: 2000
    property real width: 400
    property real height: 800

    // Frame
    Model {
        position: Qt.vector3d(0, height - 50, 0)
        source: "#Cube"
        scale: Qt.vector3d(length / 100, 1, width / 100)
        materials: [ PrincipledMaterial { baseColor: "#2c3e50"; metalness: 0.5 } ]
    }

    // Belt surface
    Model {
        position: Qt.vector3d(0, height - 40, 0)
        source: "#Cube"
        scale: Qt.vector3d(length / 100 - 0.1, 0.2, width / 100 - 0.2)
        materials: [ PrincipledMaterial { baseColor: "#111"; roughness: 0.8 } ]
    }

    // Legs
    Repeater3D {
        model: 4
        delegate: Model {
            position: Qt.vector3d(
                (index % 2 === 0 ? -1 : 1) * (length / 2 - 100),
                (height - 50) / 2,
                (index < 2 ? -1 : 1) * (width / 2 - 50)
            )
            source: "#Cube"
            scale: Qt.vector3d(0.5, (height - 50) / 50, 0.5)
            materials: [ PrincipledMaterial { baseColor: "#7f8c8d"; metalness: 0.8 } ]
        }
    }
}
