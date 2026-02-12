import QtQuick
import QtQuick.Controls
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root
    property var backend: null
    property bool damperOpen: false
    property bool doorOpen: false
    property real gantryX: 0
    property real gantryZ: 80

    View3D {
        id: view
        anchors.fill: parent
        camera: sceneCamera

        environment: SceneEnvironment {
            clearColor: "#f0f2f5"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
            lightProbe: Texture {
                textureData: ProceduralSkyTextureData {
                    sunCurve: 2.0
                }
            }
        }

        Node {
            id: cameraPivot
            eulerRotation: Qt.vector3d(-35, 45, 0)
            position: Qt.vector3d(500, 500, 0)

            PerspectiveCamera {
                id: sceneCamera
                position: Qt.vector3d(0, 0, 5500)
                clipNear: 10
                clipFar: 30000
            }
        }

        Node {
            id: sceneRoot

            DirectionalLight {
                eulerRotation.x: -30
                eulerRotation.y: -45
                brightness: 1.5
                castsShadow: true
                shadowMapFar: 10000
                shadowMapQuality: DirectionalLight.ShadowMapQualityVeryHigh
            }

            PointLight {
                position: Qt.vector3d(2000, 2000, 2000)
                brightness: 0.75
            }

            // Floor
            Model {
                y: -1
                source: "#Rectangle"
                scale: Qt.vector3d(150, 150, 1)
                eulerRotation.x: -90
                materials: [
                    DefaultMaterial {
                        diffuseColor: "#e0e0e0"
                    }
                ]
            }

            // --- COMPACT PLANT LAYOUT ---

            // 1. SAFETY FENCE
            FenceModel {
                id: fence
                position: Qt.vector3d(500, 0, 0)

                damperOpen: root.damperOpen
                doorOpen: root.doorOpen
            }

            // 2. ENTRY CONVEYOR
            ConveyorModel {
                id: entryConveyor
                position: Qt.vector3d(-2000, 0, 0)
                length: 1875
                height: 700
                conveyorController: root.backend ? root.backend.entryConveyor : null
                sensorPositions: [100.0, 1000.0, 1775.0]
            }

            // 3. MAIN ROBOT
            RobotModel {
                id: plantRobot
                position: Qt.vector3d(0, 0, 1000)
                eulerRotation.z: 90

                axis1: root.backend ? root.backend.robot.axis1 : 0
                axis2: root.backend ? root.backend.robot.axis2 : -90
                axis3: root.backend ? root.backend.robot.axis3 : 90
                axis4: root.backend ? root.backend.robot.axis4 : 0
                axis5: root.backend ? root.backend.robot.axis5 : 0
                axis6: root.backend ? root.backend.robot.axis6 : 0

                // Explicit connection for gripper state
                gripperGripped: false
                Connections {
                    target: root.backend ? root.backend.robot : null
                    function onStateChanged() {
                        plantRobot.gripperGripped = root.backend.robot.gripperGripped;
                    }
                }
            }
            // 4. STATIONS
            Node {
                id: stationsRow
                position: Qt.vector3d(0, 0, -1200)

                // Analysis Station
                Node {
                    position: Qt.vector3d(-1000, 0, 0)
                    StationModel {
                        color: "#3498db"
                    }
                    StationFrameModel {}
                    CameraModel {
                        position: Qt.vector3d(0, 1950, 0) // Centered on frame crossbar
                        eulerRotation.x: 90 // Straight down
                    }
                }

                // Laser Station
                Node {
                    position: Qt.vector3d(1000, 0, 0)
                    StationModel {
                        color: "#e74c3c"
                    }

                    // Offset frame to compensate for laser angle
                    // Laser is tilted 30deg. Height is ~1m above table.
                    // offset = tan(30) * 1000 ~= 577mm
                    Node {
                        position: Qt.vector3d(580, 0, 0)
                        StationFrameModel {}
                        LaserModel {
                            position: Qt.vector3d(0, 1950, 0)
                            eulerRotation.z: -30 // Points back to center of station
                            laserOn: true
                        }
                    }
                }
            }

            // // 5. EXIT CONVEYOR
            ConveyorModel {
                id: exitConveyor
                position: Qt.vector3d(1750, 0, 0)
                length: 1250
                height: 700
                conveyorController: root.backend ? root.backend.exitConveyor : null
                sensorPositions: [100.0, 1150.0]
            }

            // 6. TRANSFER GANTRY
            GantryModel {
                id: plantGantry
                eulerRotation.x: -90
                position: Qt.vector3d(2500, 0, -225)
                frameHeight: 1600
                xPos: root.gantryX
                zPos: root.gantryZ
            }

            // 7. NEIGHBOR EXIT CONVEYOR
            ConveyorModel {
                id: neighborConveyor
                position: Qt.vector3d(3250, 0, 0)
                length: 1250
                height: 1000
            }
        }
    }

    MouseArea {
        id: mainMouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton

        property real lastX: 0
        property real lastY: 0

        onPressed: mouse => {
            lastX = mouse.x;
            lastY = mouse.y;
        }

        onClicked: mouse => {
            // Only handle left clicks for picking
            if (mouse.button === Qt.LeftButton) {
                let result = view.pick(mouse.x, mouse.y);
                if (result.objectHit) {
                    let obj = result.objectHit;
                    console.info("3D Pick: Hit object", obj.objectName);
                    if (obj.objectName === "safetyDoorHandle") {
                        fence.doorOpen = !fence.doorOpen;
                    }
                }
            }
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
