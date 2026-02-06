import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root

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
            position: Qt.vector3d(0, 500, 0)

            PerspectiveCamera {
                id: sceneCamera
                position: Qt.vector3d(0, 0, 5000) // Zoomed back in for compact cell
                clipNear: 10
                clipFar: 30000
            }
        }

        Node {
            id: sceneRoot

            DirectionalLight {
                eulerRotation.x: -30
                eulerRotation.y: -45
                brightness: 1.2
                castsShadow: true
            }

            PointLight {
                position: Qt.vector3d(2000, 2000, 2000)
                brightness: 0.5
            }

            // Floor
            Model {
                y: -1
                source: "#Rectangle"
                scale: Qt.vector3d(100, 100, 1)
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
                width: 6000
                depth: 4500
                position: Qt.vector3d(500, 0, 0)
            }

            // 2. ENTRY CONVEYOR (Neighbor -> Cell)
            ConveyorModel {
                id: entryConveyor
                position: Qt.vector3d(-1800, 0, 0) // Compact side
                length: 1500
            }

            // 3. MAIN ROBOT (Center)
            RobotModel {
                id: plantRobot
                position: Qt.vector3d(0, 0, 0)
            }

            // 4. STATIONS (Side-by-Side behind robot)
            Node {
                id: stationsRow
                position: Qt.vector3d(0, 0, -1200) // Behind robot

                // Analysis Station
                Node {
                    position: Qt.vector3d(-800, 0, 0)
                    StationModel { color: "#3498db" }
                    CameraModel {
                        position: Qt.vector3d(0, 1800, 0)
                        eulerRotation.x: 90
                    }
                }

                // Laser Station
                Node {
                    position: Qt.vector3d(800, 0, 0)
                    StationModel { color: "#e74c3c" }
                    LaserModel {
                        position: Qt.vector3d(0, 1800, 0)
                        laserOn: true
                    }
                }
            }

            // 5. EXIT CONVEYOR (Shortened)
            ConveyorModel {
                id: exitConveyor
                position: Qt.vector3d(1800, 0, 0)
                length: 1500
            }

            // 6. TRANSFER GANTRY (2-Axis YZ)
            GantryModel {
                id: plantGantry
                position: Qt.vector3d(2800, 0, 0) // Very close to exit
                yPos: 500
                zPos: 400
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        onPressed: mouse => { lastX = mouse.x; lastY = mouse.y; }
        property real lastX: 0
        property real lastY: 0
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
            lastX = mouse.x; lastY = mouse.y;
        }
        onWheel: wheel => {
            let zoomSpeed = sceneCamera.position.z * 0.1;
            sceneCamera.position.z = Math.max(500, Math.min(15000, sceneCamera.position.z - (wheel.angleDelta.y / 120.0) * zoomSpeed));
        }
    }
}