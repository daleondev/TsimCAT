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

        environment: SceneEnvironment {
            clearColor: "#f0f0f0"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        Node {
            id: cameraPivot
            eulerRotation: Qt.vector3d(-30, 45, 0)

            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 0, 2500)
                clipNear: 10
                clipFar: 10000
            }
        }

        Node {
            id: sceneRoot

            DirectionalLight {
                eulerRotation.x: -30
                eulerRotation.y: -45
                brightness: 1.0
                castsShadow: true
            }

            PointLight {
                position: Qt.vector3d(0, 500, 500)
                brightness: 0.5
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
                            DefaultMaterial {
                                diffuseColor: "#0e0e10"
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
                                DefaultMaterial {
                                    diffuseColor: "#f67828"
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
                                    DefaultMaterial {
                                        diffuseColor: "#f67828"
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
                                        DefaultMaterial {
                                            diffuseColor: "#f67828"
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
                                            DefaultMaterial {
                                                diffuseColor: "#f67828"
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
                                                DefaultMaterial {
                                                    diffuseColor: "#f67828"
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
                                                    DefaultMaterial {
                                                        diffuseColor: "#817863"
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

                    Node {
                        id: base
                    }
                }
            }

            Model {
                source: "#Rectangle"
                y: -1
                scale: Qt.vector3d(20, 20, 1)
                eulerRotation.x: -90
                materials: [
                    DefaultMaterial {
                        diffuseColor: "#e0e0e0"
                    }
                ]
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
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
                            cameraPivot.eulerRotation.x -= dy * 0.2;
                        } else if (mouse.buttons & Qt.LeftButton) {
                            // Translation: Panning using the camera's local orientation
                            // This makes dragging left/right/up/down always follow the screen axes
                            let speed = camera.position.z / 1000.0;
                            let right = camera.mapDirectionToScene(Qt.vector3d(1, 0, 0));
                            let up = camera.mapDirectionToScene(Qt.vector3d(0, 1, 0));
                            
                            let move = right.times(-dx * speed).plus(up.times(dy * speed));
                            cameraPivot.position = cameraPivot.position.plus(move);
                        }
            lastX = mouse.x;
            lastY = mouse.y;
        }

        onWheel: wheel => {
            camera.position.z = Math.max(100, camera.position.z - wheel.angleDelta.y * 1.0);
        }
    }
}
