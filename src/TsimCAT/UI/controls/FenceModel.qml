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

    // Helper for a physical wire mesh panel
    component WireMesh : Node {
        property real w: 1000
        property real h: 2000
        
        // Vertical Wires
        Repeater3D {
            model: Math.ceil(w / 100) + 1
            delegate: Model {
                position: Qt.vector3d(-w/2 + index * 100, h/2, 0)
                source: "#Cube"
                scale: Qt.vector3d(0.02, h/100, 0.02)
                materials: [ PrincipledMaterial { baseColor: fenceRoot.fenceColor; metalness: 0.8 } ]
            }
        }
        
        // Horizontal Wires
        Repeater3D {
            model: Math.ceil(h / 100) + 1
            delegate: Model {
                position: Qt.vector3d(0, index * 100, 0)
                source: "#Cube"
                scale: Qt.vector3d(w/100, 0.02, 0.02)
                materials: [ PrincipledMaterial { baseColor: fenceRoot.fenceColor; metalness: 0.8 } ]
            }
        }

        // Faint semi-transparent backing to give it some "volume"
        Model {
            position: Qt.vector3d(0, h/2, 0)
            source: "#Cube"
            scale: Qt.vector3d(w/100, h/100, 0.01)
            materials: [
                PrincipledMaterial {
                    baseColor: fenceRoot.fenceColor
                    opacity: 0.1
                    alphaMode: PrincipledMaterial.Blend
                    cullMode: PrincipledMaterial.NoCulling
                }
            ]
        }
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

        // The Mesh
        WireMesh {
            visible: showMesh
            w: panelWidth
            h: 2000
        }

        // Horizontal Rails (Frame)
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
        Node {
            position: Qt.vector3d(0, 1400 + (fenceRoot.damperOpen ? 1000 : 0), 0)
            Behavior on position { Vector3dAnimation { duration: 1500; easing.type: Easing.InOutQuad } }
            
            WireMesh {
                w: panelWidth
                h: 1200
                // Adjust height for the blade
                y: -600 
            }
            
            // Blade Frame
            Model {
                source: "#Cube"
                scale: Qt.vector3d(panelWidth/100, 12, 0.2)
                materials: [ PrincipledMaterial { baseColor: "#bdc3c7"; opacity: 0.3; alphaMode: PrincipledMaterial.Blend } ]
            }
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
            Behavior on eulerRotation { 
                Vector3dAnimation { 
                    duration: 1200
                    easing.type: Easing.InOutBack 
                } 
            }

            WireMesh {
                position: Qt.vector3d(doorWidth/2, 0, 0)
                w: doorWidth
                h: 2000
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
