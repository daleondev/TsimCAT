pragma ComponentBehavior: Bound
import QtQuick
import QtQuick3D

Node {
    id: rotaryRoot

    property real radius: 420
    property real height: 760
    property real angleDegrees: 0
    property bool partPresent: false
    property int partType: 0
    property bool busy: false

    PrincipledMaterial {
        id: darkSteel
        baseColor: "#44515a"
        metalness: 0.82
        roughness: 0.24
    }

    PrincipledMaterial {
        id: brushedSteel
        baseColor: "#95a4ad"
        metalness: 0.9
        roughness: 0.16
    }

    PrincipledMaterial {
        id: accentPaint
        baseColor: rotaryRoot.busy ? "#cf6b38" : "#3f6d7a"
        metalness: 0.28
        roughness: 0.36
    }

    Model {
        source: "#Cylinder"
        position: Qt.vector3d(0, 120, 0)
        scale: Qt.vector3d(4.4, 1.2, 4.4)
        materials: [ darkSteel ]
    }

    Model {
        source: "#Cylinder"
        position: Qt.vector3d(0, rotaryRoot.height - 190, 0)
        scale: Qt.vector3d(3.3, 5.3, 3.3)
        materials: [ brushedSteel ]
    }

    Node {
        id: platter
        position: Qt.vector3d(0, rotaryRoot.height, 0)
        eulerRotation.y: rotaryRoot.angleDegrees

        Model {
            source: "#Cylinder"
            scale: Qt.vector3d(5.6, 0.42, 5.6)
            materials: [ accentPaint ]
        }

        Model {
            source: "#Cylinder"
            position: Qt.vector3d(0, 18, 0)
            scale: Qt.vector3d(4.45, 0.18, 4.45)
            materials: [ brushedSteel ]
        }

        Repeater3D {
            model: 4
            delegate: Node {
                required property int index
                eulerRotation.y: index * 90

                Model {
                    source: "#Cube"
                    position: Qt.vector3d(rotaryRoot.radius * 0.48, 22, 0)
                    scale: Qt.vector3d(1.1, 0.26, 1.7)
                    materials: [ darkSteel ]
                }
            }
        }

        PartModel {
            visible: rotaryRoot.partPresent
            position: Qt.vector3d(rotaryRoot.radius * 0.5, 58, 0)
            width: 140
            length: 140
            height: 80
            color: rotaryRoot.partType === 2 ? "#2ea970" : "#a6adb3"
        }
    }

    Model {
        source: "#Cube"
        position: Qt.vector3d(0, rotaryRoot.height + 72, rotaryRoot.radius + 80)
        scale: Qt.vector3d(7.6, 0.18, 0.18)
        materials: [ darkSteel ]
    }

    Model {
        source: "#Cube"
        position: Qt.vector3d(0, rotaryRoot.height + 72, -rotaryRoot.radius - 80)
        scale: Qt.vector3d(7.6, 0.18, 0.18)
        materials: [ darkSteel ]
    }
}