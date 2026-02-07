pragma ComponentBehavior: Bound
import QtQuick
import QtQuick3D

Node {
    id: fenceRoot
    readonly property real width: 6000
    readonly property real depth: 3500
    readonly property real height: 2000
    readonly property real fencePanelWidth: 1000

    property bool damperOpen: false
    property bool doorOpen: false

    PrincipledMaterial {
        id: darkMaterial
        baseColor: '#181818'
        metalness: 0.8
    }

    // Common Material for frame/posts
    PrincipledMaterial {
        id: fenceMaterial
        baseColor: '#5e5e5e'
        metalness: 0.8
    }

    // Common Material for frame/posts
    PrincipledMaterial {
        id: postMaterial
        baseColor: "#222222"
        metalness: 0.8
    }

    component WireMesh: Node {
        id: wireMesh
        property real w: fenceRoot.fencePanelWidth
        property real h: fenceRoot.height
        property bool isSafety: false
        property real wireScale: isSafety ? 0.06 : 0.02

        // Vertical Wires
        Repeater3D {
            model: Math.ceil(wireMesh.w / 100) + 1
            delegate: Model {
                required property int index
                position: Qt.vector3d(-wireMesh.w / 2 + index * 100, wireMesh.h / 2, 0)
                source: "#Cube"
                scale: Qt.vector3d(wireMesh.wireScale, wireMesh.h / 100, wireMesh.wireScale)
                materials: [wireMesh.isSafety ? darkMaterial : fenceMaterial]
            }
        }

        // Horizontal Wires
        Repeater3D {
            model: Math.ceil(wireMesh.h / 100) + 1
            delegate: Model {
                required property int index
                position: Qt.vector3d(0, index * 100, 0)
                source: "#Cube"
                scale: Qt.vector3d(wireMesh.w / 100, wireMesh.wireScale, wireMesh.wireScale)
                materials: [wireMesh.isSafety ? darkMaterial : fenceMaterial]
            }
        }

        Model {
            id: backing
            visible: wireMesh.isSafety
            position: Qt.vector3d(0, wireMesh.h / 2, 0)
            source: "#Cube"
            scale: Qt.vector3d(wireMesh.w / 100, wireMesh.h / 100, 0.01)
            materials: [
                PrincipledMaterial {
                    baseColor: '#f1c40f'
                    opacity: 0.32
                    alphaMode: PrincipledMaterial.Blend
                    cullMode: PrincipledMaterial.NoCulling
                }
            ]
        }
    }

    // Post component
    component Post: Model {
        property vector3d externalScale: Qt.vector3d(1, 1, 1)
        readonly property vector3d internalScale: Qt.vector3d(0.5, fenceRoot.height / 100, 0.5)
        source: "#Cube"
        scale: internalScale.times(externalScale)
        materials: [postMaterial]
    }

    // Bar component
    component Bar: Model {
        property vector3d externalScale: Qt.vector3d(1, 1, 1)
        readonly property vector3d internalScale: Qt.vector3d(fenceRoot.fencePanelWidth / 100, 0.3, 0.3)
        source: "#Cube"
        scale: internalScale.times(externalScale)
        materials: [postMaterial]
    }

    // Helper for a single fence panel segment
    component FencePanel: Node {
        id: fencePanel
        property real panelWidth: fenceRoot.fencePanelWidth
        property bool showMesh: true
        property bool showLeftPost: true
        property alias isSafety: wire.isSafety

        Bar {
            position: Qt.vector3d(0, 0, 0)
        }

        Post {
            position: Qt.vector3d(fencePanel.panelWidth / 2, 1000, 0)
        }

        WireMesh {
            id: wire
            visible: fencePanel.showMesh
            w: fencePanel.panelWidth
            h: 2000
        }

        Post {
            position: Qt.vector3d(-fencePanel.panelWidth / 2, 1000, 0)
        }

        Bar {
            position: Qt.vector3d(0, fenceRoot.height, 0)
        }
    }

    // Guillotine Damper Component
    component GuillotineDamper: Node {
        id: guillotineDamper
        property real panelWidth: 1500

        Post {
            position: Qt.vector3d(-guillotineDamper.panelWidth / 2, fenceRoot.height / 2, 0)
            externalScale: Qt.vector3d(2, 1.05, 2)
        }

        Bar {
            position: Qt.vector3d(0, fenceRoot.height, 0)
            externalScale: Qt.vector3d(guillotineDamper.panelWidth / fenceRoot.fencePanelWidth, 3.44, 3.44)
            materials: [postMaterial]
        }

        Post {
            position: Qt.vector3d(guillotineDamper.panelWidth / 2, fenceRoot.height / 2, 0)
            externalScale: Qt.vector3d(2, 1.05, 2)
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

            FencePanel {
                position: Qt.vector3d(0, -600, 0)
                scale: Qt.vector3d(1, 0.6, 1)
                isSafety: true
            }
        }
    }

    component SafetyDoor: Node {
        id: safetyDoor

        Node {
            position: Qt.vector3d(-fenceRoot.fencePanelWidth, 0, 0)
            eulerRotation.y: fenceRoot.doorOpen ? -100 : 0
            Behavior on eulerRotation.y {
                NumberAnimation {
                    duration: 1200
                    easing.type: Easing.InOutQuad
                }
            }

            FencePanel {
                position: Qt.vector3d(fenceRoot.fencePanelWidth / 2, 0, 0)
                isSafety: true
            }
        }

        Node {
            position: Qt.vector3d(fenceRoot.fencePanelWidth, 0, 0)
            eulerRotation.y: fenceRoot.doorOpen ? 100 : 0
            Behavior on eulerRotation.y {
                NumberAnimation {
                    duration: 1200
                    easing.type: Easing.InOutQuad
                }
            }

            FencePanel {
                position: Qt.vector3d(-fenceRoot.fencePanelWidth / 2, 0, 0)
                isSafety: true
            }
        }
    }

    // --- ASSEMBLE PERIMETER ---

    // Back Side
    Node {
        position: Qt.vector3d(0, 0, -fenceRoot.depth / 2)
        Repeater3D {
            model: 6
            delegate: FencePanel {
                required property int index
                position: Qt.vector3d(-2500 + index * 1000, 0, 0)
            }
        }
    }

    // Front Side
    Node {
        position: Qt.vector3d(0, 0, fenceRoot.depth / 2)

        FencePanel {
            position: Qt.vector3d(-2500, 0, 0)
        }

        FencePanel {
            position: Qt.vector3d(-1500, 0, 0)
        }

        FencePanel {
            position: Qt.vector3d(-500, 0, 0)
        }

        SafetyDoor {
            position: Qt.vector3d(1000, 0, 0)
        }

        FencePanel {
            position: Qt.vector3d(2500, 0, 0)
        }
    }

    // Left Side (Entry)
    Node {
        position: Qt.vector3d(-fenceRoot.width / 2, 0, 0)
        eulerRotation.y: 90

        // Use depth based positions
        FencePanel {
            position: Qt.vector3d(-1250, 0, 0)
        }

        GuillotineDamper {
            position: Qt.vector3d(0, 0, 0)
            panelWidth: 1200
        }

        FencePanel {
            position: Qt.vector3d(1250, 0, 0)
        }
    }

    // Right Side (Open for transfer)
    Node {
        position: Qt.vector3d(fenceRoot.width / 2, 0, 0)
        eulerRotation.y: 90

        FencePanel {
            position: Qt.vector3d(-1250, 0, 0)
        }

        // Middle area is open
        FencePanel {
            position: Qt.vector3d(1250, 0, 0)
        }

        Post {
            position: Qt.vector3d(2000, 1000, 0)
        }

        // Add post at the other side of the gap
        Post {
            position: Qt.vector3d(1000, 1000, 0)
        }

        Post {
            position: Qt.vector3d(-1000, 1000, 0)
        }
    }
}
