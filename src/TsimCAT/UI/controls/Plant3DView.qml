import QtQuick
import QtQuick.Controls
import QtQuick3D

Item {
    id: root
    property var backend: null
    property bool exitDamperOpen: false

    View3D {
        id: view
        anchors.fill: parent
        camera: sceneCamera

        environment: SceneEnvironment {
            clearColor: "#dbe3e0"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        Node {
            id: cameraPivot
            eulerRotation: Qt.vector3d(-26, 34, 0)
            position: Qt.vector3d(520, 420, 120)

            PerspectiveCamera {
                id: sceneCamera
                position: Qt.vector3d(0, 0, 6100)
                clipNear: 10
                clipFar: 30000
            }
        }

        DirectionalLight {
            eulerRotation.x: -38
            eulerRotation.y: -18
            brightness: 1.55
            castsShadow: true
            shadowMapFar: 12000
            shadowMapQuality: DirectionalLight.ShadowMapQualityVeryHigh
        }

        PointLight {
            position: Qt.vector3d(1200, 2400, 1400)
            brightness: 0.9
            color: "#f2f1df"
        }

        PointLight {
            position: Qt.vector3d(-2200, 1800, -800)
            brightness: 0.45
            color: "#bfd0df"
        }

        Model {
            y: -1
            source: "#Rectangle"
            scale: Qt.vector3d(180, 180, 1)
            eulerRotation.x: -90
            materials: [
                PrincipledMaterial {
                    baseColor: "#c6ccc6"
                    roughness: 0.98
                }
            ]
        }

        Model {
            y: 1
            source: "#Rectangle"
            position: Qt.vector3d(80, 0, 120)
            scale: Qt.vector3d(36, 34, 1)
            eulerRotation.x: -90
            materials: [
                PrincipledMaterial {
                    baseColor: "#b7b49f"
                    roughness: 0.95
                }
            ]
        }

        FenceModel {
            id: fence
            position: Qt.vector3d(150, 0, 120)
            exitDamperOpen: root.exitDamperOpen
        }

        Model {
            position: Qt.vector3d(-1420, 4, 120)
            source: "#Rectangle"
            scale: Qt.vector3d(9, 12, 1)
            eulerRotation.x: -90
            materials: [
                PrincipledMaterial {
                    baseColor: "#83918d"
                    roughness: 0.9
                }
            ]
        }

        Node {
            id: rotaryStation
            position: Qt.vector3d(-1350, 0, 120)

            Model {
                position: Qt.vector3d(0, 18, 0)
                source: "#Cube"
                scale: Qt.vector3d(10.4, 0.36, 9.4)
                materials: [
                    PrincipledMaterial {
                        baseColor: "#616b67"
                        roughness: 0.88
                    }
                ]
            }

            Model {
                position: Qt.vector3d(0, 150, -380)
                source: "#Cube"
                scale: Qt.vector3d(4.2, 2.6, 1.8)
                materials: [
                    PrincipledMaterial {
                        baseColor: "#394148"
                        metalness: 0.35
                        roughness: 0.42
                    }
                ]
            }

            RotaryTableModel {
                angleDegrees: root.backend ? root.backend.rotaryTable.angleDegrees : 0
                partPresent: root.backend ? root.backend.rotaryTable.partPresent : false
                partType: root.backend ? root.backend.rotaryTable.partType : 0
                busy: root.backend ? root.backend.rotaryTable.busy : false
            }
        }

        RobotModel {
            id: plantRobot
            position: Qt.vector3d(0, 0, 350)
            eulerRotation.z: 90

            axis1: root.backend ? root.backend.robot.axis1 : 0
            axis2: root.backend ? root.backend.robot.axis2 : -90
            axis3: root.backend ? root.backend.robot.axis3 : 90
            axis4: root.backend ? root.backend.robot.axis4 : 0
            axis5: root.backend ? root.backend.robot.axis5 : 0
            axis6: root.backend ? root.backend.robot.axis6 : 0
            gripperGripped: root.backend ? root.backend.robot.gripperGripped : false
            carriedPartVisible: root.backend ? root.backend.robotCarriedPartVisible : false
            carriedPartType: root.backend ? root.backend.robotCarriedPartType : 0
        }

        Node {
            id: laserStation
            position: Qt.vector3d(0, 0, -900)

            Model {
                position: Qt.vector3d(0, 15, 0)
                source: "#Cube"
                scale: Qt.vector3d(13.5, 0.3, 10.5)
                materials: [
                    PrincipledMaterial {
                        baseColor: "#616b67"
                        roughness: 0.9
                    }
                ]
            }

            StationModel {
                width: 520
                depth: 520
                height: 820
                color: "#767d82"
            }

            PartModel {
                visible: root.backend ? root.backend.laserPartVisible : false
                position: Qt.vector3d(0, 815, 0)
                width: 140
                length: 140
                height: 80
                color: (root.backend && root.backend.laserPartType === 2) ? "#2ecc71" : "#8f8f8f"
            }

            Node {
                position: Qt.vector3d(0, 0, 0)

                PrincipledMaterial {
                    id: enclosureFrameMaterial
                    baseColor: "#2f3438"
                    metalness: 0.55
                    roughness: 0.34
                }

                PrincipledMaterial {
                    id: enclosureBodyMaterial
                    baseColor: "#d8dbd4"
                    metalness: 0.18
                    roughness: 0.58
                }

                PrincipledMaterial {
                    id: safetyGlassMaterial
                    baseColor: "#99a1a5"
                    opacity: 0.24
                    alphaMode: PrincipledMaterial.Blend
                    metalness: 0.0
                    roughness: 0.2
                    cullMode: PrincipledMaterial.NoCulling
                }

                Model {
                    position: Qt.vector3d(0, 20, 0)
                    source: "#Cube"
                    scale: Qt.vector3d(12.0, 0.4, 9.0)
                    materials: [ enclosureBodyMaterial ]
                }

                Model {
                    position: Qt.vector3d(0, 1860, 0)
                    source: "#Cube"
                    scale: Qt.vector3d(12.0, 0.44, 9.0)
                    materials: [ enclosureBodyMaterial ]
                }

                Repeater3D {
                    model: [
                        Qt.vector3d(-580, 930, -420),
                        Qt.vector3d(580, 930, -420),
                        Qt.vector3d(-580, 930, 420),
                        Qt.vector3d(580, 930, 420)
                    ]

                    delegate: Model {
                        required property vector3d modelData
                        position: modelData
                        source: "#Cube"
                        scale: Qt.vector3d(0.32, 18.6, 0.32)
                        materials: [ enclosureFrameMaterial ]
                    }
                }

                Model {
                    position: Qt.vector3d(0, 930, -420)
                    source: "#Cube"
                    scale: Qt.vector3d(11.6, 0.24, 0.24)
                    materials: [ enclosureFrameMaterial ]
                }

                Model {
                    position: Qt.vector3d(0, 930, 420)
                    source: "#Cube"
                    scale: Qt.vector3d(11.6, 0.24, 0.24)
                    materials: [ enclosureFrameMaterial ]
                }

                Model {
                    position: Qt.vector3d(-580, 930, 0)
                    source: "#Cube"
                    scale: Qt.vector3d(0.24, 0.24, 8.4)
                    materials: [ enclosureFrameMaterial ]
                }

                Model {
                    position: Qt.vector3d(580, 930, 0)
                    source: "#Cube"
                    scale: Qt.vector3d(0.24, 0.24, 8.4)
                    materials: [ enclosureFrameMaterial ]
                }

                Model {
                    position: Qt.vector3d(0, 900, -418)
                    source: "#Cube"
                    scale: Qt.vector3d(10.8, 17.2, 0.03)
                    materials: [ safetyGlassMaterial ]
                }

                Model {
                    position: Qt.vector3d(-578, 900, 0)
                    source: "#Cube"
                    scale: Qt.vector3d(0.03, 17.2, 8.2)
                    materials: [ safetyGlassMaterial ]
                }

                Model {
                    position: Qt.vector3d(578, 900, 0)
                    source: "#Cube"
                    materials: [ safetyGlassMaterial ]
                }

                Model {
                    position: Qt.vector3d(0, 1760, 420)
                    source: "#Cube"
                    scale: Qt.vector3d(7.2, 1.0, 0.28)
                    materials: [ enclosureFrameMaterial ]
                }

                Model {
                    position: Qt.vector3d(0, 1600, 420)
                    source: "#Cube"
                    scale: Qt.vector3d(5.8, 2.8, 0.38)
                    materials: [ enclosureBodyMaterial ]
                }

                Model {
                    position: Qt.vector3d(0, 1860, -120)
                    source: "#Cube"
                    scale: Qt.vector3d(3.2, 0.56, 5.6)
                    materials: [ enclosureBodyMaterial ]
                }

                LaserModel {
                    position: Qt.vector3d(0, 1700, -80)
                    eulerRotation.z: -18
                    laserOn: true
                }

                Model {
                    position: Qt.vector3d(0, 820, 0)
                    source: "#Cube"
                    scale: Qt.vector3d(4.4, 1.1, 4.0)
                    materials: [
                        PrincipledMaterial {
                            baseColor: "#59636a"
                            metalness: 0.32
                            roughness: 0.4
                        }
                    ]
                }
            }
        }

        ConveyorModel {
            id: exitConveyor
            position: Qt.vector3d(1750, 0, 120)
            length: 1250
            height: 720
            width: 430
            conveyorController: root.backend ? root.backend.exitConveyor : null
            sensorPositions: [120.0, 420.0, 760.0, 1120.0]
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton

        property real lastX: 0
        property real lastY: 0

        onPressed: mouse => {
            lastX = mouse.x;
            lastY = mouse.y;
        }

        onPositionChanged: mouse => {
            let dx = mouse.x - lastX;
            let dy = mouse.y - lastY;
            if (mouse.buttons & Qt.RightButton) {
                cameraPivot.eulerRotation.y -= dx * 0.2;
                cameraPivot.eulerRotation.x = Math.max(-90, Math.min(0, cameraPivot.eulerRotation.x - dy * 0.2));
            } else if (mouse.buttons & (Qt.LeftButton | Qt.MiddleButton)) {
                let speed = sceneCamera.position.z / 2000.0;
                let right = sceneCamera.mapDirectionToScene(Qt.vector3d(1, 0, 0));
                let up = sceneCamera.mapDirectionToScene(Qt.vector3d(0, 1, 0));
                let move = right.times(-dx * speed).plus(up.times(dy * speed));
                cameraPivot.position = cameraPivot.position.plus(move);
            }
            lastX = mouse.x;
            lastY = mouse.y;
        }

        onWheel: wheel => {
            let zoomSpeed = sceneCamera.position.z * 0.1;
            sceneCamera.position.z = Math.max(500, Math.min(15000, sceneCamera.position.z - (wheel.angleDelta.y / 120.0) * zoomSpeed));
        }
    }
}
