import QtQuick
import QtQuick3D

Node {
    id: conveyorRoot
    property real length: 2000
    property real width: 400
    property real height: 800

    readonly property real frameThickness: 50

    // Frame
    Model {
        position: Qt.vector3d(0, height - frameThickness / 2, 0)
        source: "#Cube"
        scale: Qt.vector3d(length / 100, frameThickness / 100, width / 100)
        materials: [
            PrincipledMaterial {
                baseColor: "#2c3e50"
                metalness: 0.5
            }
        ]
    }

    // Belt surface
    Model {
        position: Qt.vector3d(0, height - 10, 0) // Slightly above frame
        source: "#Cube"
        scale: Qt.vector3d(length / 100 - 0.1, 0.1, width / 100 - 0.2)
        materials: [
            PrincipledMaterial {
                baseColor: "#111"
                roughness: 0.8
            }
        ]
    }

    // Legs
    Repeater3D {
        model: 4
        delegate: Model {
            // Positioned so the top of the leg is at the bottom of the frame
            position: Qt.vector3d((index % 2 === 0 ? -1 : 1) * (length / 2 - 100), (height - frameThickness) / 2, (index < 2 ? -1 : 1) * (width / 2 - 50))
            source: "#Cube"
            scale: Qt.vector3d(0.5, (height - frameThickness) / 100, 0.5)
            materials: [
                PrincipledMaterial {
                    baseColor: "#7f8c8d"
                    metalness: 0.8
                }
            ]
        }
    }
}
