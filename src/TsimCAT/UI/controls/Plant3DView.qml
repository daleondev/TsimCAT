import QtQuick
import QtQuick.Controls
import QtQuick3D

Item {
    id: root
    property var backend: null
    property bool exitDamperOpen: false

    View3D {
        id: view
        anchors.fill: parent
        camera: sceneCamera

        environment: SceneEnvironment {
            clearColor: "#dbe3e0"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        Node {
            id: cameraPivot
            eulerRotation: Qt.vector3d(-26, 34, 0)
            position: Qt.vector3d(520, 420, 120)

            PerspectiveCamera {
                id: sceneCamera
                position: Qt.vector3d(0, 0, 6100)
                clipNear: 10
                clipFar: 30000
            }
        }

        DirectionalLight {
            eulerRotation.x: -38
            eulerRotation.y: -18
            brightness: 1.55
            castsShadow: true
            shadowMapFar: 12000
            shadowMapQuality: DirectionalLight.ShadowMapQualityVeryHigh
        }

        PointLight {
            position: Qt.vector3d(1200, 2400, 1400)
            brightness: 0.9
            color: "#f2f1df"
        }

        PointLight {
            position: Qt.vector3d(-2200, 1800, -800)
            brightness: 0.45
            color: "#bfd0df"
        }

        Model {
            y: -1
            source: "#Rectangle"
            scale: Qt.vector3d(180, 180, 1)
            eulerRotation.x: -90
            materials: [
                PrincipledMaterial {
                    baseColor: "#c6ccc6"
                    roughness: 0.98
                }
            ]
        }

        Model {
            y: 1
            source: "#Rectangle"
            position: Qt.vector3d(80, 0, 120)
            scale: Qt.vector3d(36, 34, 1)
            eulerRotation.x: -90
            materials: [
                PrincipledMaterial {
                    baseColor: "#b7b49f"
                    roughness: 0.95
                }
            ]
        }

        FenceModel {
            id: fence
            position: Qt.vector3d(150, 0, 120)
            exitDamperOpen: root.exitDamperOpen
        }

        Model {
            position: Qt.vector3d(-1420, 4, 120)
            source: "#Rectangle"
            scale: Qt.vector3d(9, 12, 1)
            eulerRotation.x: -90
            materials: [
                PrincipledMaterial {
                    baseColor: "#83918d"
                    roughness: 0.9
                }
            ]
        }

        Node {
            id: rotaryStation
            position: Qt.vector3d(-1350, 0, 120)

            StationFrameModel {
                width: 760
                depth: 1200
                position: Qt.vector3d(120, 0, 0)
            }

            RotaryTableModel {
                angleDegrees: root.backend ? root.backend.rotaryTable.angleDegrees : 0
                partPresent: root.backend ? root.backend.rotaryTable.partPresent : false
                partType: root.backend ? root.backend.rotaryTable.partType : 0
                busy: root.backend ? root.backend.rotaryTable.busy : false
            }
        }

        RobotModel {
            id: plantRobot
            position: Qt.vector3d(0, 0, 350)
            eulerRotation.z: 90

            axis1: root.backend ? root.backend.robot.axis1 : 0
            axis2: root.backend ? root.backend.robot.axis2 : -90
            axis3: root.backend ? root.backend.robot.axis3 : 90
            axis4: root.backend ? root.backend.robot.axis4 : 0
            axis5: root.backend ? root.backend.robot.axis5 : 0
            axis6: root.backend ? root.backend.robot.axis6 : 0
            gripperGripped: root.backend ? root.backend.robot.gripperGripped : false
            carriedPartVisible: root.backend ? root.backend.robotCarriedPartVisible : false
            carriedPartType: root.backend ? root.backend.robotCarriedPartType : 0
        }

        Node {
            id: laserStation
            position: Qt.vector3d(0, 0, -900)

            StationModel {
                color: "#b45e43"
            }

            PartModel {
                visible: root.backend ? root.backend.laserPartVisible : false
                position: Qt.vector3d(0, 815, 0)
                width: 140
                length: 140
                height: 80
                color: (root.backend && root.backend.laserPartType === 2) ? "#2ecc71" : "#8f8f8f"
            }

            Node {
                position: Qt.vector3d(560, 0, 0)

                StationFrameModel {}

                LaserModel {
                    position: Qt.vector3d(0, 1950, 0)
                    eulerRotation.z: -30
                    laserOn: true
                }
            }
        }

        ConveyorModel {
            id: exitConveyor
            position: Qt.vector3d(1750, 0, 120)
            length: 1250
            height: 720
            width: 430
            conveyorController: root.backend ? root.backend.exitConveyor : null
            sensorPositions: [250.0, 700.0, 1150.0]
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton

        property real lastX: 0
        property real lastY: 0

        onPressed: mouse => {
            lastX = mouse.x;
            lastY = mouse.y;
        }

        onPositionChanged: mouse => {
            let dx = mouse.x - lastX;
            let dy = mouse.y - lastY;
            if (mouse.buttons & Qt.RightButton) {
                cameraPivot.eulerRotation.y -= dx * 0.2;
                cameraPivot.eulerRotation.x = Math.max(-90, Math.min(0, cameraPivot.eulerRotation.x - dy * 0.2));
            } else if (mouse.buttons & (Qt.LeftButton | Qt.MiddleButton)) {
                let speed = sceneCamera.position.z / 2000.0;
                let right = sceneCamera.mapDirectionToScene(Qt.vector3d(1, 0, 0));
                let up = sceneCamera.mapDirectionToScene(Qt.vector3d(0, 1, 0));
                let move = right.times(-dx * speed).plus(up.times(dy * speed));
                cameraPivot.position = cameraPivot.position.plus(move);
            }
            lastX = mouse.x;
            lastY = mouse.y;
        }

        onWheel: wheel => {
            let zoomSpeed = sceneCamera.position.z * 0.1;
            sceneCamera.position.z = Math.max(500, Math.min(15000, sceneCamera.position.z - (wheel.angleDelta.y / 120.0) * zoomSpeed));
        }
    }
}
