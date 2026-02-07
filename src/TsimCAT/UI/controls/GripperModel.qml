import QtQuick
import QtQuick3D

Node {
    id: root
    property bool gripped: false
    
    // Debug log to verify signal reception
    onGrippedChanged: console.info("GripperModel: 'gripped' state changed to", gripped)

    // --- SCALING LOGIC ---
    // Robot link meshes are in METERS. 
    // #Cube is 100x100x100 units.
    // To get 1mm (0.001m), we need scale = 0.001 / 100 = 0.00001
    readonly property real mm: 0.00001 

    // --- VISUALS ---

    // 1. MAIN BODY (Approx 120x80x80 mm)
    Model {
        source: "#Cube"
        position: Qt.vector3d(0, 40 * 0.001, 0) // Centered at 40mm height
        scale: Qt.vector3d(120 * mm, 80 * mm, 80 * mm)
        materials: [
            PrincipledMaterial {
                baseColor: "#2c3e50"
                metalness: 0.6
                roughness: 0.4
            }
        ]
    }

    // 2. LEFT FINGER (Sliding)
    Node {
        id: leftFinger
        // Open: -45mm, Closed: -15mm
        position: Qt.vector3d((root.gripped ? -15 : -45) * 0.001, 80 * 0.001, 0)

        Behavior on position.x {
            NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
        }

        // Finger Tip (High contrast silver)
        Model {
            position: Qt.vector3d(0, 40 * 0.001, 0) // 40mm forward from base
            source: "#Cube"
            scale: Qt.vector3d(20 * mm, 100 * mm, 30 * mm)
            materials: [ PrincipledMaterial { baseColor: "#ecf0f1"; metalness: 0.9 } ]
        }
        
        // Grip Pad
        Model {
            position: Qt.vector3d(10 * 0.001, 70 * 0.001, 0)
            source: "#Cube"
            scale: Qt.vector3d(5 * mm, 40 * mm, 25 * mm)
            materials: [ PrincipledMaterial { baseColor: "#111" } ]
        }
    }

    // 3. RIGHT FINGER (Mirror)
    Node {
        id: rightFinger
        position: Qt.vector3d((root.gripped ? 15 : 45) * 0.001, 80 * 0.001, 0)

        Behavior on position.x {
            NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
        }

        Model {
            position: Qt.vector3d(0, 40 * 0.001, 0)
            source: "#Cube"
            scale: Qt.vector3d(20 * mm, 100 * mm, 30 * mm)
            materials: [ PrincipledMaterial { baseColor: "#ecf0f1"; metalness: 0.9 } ]
        }

        Model {
            position: Qt.vector3d(-10 * 0.001, 70 * 0.001, 0)
            source: "#Cube"
            scale: Qt.vector3d(5 * mm, 40 * mm, 25 * mm)
            materials: [ PrincipledMaterial { baseColor: "#111" } ]
        }
    }
}