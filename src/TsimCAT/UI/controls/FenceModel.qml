import QtQuick
import QtQuick3D

Node {
    id: fenceRoot
    property real width: 5000
    property real depth: 4000
    property real height: 2000

    readonly property color fenceColor: "#f1c40f" // Safety Yellow

    Node {
        id: perimeter
        
        // Horizontal bars and vertical posts
        // We'll create 4 sides
        
        // Side 1 (Front)
        FenceSide { 
            position: Qt.vector3d(0, 0, depth/2)
            width: fenceRoot.width
        }
        // Side 2 (Back)
        FenceSide { 
            position: Qt.vector3d(0, 0, -depth/2)
            width: fenceRoot.width
        }
        // Side 3 (Left)
        FenceSide { 
            position: Qt.vector3d(-width/2, 0, 0)
            eulerRotation.y: 90
            width: fenceRoot.depth
        }
        // Side 4 (Right)
        FenceSide { 
            position: Qt.vector3d(width/2, 0, 0)
            eulerRotation.y: 90
            width: fenceRoot.depth
        }
    }

    component FenceSide : Node {
        property real width: 1000
        
        // Posts
        Repeater3D {
            model: Math.ceil(width / 1000) + 1
            delegate: Model {
                position: Qt.vector3d(-width/2 + (index * 1000), 1000, 0)
                source: "#Cube"
                scale: Qt.vector3d(0.5, 20, 0.5)
                materials: [ PrincipledMaterial { baseColor: "#333"; metalness: 0.8 } ]
            }
        }

        // Mesh Panel (Semi-transparent)
        Model {
            position: Qt.vector3d(0, 1000, 0)
            source: "#Rectangle"
            scale: Qt.vector3d(width/100, 20, 1)
            materials: [
                PrincipledMaterial {
                    baseColor: fenceRoot.fenceColor
                    opacity: 0.2
                    alphaMode: PrincipledMaterial.Blend
                    metalness: 0.5
                }
            ]
        }

        // Top/Bottom Rails
        Model {
            position: Qt.vector3d(0, 1900, 0)
            source: "#Cube"
            scale: Qt.vector3d(width/100, 0.2, 0.2)
            materials: [ PrincipledMaterial { baseColor: fenceRoot.fenceColor } ]
        }
        Model {
            position: Qt.vector3d(0, 100, 0)
            source: "#Cube"
            scale: Qt.vector3d(width/100, 0.2, 0.2)
            materials: [ PrincipledMaterial { baseColor: fenceRoot.fenceColor } ]
        }
    }
}
