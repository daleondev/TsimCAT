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
            id: sceneRoot

            DirectionalLight {
                eulerRotation.x: -30
                eulerRotation.y: -45
                brightness: 1.0
            }

            DirectionalLight {
                eulerRotation.x: 30
                eulerRotation.y: 135
                brightness: 0.5
            }

            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(500, 500, 500)
                eulerRotation: Qt.vector3d(-30, 45, 0)
            }

            Node {
                id: robotRoot
                eulerRotation.x: -90
                scale: Qt.vector3d(1000, 1000, 1000) // Scale to mm for better viewing with default camera settings

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
                        eulerRotation.z: -root.axis1 // Axis 0 0 -1

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
                            eulerRotation.y: root.axis2 // Axis 0 1 0

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
                                eulerRotation.y: root.axis3 // Axis 0 1 0

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
                                    eulerRotation.x: -root.axis4 // Axis -1 0 0

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
                                        eulerRotation.y: root.axis5 // Axis 0 1 0

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
                                            eulerRotation.x: -root.axis6 // Axis -1 0 0

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
                                                // joint_a6-flange is fixed at 0 0 0
                                                Node {
                                                    id: tool0
                                                    // joint_flange-tool0: origin 0 0 0, rpy 0 90 0
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
                        // joint_base_link-base is fixed at 0 0 0
                    }
                }
            }

            // Ground plane
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

        WasdController {
            anchors.fill: parent
            controlledObject: camera
        }
    }
}
