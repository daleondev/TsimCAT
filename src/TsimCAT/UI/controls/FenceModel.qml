import QtQuick
import QtQuick3D

Node {
    id: fenceRoot
    property real width: 6000
    property real depth: 4500
    property real height: 2000

    property bool damperOpen: false
    property bool doorOpen: false

    // readonly property color fenceColor: "#f1c40f" // Safety Yellow
    readonly property color fenceColor: '#5e5e5e'

    // Improved Procedural Chain Link Texture
    Texture {
        id: gridTexture
        sourceItem: Item {
            width: 64
            height: 64
            // Removed suspicious x/y offsets

            Rectangle {
                anchors.fill: parent
                color: "black"

                Rectangle {
                    width: 100
                    height: 6
                    color: "white"
                    rotation: 45
                    anchors.centerIn: parent
                }
                Rectangle {
                    width: 100
                    height: 6
                    color: "white"
                    rotation: -45
                    anchors.centerIn: parent
                }
            }
            layer.enabled: true
        }
        scaleU: 30
        scaleV: 15
        tilingModeHorizontal: Texture.Repeat
        tilingModeVertical: Texture.Repeat
    }

    // Helper for a physical wire mesh panel
    component WireMesh: Node {
        property real w: 1000
        property real h: 2000

        // Vertical Wires
        Repeater3D {
            model: Math.ceil(w / 100) + 1
            delegate: Model {
                position: Qt.vector3d(-w / 2 + index * 100, h / 2, 0)
                source: "#Cube"
                scale: Qt.vector3d(0.02, h / 100, 0.02)
                materials: [
                    PrincipledMaterial {
                        baseColor: fenceRoot.fenceColor
                        metalness: 0.8
                    }
                ]
            }
        }

        // Horizontal Wires
        Repeater3D {
            model: Math.ceil(h / 100) + 1
            delegate: Model {
                position: Qt.vector3d(0, index * 100, 0)
                source: "#Cube"
                scale: Qt.vector3d(w / 100, 0.02, 0.02)
                materials: [
                    PrincipledMaterial {
                        baseColor: fenceRoot.fenceColor
                        metalness: 0.8
                    }
                ]
            }
        }
    }

    // Common Material for frame/posts
    PrincipledMaterial {
        id: postMaterial
        baseColor: "#222"
        metalness: 0.8
    }

    // Helper for a single fence panel segment
    component FencePanel: Node {
        property real panelWidth: 1000
        property bool showMesh: true
        property bool showLeftPost: true

        Model {
            visible: showLeftPost
            position: Qt.vector3d(-panelWidth / 2, 1000, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.5, 20, 0.5)
            materials: [postMaterial]
        }

        WireMesh {
            visible: showMesh
            w: panelWidth
            h: 2000
        }

        // Frame Rails
        Model {
            position: Qt.vector3d(0, 1950, 0)
            source: "#Cube"
            scale: Qt.vector3d(panelWidth / 100 + 0.1, 0.3, 0.3)
            materials: [
                PrincipledMaterial {
                    baseColor: fenceRoot.fenceColor
                }
            ]
        }
        Model {
            position: Qt.vector3d(0, 50, 0)
            source: "#Cube"
            scale: Qt.vector3d(panelWidth / 100 + 0.1, 0.3, 0.3)
            materials: [
                PrincipledMaterial {
                    baseColor: fenceRoot.fenceColor
                }
            ]
        }
    }

    // Closing post component
    component EndPost: Model {
        position: Qt.vector3d(0, 1000, 0)
        source: "#Cube"
        scale: Qt.vector3d(0.5, 20, 0.5)
        materials: [postMaterial]
    }

    // Guillotine Damper Component
    component GuillotineDamper: Node {
        property real panelWidth: 1200

        Model {
            position: Qt.vector3d(-panelWidth / 2, 1000, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.8, 20, 0.8)
            materials: [postMaterial]
        }
        Model {
            position: Qt.vector3d(panelWidth / 2, 1000, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.8, 20, 0.8)
            materials: [postMaterial]
        }

        Model {
            position: Qt.vector3d(0, 2100, 0)
            source: "#Cube"
            scale: Qt.vector3d(panelWidth / 100 + 1, 2, 1.2)
            materials: [
                PrincipledMaterial {
                    baseColor: "#333"
                }
            ]
        }

        // Moving Blade
        Node {
            id: damperBlade
            y: 1400 + (fenceRoot.damperOpen ? 1000 : 0)

            Behavior on y {
                NumberAnimation {
                    duration: 1500
                    easing.type: Easing.InOutQuad
                }
            }

            WireMesh {
                w: panelWidth
                h: 1200
                y: -600
            }
        }
    }

    // Double Safety Door Component
    component SafetyDoor: Node {
        property real doorWidth: 2000

        // Left Hinge Post
        Model {
            position: Qt.vector3d(-doorWidth / 2, 1000, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.7, 20, 0.7)
            materials: [postMaterial]
        }

        // Left Swinging Leaf
        Node {
            position: Qt.vector3d(-doorWidth / 2, 0, 0)
            eulerRotation.y: fenceRoot.doorOpen ? -100 : 0
            Behavior on eulerRotation.y {
                NumberAnimation {
                    duration: 1200
                    easing.type: Easing.InOutBack
                }
            }

            WireMesh {
                position: Qt.vector3d(doorWidth / 4, 0, 0)
                w: doorWidth / 2
                h: 2000
            }

            Model {
                position: Qt.vector3d(doorWidth / 4, 1950, 0)
                source: "#Cube"
                scale: Qt.vector3d(doorWidth / 200, 0.4, 0.4)
                materials: [
                    PrincipledMaterial {
                        baseColor: fenceRoot.fenceColor
                    }
                ]
            }
        }

        // Right Swinging Leaf
        Node {
            position: Qt.vector3d(doorWidth / 2, 0, 0)
            eulerRotation.y: fenceRoot.doorOpen ? 100 : 0
            Behavior on eulerRotation.y {
                NumberAnimation {
                    duration: 1200
                    easing.type: Easing.InOutBack
                }
            }

            WireMesh {
                position: Qt.vector3d(-doorWidth / 4, 0, 0)
                w: doorWidth / 2
                h: 2000
            }

            Model {
                position: Qt.vector3d(-doorWidth / 4, 1950, 0)
                source: "#Cube"
                scale: Qt.vector3d(doorWidth / 200, 0.4, 0.4)
                materials: [
                    PrincipledMaterial {
                        baseColor: fenceRoot.fenceColor
                    }
                ]
            }
        }

        // Right Hinge Post
        Model {
            position: Qt.vector3d(doorWidth / 2, 1000, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.7, 20, 0.7)
            materials: [postMaterial]
        }
    }

    // --- ASSEMBLE PERIMETER ---

    // Back Side (Fixed positions)
    Node {
        position: Qt.vector3d(0, 0, -depth / 2)
        Repeater3D {
            model: 6
            delegate: FencePanel {
                position: Qt.vector3d(-2500 + index * 1000, 0, 0)
            }
        }
        EndPost {
            position: Qt.vector3d(3000, 1000, 0)
        }
    }

    // Front Side (Fixed positions and missing panel)
    Node {
        position: Qt.vector3d(0, 0, depth / 2)
        FencePanel {
            position: Qt.vector3d(-2500, 0, 0)
        }
        FencePanel {
            position: Qt.vector3d(-1500, 0, 0)
        } // Added missing panel

        // Double Safety Door (covers -1000 to 1000)
        SafetyDoor {
            position: Qt.vector3d(0, 0, 0)
            doorWidth: 2000
        }

        FencePanel {
            position: Qt.vector3d(1500, 0, 0)
            showLeftPost: false
        }
        FencePanel {
            position: Qt.vector3d(2500, 0, 0)
        }
        EndPost {
            position: Qt.vector3d(3000, 1000, 0)
        }
    }

    // Left Side (Entry)
    Node {
        position: Qt.vector3d(-width / 2, 0, 0)
        eulerRotation.y: 90
        // Use depth based positions
        FencePanel {
            position: Qt.vector3d(-1500, 0, 0)
        }
        EndPost {
            position: Qt.vector3d(-1000, 1000, 0)
        }
        GuillotineDamper {
            position: Qt.vector3d(0, 0, 0)
            panelWidth: 1200
        }
        FencePanel {
            position: Qt.vector3d(1500, 0, 0)
        }
        EndPost {
            position: Qt.vector3d(2000, 1000, 0)
        }
    }

    // Right Side (Open for transfer)
    Node {
        position: Qt.vector3d(width / 2, 0, 0)
        eulerRotation.y: 90
        FencePanel {
            position: Qt.vector3d(-1500, 0, 0)
        }
        // Middle area is open
        FencePanel {
            position: Qt.vector3d(1500, 0, 0)
        }
        EndPost {
            position: Qt.vector3d(2000, 1000, 0)
        }
        // Add post at the other side of the gap
        EndPost {
            position: Qt.vector3d(1000, 1000, 0)
        }
        EndPost {
            position: Qt.vector3d(-1000, 1000, 0)
        }
    }
}
