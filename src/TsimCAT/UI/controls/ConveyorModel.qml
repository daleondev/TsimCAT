pragma ComponentBehavior: Bound
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
        position: Qt.vector3d(0, conveyorRoot.height - conveyorRoot.frameThickness / 2, 0)
        source: "#Cube"
        scale: Qt.vector3d(conveyorRoot.length / 100, conveyorRoot.frameThickness / 100, conveyorRoot.width / 100)
        materials: [
            PrincipledMaterial {
                baseColor: "#2c3e50"
                metalness: 0.5
            }
        ]
    }

    // Belt surface
    Model {
        position: Qt.vector3d(0, conveyorRoot.height - 10, 0) // Slightly above frame
        source: "#Cube"
        scale: Qt.vector3d(conveyorRoot.length / 100 - 0.1, 0.1, conveyorRoot.width / 100 - 0.2)
        materials: [
            PrincipledMaterial {
                baseColor: "#111111"
                roughness: 0.8
            }
        ]
    }

    // Legs
    Repeater3D {
        model: 4
        delegate: Model {
            required property int index
            // Positioned so the top of the leg is at the bottom of the frame
            position: Qt.vector3d((index % 2 === 0 ? -1 : 1) * (conveyorRoot.length / 2 - 100), (conveyorRoot.height - conveyorRoot.frameThickness) / 2, (index < 2 ? -1 : 1) * (conveyorRoot.width / 2 - 50))
            source: "#Cube"
            scale: Qt.vector3d(0.5, (conveyorRoot.height - conveyorRoot.frameThickness) / 100, 0.5)
            materials: [
                PrincipledMaterial {
                    baseColor: "#7f8c8d"
                    metalness: 0.8
                }
            ]
        }
    }
}
