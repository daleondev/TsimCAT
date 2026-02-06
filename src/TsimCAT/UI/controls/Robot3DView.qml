import QtQuick
import QtQuick3D
import QtQuick3D.Helpers

Item {
    id: root

    // Joint angles in degrees
    property real axis1: 0
    property real axis2: 0
    property real axis3: 0
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
            position: Qt.vector3d(0, 300, 0) // Lift the pivot center slightly

            PerspectiveCamera {
                id: sceneCamera
                position: Qt.vector3d(0, 0, 2500) // Move camera further back
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
                color: "#fff4e5" // Slightly warm
            }

            PointLight {
                position: Qt.vector3d(-1000, 500, -500)
                brightness: 0.4
                color: "#e5f1ff" // Slightly cool
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

            Node {
                id: robotRoot
                eulerRotation.x: -90
                scale: Qt.vector3d(1000, 1000, 1000)

                Node {
                    id: robotBase

                    Model {
                        id: baseLink
                        source: "../assets/meshes/base_link/meshes/node3.mesh"
                        materials: [
                            PrincipledMaterial {
                                baseColor: "#1a1a1a"
                                metalness: 0.8
                                roughness: 0.2
                            }
                        ]
                    }

                    Node {
                        id: joint1
                        position: Qt.vector3d(0, 0, 0.450)
                        eulerRotation.z: -root.axis1

                        Model {
                            id: link1
                            source: "../assets/meshes/link_1/meshes/node3.mesh"
                            materials: [
                                PrincipledMaterial {
                                    baseColor: "#f67828"
                                    metalness: 0.2
                                    roughness: 0.4
                                }
                            ]
                        }

                        Node {
                            id: joint2
                            position: Qt.vector3d(0.150, 0, 0)
                            eulerRotation.y: root.axis2

                            Model {
                                id: link2
                                source: "../assets/meshes/link_2/meshes/node3.mesh"
                                materials: [
                                    PrincipledMaterial {
                                        baseColor: "#f67828"
                                        metalness: 0.2
                                        roughness: 0.4
                                    }
                                ]
                            }

                            Node {
                                id: joint3
                                position: Qt.vector3d(0.610, 0, 0)
                                eulerRotation.y: root.axis3

                                Model {
                                    id: link3
                                    source: "../assets/meshes/link_3/meshes/node3.mesh"
                                    materials: [
                                        PrincipledMaterial {
                                            baseColor: "#f67828"
                                            metalness: 0.2
                                            roughness: 0.4
                                        }
                                    ]
                                }

                                Node {
                                    id: joint4
                                    position: Qt.vector3d(0, 0, 0.02)
                                    eulerRotation.x: -root.axis4

                                    Model {
                                        id: link4
                                        source: "../assets/meshes/link_4/meshes/node3.mesh"
                                        materials: [
                                            PrincipledMaterial {
                                                baseColor: "#f67828"
                                                metalness: 0.2
                                                roughness: 0.4
                                            }
                                        ]
                                    }

                                    Node {
                                        id: joint5
                                        position: Qt.vector3d(0.660, 0, 0)
                                        eulerRotation.y: root.axis5

                                        Model {
                                            id: link5
                                            source: "../assets/meshes/link_5/meshes/node3.mesh"
                                            materials: [
                                                PrincipledMaterial {
                                                    baseColor: "#f67828"
                                                    metalness: 0.2
                                                    roughness: 0.4
                                                }
                                            ]
                                        }

                                        Node {
                                            id: joint6
                                            position: Qt.vector3d(0.080, 0, 0)
                                            eulerRotation.x: -root.axis6

                                            Model {
                                                id: link6
                                                source: "../assets/meshes/link_6/meshes/node3.mesh"
                                                materials: [
                                                    PrincipledMaterial {
                                                        baseColor: "#2a2a2a"
                                                        metalness: 0.9
                                                        roughness: 0.1
                                                    }
                                                ]
                                            }

                                            Node {
                                                id: flange
                                                Node {
                                                    id: tool0
                                                    eulerRotation.y: 90
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
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
                // Rotation: Orbiting around pivot
                cameraPivot.eulerRotation.y -= dx * 0.2;
                cameraPivot.eulerRotation.x = Math.max(-90, Math.min(0, cameraPivot.eulerRotation.x - dy * 0.2));
            } else if (mouse.buttons & (Qt.LeftButton | Qt.MiddleButton)) {
                // Translation: Panning using the camera's local orientation
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
