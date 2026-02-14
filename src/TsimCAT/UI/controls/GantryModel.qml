import QtQuick
import QtQuick3D

Node {
    id: gantryRoot

    readonly property color profileColor: "#d0d0d0"
    readonly property color railColor: "#cccccc"
    readonly property color motorColor: "#1a1a1a"

    property real xPos: 0
    property real zPos: 80
    property bool gripperGripped: false
    property bool carriedPartVisible: false
    property int carriedPartType: 0

    property real frameHeight: 800
    property real frameWidth: 200

    property real xAxisLength: 800
    property real xAxisWidth: 85

    property real zAxisLength: 400
    property real zAxisWidth: 70

    property real gripperFrameLength: 320
    property real gripperFrameWidth: 60

    component ItemProfile: Node {
        property real axisLength: 1000
        property real axisWidth: 80

        // Base Profile
        Model {
            source: "#Cube"
            scale: Qt.vector3d(axisLength / 100, axisWidth / 100, axisWidth / 100)
            materials: [
                PrincipledMaterial {
                    baseColor: gantryRoot.profileColor
                    metalness: 0.6
                    roughness: 0.3
                }
            ]
        }

        // Guide Rails
        Model {
            position: Qt.vector3d(0, axisWidth / 2, axisWidth / 4)
            source: "#Cube"
            scale: Qt.vector3d(axisLength / 100, axisWidth / 1000, axisWidth / 1000)
            materials: [
                PrincipledMaterial {
                    baseColor: gantryRoot.railColor
                    metalness: 0.8
                    roughness: 0.1
                }
            ]
        }
        Model {
            position: Qt.vector3d(0, axisWidth / 2, -axisWidth / 4)
            source: "#Cube"
            scale: Qt.vector3d(axisLength / 100, axisWidth / 1000, axisWidth / 1000)
            materials: [
                PrincipledMaterial {
                    baseColor: gantryRoot.railColor
                    metalness: 0.8
                    roughness: 0.1
                }
            ]
        }
        Model {
            position: Qt.vector3d(0, -axisWidth / 2, axisWidth / 4)
            source: "#Cube"
            scale: Qt.vector3d(axisLength / 100, axisWidth / 1000, axisWidth / 1000)
            materials: [
                PrincipledMaterial {
                    baseColor: gantryRoot.railColor
                    metalness: 0.8
                    roughness: 0.1
                }
            ]
        }
        Model {
            position: Qt.vector3d(0, -axisWidth / 2, -axisWidth / 4)
            source: "#Cube"
            scale: Qt.vector3d(axisLength / 100, axisWidth / 1000, axisWidth / 1000)
            materials: [
                PrincipledMaterial {
                    baseColor: gantryRoot.railColor
                    metalness: 0.8
                    roughness: 0.1
                }
            ]
        }
        Model {
            position: Qt.vector3d(0, axisWidth / 4, -axisWidth / 2)
            source: "#Cube"
            scale: Qt.vector3d(axisLength / 100, axisWidth / 1000, axisWidth / 1000)
            materials: [
                PrincipledMaterial {
                    baseColor: gantryRoot.railColor
                    metalness: 0.8
                    roughness: 0.1
                }
            ]
        }
        Model {
            position: Qt.vector3d(0, -axisWidth / 4, -axisWidth / 2)
            source: "#Cube"
            scale: Qt.vector3d(axisLength / 100, axisWidth / 1000, axisWidth / 1000)
            materials: [
                PrincipledMaterial {
                    baseColor: gantryRoot.railColor
                    metalness: 0.8
                    roughness: 0.1
                }
            ]
        }
        Model {
            position: Qt.vector3d(0, axisWidth / 4, axisWidth / 2)
            source: "#Cube"
            scale: Qt.vector3d(axisLength / 100, axisWidth / 1000, axisWidth / 1000)
            materials: [
                PrincipledMaterial {
                    baseColor: gantryRoot.railColor
                    metalness: 0.8
                    roughness: 0.1
                }
            ]
        }
        Model {
            position: Qt.vector3d(0, -axisWidth / 4, axisWidth / 2)
            source: "#Cube"
            scale: Qt.vector3d(axisLength / 100, axisWidth / 1000, axisWidth / 1000)
            materials: [
                PrincipledMaterial {
                    baseColor: gantryRoot.railColor
                    metalness: 0.8
                    roughness: 0.1
                }
            ]
        }
    }

    component Motor: Node {
        property real motorLength: 200
        property real motorWidth: 50

        Model {
            source: "#Cube"
            scale: Qt.vector3d(motorLength / 100, motorWidth / 100, motorWidth / 100)
            materials: [
                PrincipledMaterial {
                    baseColor: motorColor
                    roughness: 0.5
                }
            ]
        }
        Model {
            position: Qt.vector3d(motorLength / 100, -100, 0)
            source: "#Cylinder"
            scale: Qt.vector3d(0.01, 0.005, 0.01)
            materials: [
                PrincipledMaterial {
                    baseColor: "#444"
                }
            ]
        }
    }

    ItemProfile {
        axisLength: frameHeight
        axisWidth: frameWidth
        eulerRotation.y: 90
        position: Qt.vector3d(0, 0, gantryRoot.frameHeight / 2)
        scale: Qt.vector3d(1, 0.5, 1)
    }

    Node {
        id: xAxis
        position: Qt.vector3d(0, -xAxisWidth / 2 - frameWidth / 4, frameHeight - xAxisWidth / 2)

        ItemProfile {
            axisLength: xAxisLength
            axisWidth: xAxisWidth
        }

        Motor {
            motorLength: 160
            motorWidth: xAxisWidth - 20
            position: Qt.vector3d(xAxisLength / 2 + motorLength / 2, 0, 0)
        }

        Node {
            id: zAxis
            eulerRotation.y: -90
            position: Qt.vector3d(xPos, -zAxisWidth / 2 - xAxisWidth / 2, 0)

            ItemProfile {
                axisLength: zAxisLength
                axisWidth: zAxisWidth
            }

            Motor {
                motorLength: 120
                motorWidth: zAxisWidth - 20
                position: Qt.vector3d(zAxisLength / 2 + motorLength / 2, 0, 0)
            }

            Node {
                position: Qt.vector3d(zPos - zAxisLength / 2, -zAxisWidth / 2 - gripperFrameWidth / 2, 0)

                ItemProfile {
                    axisLength: gripperFrameLength
                    axisWidth: gripperFrameWidth
                }

                GripperModel {
                    id: gantryGripper
                    x: -gripperFrameLength / 2
                    eulerRotation.y: -90
                    eulerRotation.z: 90
                    gripped: gantryRoot.gripperGripped

                    PartModel {
                        visible: gantryRoot.carriedPartVisible
                        position: Qt.vector3d(0, 120, 85)
                        width: 140
                        length: 140
                        height: 80
                        color: gantryRoot.carriedPartType === 2 ? "#3498db" : "#e67e22"
                    }
                }
            }
        }
    }
}
