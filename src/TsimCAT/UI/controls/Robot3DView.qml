import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root

    // Joint angles in degrees
    property real axis1: 0
    property real axis2: -90
    property real axis3: 90
    property real axis4: 0
    property real axis5: 0
    property real axis6: 0

    View3D {
        id: view
        anchors.fill: parent
        camera: sceneCamera

        environment: SceneEnvironment {
            clearColor: "#fdfdfd"
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
            eulerRotation: Qt.vector3d(-25, 45, 0)
            position: Qt.vector3d(0, 300, 0)

            PerspectiveCamera {
                id: sceneCamera
                position: Qt.vector3d(0, 0, 2500)
                clipNear: 1.0
                clipFar: 10000
            }
        }

        Node {
            id: sceneRoot

            DirectionalLight {
                eulerRotation.x: -30
                eulerRotation.y: -45
                brightness: 1.5
                castsShadow: true
                shadowFactor: 10
                shadowMapQuality: DirectionalLight.ShadowMapQualityHigh
            }

            PointLight {
                position: Qt.vector3d(1000, 1000, 1000)
                brightness: 0.8
                color: "#fff4e5"
            }

            PointLight {
                position: Qt.vector3d(-1000, 500, -500)
                brightness: 0.4
                color: "#e5f1ff"
            }

            // Ground Plane
            Node {
                y: -1
                Model {
                    source: "#Rectangle"
                    scale: Qt.vector3d(50, 50, 1)
                    eulerRotation.x: -90
                    materials: [
                        DefaultMaterial {
                            diffuseColor: "#eeeeee"
                        }
                    ]
                }
            }

            RobotModel {
                id: robot
                axis1: root.axis1
                axis2: root.axis2
                axis3: root.axis3
                axis4: root.axis4
                axis5: root.axis5
                axis6: root.axis6
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        preventStealing: true

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
                let speed = sceneCamera.position.z / 1500.0;
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
            sceneCamera.position.z = Math.max(200, Math.min(5000, sceneCamera.position.z - (wheel.angleDelta.y / 120.0) * zoomSpeed));
        }
    }
}