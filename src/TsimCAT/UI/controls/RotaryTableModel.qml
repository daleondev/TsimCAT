pragma ComponentBehavior: Bound
import QtQuick
import QtQuick3D

Node {
    id: rotaryRoot

    property real radius: 480
    property real height: 760
    property real angleDegrees: 0
    property bool partPresent: false
    property int partType: 0
    property bool busy: false

    readonly property real platterScale: rotaryRoot.radius / 50
    readonly property real innerPlatterScale: rotaryRoot.platterScale * 0.8
    readonly property real pedestalScale: rotaryRoot.radius / 95

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

    PrincipledMaterial {
        id: machineBasePaint
        baseColor: "#2a3138"
        metalness: 0.35
        roughness: 0.46
    }

    PrincipledMaterial {
        id: trimPaint
        baseColor: "#6e7b83"
        metalness: 0.42
        roughness: 0.32
    }

    Model {
        source: "#Cube"
        position: Qt.vector3d(0, 90, 0)
        scale: Qt.vector3d(rotaryRoot.platterScale * 1.34, 1.8, rotaryRoot.platterScale * 1.34)
        materials: [ machineBasePaint ]
    }

    Model {
        source: "#Cube"
        position: Qt.vector3d(0, 245, 0)
        scale: Qt.vector3d(rotaryRoot.platterScale * 1.06, 1.35, rotaryRoot.platterScale * 1.06)
        materials: [ trimPaint ]
    }

    Model {
        source: "#Cylinder"
        position: Qt.vector3d(0, 120, 0)
        scale: Qt.vector3d(rotaryRoot.pedestalScale, 1.2, rotaryRoot.pedestalScale)
        materials: [ darkSteel ]
    }

    Model {
        source: "#Cylinder"
        position: Qt.vector3d(0, rotaryRoot.height - 190, 0)
        scale: Qt.vector3d(rotaryRoot.pedestalScale * 0.76, 5.3, rotaryRoot.pedestalScale * 0.76)
        materials: [ brushedSteel ]
    }

    Node {
        id: platter
        position: Qt.vector3d(0, rotaryRoot.height, 0)
        eulerRotation.y: rotaryRoot.angleDegrees

        Model {
            source: "#Cylinder"
            scale: Qt.vector3d(rotaryRoot.platterScale, 0.42, rotaryRoot.platterScale)
            materials: [ accentPaint ]
        }

        Model {
            source: "#Cylinder"
            position: Qt.vector3d(0, 18, 0)
            scale: Qt.vector3d(rotaryRoot.innerPlatterScale, 0.18, rotaryRoot.innerPlatterScale)
            materials: [ brushedSteel ]
        }

        // Center safety shield
        Model {
            source: "#Cylinder"
            position: Qt.vector3d(0, 170, 0)
            scale: Qt.vector3d(1.2, 3.0, 1.2)
            materials: [ darkSteel ]
        }

        Model {
            source: "#Cylinder"
            position: Qt.vector3d(0, 320, 0)
            scale: Qt.vector3d(2.4, 0.3, 2.4)
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
                    scale: Qt.vector3d(1.25, 0.26, 2.0)
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
            color: "#a6adb3"
        }
    }

    Model {
        source: "#Cube"
        position: Qt.vector3d(-120, rotaryRoot.height + 54, 0)
        scale: Qt.vector3d(0.34, 0.3, rotaryRoot.platterScale * 0.95)
        materials: [ darkSteel ]
    }

    Model {
        source: "#Cube"
        position: Qt.vector3d(120, rotaryRoot.height + 54, 0)
        scale: Qt.vector3d(0.34, 0.3, rotaryRoot.platterScale * 0.95)
        materials: [ darkSteel ]
    }

    Model {
        source: "#Cube"
        position: Qt.vector3d(0, rotaryRoot.height + 54, -rotaryRoot.radius * 0.48)
        scale: Qt.vector3d(rotaryRoot.platterScale * 0.48, 0.24, 0.28)
        materials: [ darkSteel ]
    }
}