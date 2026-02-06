import QtQuick
import QtQuick3D

Node {
    id: robotRoot

    // Joint angles in degrees
    property real axis1: 0
    property real axis2: -90
    property real axis3: 90
    property real axis4: 0
    property real axis5: 0
    property real axis6: 0

    eulerRotation.x: -90
    scale: Qt.vector3d(2000, 2000, 2000)

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
            eulerRotation.z: -robotRoot.axis1

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
                eulerRotation.y: robotRoot.axis2

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
                    eulerRotation.y: robotRoot.axis3

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
                        eulerRotation.x: -robotRoot.axis4

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
                            eulerRotation.y: robotRoot.axis5

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
                                eulerRotation.x: -robotRoot.axis6

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
