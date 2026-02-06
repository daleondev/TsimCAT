import QtQuick
import QtQuick3D

Node {
    id: fenceRoot
    property real width: 6000
    property real depth: 4500
    property real height: 2000

    property bool damperOpen: false
    property bool doorOpen: false

    readonly property color fenceColor: "#f1c40f" // Safety Yellow

    // Define the 2D pattern here so it's part of the same file but outside the 3D scene
    Item {
        id: patternSource
        width: 128
        height: 128
        visible: false // sourceItem still works if visible is false but it must be in the 2D tree
        
        Rectangle {
            anchors.fill: parent
            color: "black"
            
            // Draw a diagonal mesh pattern
            Rectangle {
                width: 200; height: 15
                color: "white"
                rotation: 45
                anchors.centerIn: parent
            }
            Rectangle {
                width: 200; height: 15
                color: "white"
                rotation: -45
                anchors.centerIn: parent
            }
        }
        layer.enabled: true
    }

    Texture {
        id: gridTexture
        sourceItem: patternSource
        scaleU: 40
        scaleV: 20
        tilingModeHorizontal: Texture.Repeat
        tilingModeVertical: Texture.Repeat
    }

    // Common Material for all mesh panels
    PrincipledMaterial {
        id: fenceMeshMaterial
        baseColor: fenceRoot.fenceColor
        metalness: 0.5
        roughness: 0.1
        opacityMap: gridTexture
        alphaMode: PrincipledMaterial.Mask
        cullMode: PrincipledMaterial.NoCulling // Fix: Show from both sides
    }

    // Helper for a single fence panel segment
    component FencePanel : Node {
        property real panelWidth: 1000
        property bool showMesh: true

        // Left Post
        Model {
            position: Qt.vector3d(-panelWidth/2, 1000, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.5, 20, 0.5)
            materials: [ PrincipledMaterial { baseColor: "#222"; metalness: 0.8 } ]
        }

        // Mesh Panel
        Model {
            visible: showMesh
            position: Qt.vector3d(0, 1000, 0)
            source: "#Rectangle"
            scale: Qt.vector3d(panelWidth/100, 20, 1)
            materials: [ fenceMeshMaterial ]
        }

        // Rails
        Model {
            position: Qt.vector3d(0, 1950, 0)
            source: "#Cube"
            scale: Qt.vector3d(panelWidth/100, 0.3, 0.3)
            materials: [ PrincipledMaterial { baseColor: fenceRoot.fenceColor } ]
        }
        Model {
            position: Qt.vector3d(0, 50, 0)
            source: "#Cube"
            scale: Qt.vector3d(panelWidth/100, 0.3, 0.3)
            materials: [ PrincipledMaterial { baseColor: fenceRoot.fenceColor } ]
        }
    }

    // Guillotine Damper Component
    component GuillotineDamper : Node {
        property real panelWidth: 1200
        
        // Frame/Guides
        Model {
            position: Qt.vector3d(-panelWidth/2, 1000, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.8, 20, 0.8)
            materials: [ PrincipledMaterial { baseColor: "#111" } ]
        }
        Model {
            position: Qt.vector3d(panelWidth/2, 1000, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.8, 20, 0.8)
            materials: [ PrincipledMaterial { baseColor: "#111" } ]
        }
        
        Model {
            position: Qt.vector3d(0, 2100, 0)
            source: "#Cube"
            scale: Qt.vector3d(panelWidth/100 + 1, 2, 1.2)
            materials: [ PrincipledMaterial { baseColor: "#333" } ]
        }

        // Moving Blade
        Model {
            position: Qt.vector3d(0, 1400 + (fenceRoot.damperOpen ? 1000 : 0), 0)
            source: "#Rectangle"
            scale: Qt.vector3d(panelWidth/100, 12, 1)
            materials: [
                PrincipledMaterial {
                    baseColor: "#7f8c8d"
                    opacityMap: gridTexture
                    alphaMode: PrincipledMaterial.Mask
                    cullMode: PrincipledMaterial.NoCulling
                }
            ]
            Behavior on position { Vector3dAnimation { duration: 1500; easing.type: Easing.InOutQuad } }
        }
    }

    // Safety Door Component
    component SafetyDoor : Node {
        property real doorWidth: 1000
        
        Model {
            position: Qt.vector3d(-doorWidth/2, 1000, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.7, 20, 0.7)
            materials: [ PrincipledMaterial { baseColor: "#222" } ]
        }

        Node {
            position: Qt.vector3d(-doorWidth/2, 0, 0)
            eulerRotation.y: fenceRoot.doorOpen ? -100 : 0
            Behavior on eulerRotation { Vector3dAnimation { duration: 1000; easing.type: Easing.InOutQuad } }

            Model {
                position: Qt.vector3d(doorWidth/2, 1000, 0)
                source: "#Rectangle"
                scale: Qt.vector3d(doorWidth/100, 20, 1)
                materials: [ fenceMeshMaterial ]
            }
            Model {
                position: Qt.vector3d(doorWidth/2, 1950, 0)
                source: "#Cube"
                scale: Qt.vector3d(doorWidth/100 + 0.2, 0.4, 0.4)
                materials: [ PrincipledMaterial { baseColor: fenceRoot.fenceColor } ]
            }
        }
    }

    // --- ASSEMBLE PERIMETER ---

    // Back Side
    Node {
        position: Qt.vector3d(0, 0, -depth/2)
        Repeater3D {
            model: 6
            delegate: FencePanel { position: Qt.vector3d(-2500 + index * 1000, 0, 0) }
        }
    }

    // Front Side
    Node {
        position: Qt.vector3d(0, 0, depth/2)
        FencePanel { position: Qt.vector3d(-2500, 0, 0) }
        FencePanel { position: Qt.vector3d(-1500, 0, 0) }
        SafetyDoor { position: Qt.vector3d(0, 0, 0) }
        FencePanel { position: Qt.vector3d(1500, 0, 0) }
        FencePanel { position: Qt.vector3d(2500, 0, 0) }
    }

    // Left Side
    Node {
        position: Qt.vector3d(-width/2, 0, 0)
        eulerRotation.y: 90
        FencePanel { position: Qt.vector3d(-1500, 0, 0) }
        GuillotineDamper { position: Qt.vector3d(0, 0, 0) }
        FencePanel { position: Qt.vector3d(1500, 0, 0) }
    }

    // Right Side
    Node {
        position: Qt.vector3d(width/2, 0, 0)
        eulerRotation.y: 90
        FencePanel { position: Qt.vector3d(-1500, 0, 0) }
        FencePanel { position: Qt.vector3d(0, 0, 0); showMesh: false } 
        FencePanel { position: Qt.vector3d(1500, 0, 0) }
    }
}