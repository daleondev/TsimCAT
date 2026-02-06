import QtQuick
import QtQuick3D

Node {
    id: gantryRoot

    // Positions in mm
    property real yPos: 0
    property real zPos: 0

    readonly property color frameColor: "#2c3e50"
    readonly property color axisColor: "#f67828"

    // Two vertical pillars
    Model {
        position: Qt.vector3d(0, 1000, -1000)
        source: "#Cube"
        scale: Qt.vector3d(1, 20, 1)
        materials: [ PrincipledMaterial { baseColor: frameColor; metalness: 0.8 } ]
    }
    Model {
        position: Qt.vector3d(0, 1000, 1000)
        source: "#Cube"
        scale: Qt.vector3d(1, 20, 1)
        materials: [ PrincipledMaterial { baseColor: frameColor; metalness: 0.8 } ]
    }

    // Horizontal Rail (Y-axis)
    Model {
        position: Qt.vector3d(0, 2000, 0)
        source: "#Cube"
        scale: Qt.vector3d(0.8, 0.8, 20)
        materials: [ PrincipledMaterial { baseColor: frameColor; metalness: 0.9 } ]
    }

    // Y Carriage
    Node {
        id: carriage
        position: Qt.vector3d(0, 2000, gantryRoot.yPos)

        Model {
            source: "#Cube"
            scale: Qt.vector3d(1.2, 1.2, 1.2)
            materials: [ PrincipledMaterial { baseColor: axisColor } ]
        }

        // Z Axis Rod
        Model {
            id: zRod
            // Make rod longer (2000mm) so it stays inside carriage even when extended
            // When zPos=0, we want tip flush with carriage bottom.
            // Carriage bottom is at 0 (local). Rod length 2000. Center 0 means -1000 to +1000.
            // If we want tip at 0, center needs to be +1000? No, that would be 0 to 2000 (sticking up).
            // We want it to stick DOWN.
            // But if it extends down, it must have length "stored" above.
            // Let's say zPos is extension distance downwards.
            // zPos=0: Tip at 0. Top at +2000.
            // zPos=1000: Tip at -1000. Top at +1000.
            // Rod moves DOWN by zPos.
            // So position.y = 1000 - zPos.
            // Wait, coordinate system: +Y is Up.
            // Carriage is at Y=2000 (global).
            // Local 0 is at Carriage center.
            // Rod Center at Y=1000 - zPos.
            // Tip is at (1000 - zPos) - 1000 = -zPos.
            // Top is at (1000 - zPos) + 1000 = 2000 - zPos.
            // This works!
            
            position: Qt.vector3d(0, 1000 - gantryRoot.zPos, 0)
            source: "#Cube"
            scale: Qt.vector3d(0.4, 20, 0.4) // 2000mm long
            materials: [ PrincipledMaterial { baseColor: "#bdc3c7"; metalness: 1.0 } ]
        }

        // INDUSTRIAL GRIPPER (At the very tip of the Z Rod)
        Node {
            // Rod tip is at -zPos.
            position: Qt.vector3d(0, -gantryRoot.zPos, 0)
            
            // Mounting Plate
            Model {
                position: Qt.vector3d(0, 10, 0)
                source: "#Cube"
                scale: Qt.vector3d(1.2, 0.2, 1.2)
                materials: [ PrincipledMaterial { baseColor: "#333" } ]
            }
            
            // Actuator Body
            Model {
                position: Qt.vector3d(0, -25, 0)
                source: "#Cube"
                scale: Qt.vector3d(0.8, 0.5, 0.8)
                materials: [ PrincipledMaterial { baseColor: "#2c3e50"; metalness: 0.9 } ]
            }
            
            // Left Finger
            Model {
                position: Qt.vector3d(-30, -75, 0)
                source: "#Cube"
                scale: Qt.vector3d(0.15, 1, 0.4)
                materials: [ PrincipledMaterial { baseColor: "#7f8c8d"; metalness: 1.0 } ]
            }
            
            // Right Finger
            Model {
                position: Qt.vector3d(30, -75, 0)
                source: "#Cube"
                scale: Qt.vector3d(0.15, 1, 0.4)
                materials: [ PrincipledMaterial { baseColor: "#7f8c8d"; metalness: 1.0 } ]
            }
        }
    }
}