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
                position: Qt.vector3d(0, 0, 8000) // Zoomed out a bit more for larger scene
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
                scale: Qt.vector3d(250, 250, 1)
                eulerRotation.x: -90
                materials: [
                    DefaultMaterial {
                        diffuseColor: "#e0e0e0"
                    }
                ]
            }

            // --- THE PLANT LAYOUT (Adjusted spacing for 2x robot scale) ---

            // 1. ENTRY CONVEYOR
            ConveyorModel {
                id: entryConveyor
                position: Qt.vector3d(-3500, 0, 0) // Moved further left
                length: 3000
            }

            // 2. MAIN ROBOT (Center)
            RobotModel {
                id: plantRobot
                position: Qt.vector3d(0, 0, 0)
            }

            // 3. ANALYSIS STATION
            Node {
                id: analysisStation
                position: Qt.vector3d(0, 0, 1800) // Moved further forward

                StationModel {
                    color: "#3498db"
                }

                CameraModel {
                    position: Qt.vector3d(0, 1800, 0)
                    eulerRotation.x: 90
                }
            }

            // 4. LASER STATION
            Node {
                id: laserStation
                position: Qt.vector3d(1800, 0, 0) // Moved further right

                StationModel {
                    color: "#e74c3c"
                }

                LaserModel {
                    position: Qt.vector3d(0, 1800, 0)
                    laserOn: true
                }
            }

            // 5. EXIT CONVEYOR
            ConveyorModel {
                id: exitConveyor
                position: Qt.vector3d(4500, 0, 0) // Moved further right
                length: 3500
            }

            // 6. TRANSFER GANTRY
            GantryModel {
                id: plantGantry
                position: Qt.vector3d(6000, 0, 0) // Over end of exit conveyor
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
            sceneCamera.position.z = Math.max(500, Math.min(20000, sceneCamera.position.z - (wheel.angleDelta.y / 120.0) * zoomSpeed));
        }
    }
}
