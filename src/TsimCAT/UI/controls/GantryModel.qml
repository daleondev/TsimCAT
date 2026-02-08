import QtQuick
import QtQuick3D

Node {
    id: gantryRoot

    // Movement Properties (in mm)
    property real xPos: 0    // Translation along the main rail (horizontal)
    property real yLift: 0   // Vertical lift position (0 is top/retracted)

    // Structural Dimensions
    readonly property real horizontalLength: 3000
    readonly property real verticalLength: 1200
    readonly property real bridgeHeight: 2000
    
    // Colors based on the image
    readonly property color profileColor: "#555555"
    readonly property color railColor: "#bbbbbb"
    readonly property color carriageColor: "#d0d0d0"
    readonly property color motorColor: "#1a1a1a"
    readonly property color cableCarrierColor: "#333333"

    // --- SUPPORT STRUCTURE ---
    Node {
        id: supports
        
        // Two sturdy support pillars
        Repeater3D {
            model: 2
            delegate: Model {
                required property int index
                position: Qt.vector3d((index === 0 ? -1 : 1) * (gantryRoot.horizontalLength / 2 - 100), gantryRoot.bridgeHeight / 2, 0)
                source: "#Cube"
                scale: Qt.vector3d(1.5, gantryRoot.bridgeHeight / 100, 1.5)
                materials: [ PrincipledMaterial { baseColor: "#2c3e50"; metalness: 0.8 } ]
            }
        }
    }

    // Component for a Linear Actuator Module
    component LinearActuator: Node {
        property real length: 1000
        property bool horizontal: true
        
        // Base Profile
        Model {
            source: "#Cube"
            scale: horizontal ? Qt.vector3d(length / 100, 0.6, 1.0) : Qt.vector3d(0.8, length / 100, 0.8)
            materials: [ PrincipledMaterial { baseColor: gantryRoot.profileColor; metalness: 0.6; roughness: 0.3 } ]
        }

        // Guide Rails
        Model {
            position: horizontal ? Qt.vector3d(0, 30, -30) : Qt.vector3d(40, 0, 0)
            source: "#Cube"
            scale: horizontal ? Qt.vector3d(length / 100, 0.1, 0.1) : Qt.vector3d(0.1, length / 100, 0.1)
            materials: [ PrincipledMaterial { baseColor: gantryRoot.railColor; metalness: 1.0; roughness: 0.1 } ]
        }
        Model {
            position: horizontal ? Qt.vector3d(0, 30, 30) : Qt.vector3d(-40, 0, 0)
            source: "#Cube"
            scale: horizontal ? Qt.vector3d(length / 100, 0.1, 0.1) : Qt.vector3d(0.1, length / 100, 0.1)
            materials: [ PrincipledMaterial { baseColor: gantryRoot.railColor; metalness: 1.0; roughness: 0.1 } ]
        }
    }

    // 1. HORIZONTAL AXIS (MAIN RAIL)
    Node {
        id: horizontalAxis
        y: gantryRoot.bridgeHeight
        
        LinearActuator {
            length: gantryRoot.horizontalLength
            horizontal: true
        }

        // Support Mounting Plates
        Repeater3D {
            model: 2
            delegate: Model {
                required property int index
                position: Qt.vector3d((index === 0 ? -1 : 1) * (gantryRoot.horizontalLength / 2 - 100), -40, 0)
                source: "#Cube"
                scale: Qt.vector3d(2, 0.2, 1.2)
                materials: [ PrincipledMaterial { baseColor: "#333"; metalness: 0.5 } ]
            }
        }

        // Main Carriage (X-Axis)
        Node {
            id: xCarriage
            x: gantryRoot.xPos - (gantryRoot.horizontalLength / 2) + 500 // Offset so 0 is a valid start point

            Model {
                position: Qt.vector3d(0, 40, 0)
                source: "#Cube"
                scale: Qt.vector3d(2.5, 0.4, 1.4)
                materials: [ PrincipledMaterial { baseColor: gantryRoot.carriageColor } ]
            }

            // X-Axis Motor (Mounted on carriage in the image)
            Node {
                position: Qt.vector3d(-80, 150, 0)
                Model {
                    source: "#Cube"
                    scale: Qt.vector3d(1.2, 1.8, 1.2)
                    materials: [ PrincipledMaterial { baseColor: gantryRoot.motorColor; roughness: 0.5 } ]
                }
                Model {
                    position: Qt.vector3d(0, -100, 0)
                    source: "#Cylinder"
                    scale: Qt.vector3d(0.01, 0.005, 0.01)
                    materials: [ PrincipledMaterial { baseColor: "#444" } ]
                }
            }

            // 2. VERTICAL AXIS (ATTACHED TO X CARRIAGE)
            Node {
                id: verticalAxis
                position: Qt.vector3d(150, 400, 0)

                LinearActuator {
                    length: gantryRoot.verticalLength
                    horizontal: false
                }

                // Vertical Motor (at the top of vertical rail)
                Node {
                    position: Qt.vector3d(0, gantryRoot.verticalLength / 2 + 100, 0)
                    Model {
                        source: "#Cube"
                        scale: Qt.vector3d(1.0, 1.5, 1.0)
                        materials: [ PrincipledMaterial { baseColor: gantryRoot.motorColor } ]
                    }
                }

                // Lift Carriage (Y-Axis)
                Node {
                    id: yCarriage
                    y: -gantryRoot.yLift + (gantryRoot.verticalLength / 2) - 200

                    Model {
                        source: "#Cube"
                        scale: Qt.vector3d(1.2, 2.0, 1.2)
                        materials: [ PrincipledMaterial { baseColor: gantryRoot.carriageColor } ]
                    }

                    // Tool Mounting Plate
                    Model {
                        position: Qt.vector3d(0, -120, 0)
                        source: "#Cube"
                        scale: Qt.vector3d(1.5, 0.2, 1.5)
                        materials: [ PrincipledMaterial { baseColor: "#222" } ]
                    }

                    // GRIPPER
                    GripperModel {
                        position: Qt.vector3d(0, -150, 0)
                        scale: Qt.vector3d(1000, 1000, 1000)
                    }
                }

                // Cable Carrier (E-Chain)
                Node {
                    id: cableCarrier
                    position: Qt.vector3d(80, 0, 0)
                    
                    // Stationary part
                    Model {
                        position: Qt.vector3d(0, -gantryRoot.verticalLength / 4, 0)
                        source: "#Cube"
                        scale: Qt.vector3d(0.2, gantryRoot.verticalLength / 200, 0.4)
                        materials: [ PrincipledMaterial { baseColor: gantryRoot.cableCarrierColor } ]
                    }

                    // Curved part (Placeholder using a cylinder or arc)
                    Model {
                        position: Qt.vector3d(0, yCarriage.y + 100, 0)
                        source: "#Cylinder"
                        scale: Qt.vector3d(0.004, 0.002, 0.004)
                        eulerRotation.z: 90
                        materials: [ PrincipledMaterial { baseColor: gantryRoot.cableCarrierColor } ]
                    }
                }
            }
        }
    }
}
