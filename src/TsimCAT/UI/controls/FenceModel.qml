pragma ComponentBehavior: Bound
import QtQuick
import QtQuick3D

Node {
    id: fenceRoot
    readonly property real width: 6000
    readonly property real depth: 3000
    readonly property real height: 2000
    readonly property real fencePanelWidth: 1000

    property bool entryDamperOpen: false
    property bool exitDamperOpen: false
    property bool doorOpen: false

    PrincipledMaterial {
        id: interactiveMaterial
        baseColor: '#f67828'
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
        property bool isSolid: false
        property real wireScale: isSolid ? 0.06 : 0.02

        // Vertical Wires
        Repeater3D {
            visible: !wireMesh.isSolid
            model: Math.ceil(wireMesh.w / 100) + 1
            delegate: Model {
                required property int index
                position: Qt.vector3d(-wireMesh.w / 2 + index * 100, wireMesh.h / 2, 0)
                source: "#Cube"
                scale: Qt.vector3d(wireMesh.wireScale, wireMesh.h / 100, wireMesh.wireScale)
                materials: [fenceMaterial]
            }
        }

        // Horizontal Wires
        Repeater3D {
            visible: !wireMesh.isSolid
            model: Math.ceil(wireMesh.h / 100) + 1
            delegate: Model {
                required property int index
                position: Qt.vector3d(0, index * 100, 0)
                source: "#Cube"
                scale: Qt.vector3d(wireMesh.w / 100, wireMesh.wireScale, wireMesh.wireScale)
                materials: [fenceMaterial]
            }
        }

        Model {
            id: backing
            visible: wireMesh.isSolid
            position: Qt.vector3d(0, wireMesh.h / 2, 0)
            source: "#Cube"
            scale: Qt.vector3d(wireMesh.w / 100, wireMesh.h / 100, 0.01)
            materials: [
                PrincipledMaterial {
                    baseColor: '#5e5e5e'
                    // opacity: 0.32
                    opacity: 1
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
        property alias isSolid: wire.isSolid

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
        property real ratio: 0.6
        property bool open: false

        // Moving Blade
        Node {
            id: damperBlade
            y: (1 - guillotineDamper.ratio) * fenceRoot.height + (guillotineDamper.open ? 600 : 0)

            Behavior on y {
                NumberAnimation {
                    duration: 1500
                    easing.type: Easing.InOutQuad
                }
            }

            FencePanel {
                scale: Qt.vector3d(1, guillotineDamper.ratio, 1)
                isSolid: true
            }
        }
    }

    component DoorHandle: Node {
        id: handleRoot
        objectName: "doorHandle"

        // Handle Base/Escutcheon (Fixed to the door)
        Model {
            id: handleBase
            objectName: handleRoot.objectName
            pickable: true
            source: "#Cube"
            scale: Qt.vector3d(1, 1.2, 0.5)
            position: Qt.vector3d(0, 0, 0)
            materials: [postMaterial]
        }

        // Rotating Pivot Assembly
        Node {
            id: leverPivot
            position: Qt.vector3d(0, 0, 35) // Offset from base center
            eulerRotation.z: fenceRoot.doorOpen ? -45 : 0

            Behavior on eulerRotation.z {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.InOutQuad
                }
            }

            // The "Shaft" part of the lever
            Model {
                id: handleShaft
                objectName: handleRoot.objectName
                pickable: true
                source: "#Cube"
                scale: Qt.vector3d(0.4, 0.4, 0.4)
                materials: [interactiveMaterial]
            }

            // The "Grip" part of the lever
            Model {
                id: handleGrip
                objectName: handleRoot.objectName
                pickable: true
                source: "#Cube"
                position: Qt.vector3d(60, 0, 30)
                scale: Qt.vector3d(1.6, 0.4, 0.2)
                materials: [interactiveMaterial]
            }
        }
    }

    component SafetyDoor: Node {
        id: safetyDoor

        Node {
            position: Qt.vector3d(fenceRoot.fencePanelWidth / 2, 0, 0)
            eulerRotation.y: fenceRoot.doorOpen ? 100 : 0
            Behavior on eulerRotation.y {
                NumberAnimation {
                    duration: 1200
                    easing.type: Easing.InOutQuad
                }
            }

            FencePanel {
                position: Qt.vector3d(-fenceRoot.fencePanelWidth / 2, 0, 0)
                isSolid: true
            }

            DoorHandle {
                objectName: "safetyDoorHandle"
                position: Qt.vector3d(-fenceRoot.fencePanelWidth + 50, 1000, 30)
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

        Repeater3D {
            model: 4
            delegate: FencePanel {
                required property int index
                position: Qt.vector3d(-2500 + index * 1000, 0, 0)
            }
        }

        SafetyDoor {
            position: Qt.vector3d(1500, 0, 0)
        }

        FencePanel {
            position: Qt.vector3d(2500, 0, 0)
        }
    }

    // Left Side (Entry)
    Node {
        position: Qt.vector3d(-fenceRoot.width / 2, 0, 0)
        eulerRotation.y: 90

        FencePanel {
            position: Qt.vector3d(-1000, 0, 0)
        }

        FencePanel {
            scale: Qt.vector3d(1, 0.3, 1)
        }

        GuillotineDamper {
            open: fenceRoot.entryDamperOpen
        }

        FencePanel {
            position: Qt.vector3d(1000, 0, 0)
        }
    }

    // Right Side (Open for transfer)
    Node {
        position: Qt.vector3d(fenceRoot.width / 2, 0, 0)
        eulerRotation.y: 90

        FencePanel {
            position: Qt.vector3d(-1000, 0, 0)
        }

        FencePanel {
            scale: Qt.vector3d(1, 0.45, 1)
        }

        GuillotineDamper {
            position: Qt.vector3d(0, 0, 0)
            ratio: 0.45
            open: fenceRoot.exitDamperOpen
        }

        // Middle area is open
        FencePanel {
            position: Qt.vector3d(1000, 0, 0)
        }
    }
}
