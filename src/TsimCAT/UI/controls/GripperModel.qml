import QtQuick
import QtQuick3D

Node {
    id: root
    property bool gripped: false
    property real gripperLength: 200
    property real gripperWidth: 100

    Model {
        source: "#Cube"
        scale: Qt.vector3d(root.gripperLength / 100, root.gripperWidth / 100, root.gripperWidth / 100)
        materials: [
            PrincipledMaterial {
                baseColor: '#6e6e6e'
                metalness: 0.6
                roughness: 0.4
            }
        ]
    }

    Node {
        id: leftFinger
        x: root.gripped ? 25 : 75
        z: root.gripperWidth

        Behavior on x {
            NumberAnimation {
                duration: 300
                easing.type: Easing.InOutQuad
            }
        }

        Model {
            position: Qt.vector3d(0, root.gripperLength / 2 / 100, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.1, 0.4, 0.8)
            materials: [
                PrincipledMaterial {
                    baseColor: "#b1b1b1"
                    metalness: 0.6
                }
            ]
        }
    }

    Node {
        id: rightFinger
        x: root.gripped ? -25 : -75
        z: root.gripperWidth

        Behavior on x {
            NumberAnimation {
                duration: 300
                easing.type: Easing.InOutQuad
            }
        }

        Model {
            position: Qt.vector3d(0, root.gripperLength / 2 / 100, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.1, 0.4, 0.8)
            materials: [
                PrincipledMaterial {
                    baseColor: '#b1b1b1'
                    metalness: 0.6
                }
            ]
        }
    }
}
