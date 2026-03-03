import QtQuick
import QtQuick.Controls
import QtQuick3D

Item {
    id: root
    property var backend: null
    property bool entryDamperOpen: false
    property bool exitDamperOpen: false
    property bool doorOpen: false

    View3D {
        id: view
        anchors.fill: parent
        camera: sceneCamera

        environment: SceneEnvironment {
            clearColor: "#f0f2f5"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        Node {
            id: cameraPivot
            eulerRotation: Qt.vector3d(-30, 40, 0)
            position: Qt.vector3d(600, 450, 0)

            PerspectiveCamera {
                id: sceneCamera
                position: Qt.vector3d(0, 0, 5200)
                clipNear: 10
                clipFar: 30000
            }
        }

        DirectionalLight {
            eulerRotation.x: -35
            eulerRotation.y: -30
            brightness: 1.4
            castsShadow: true
            shadowMapFar: 12000
            shadowMapQuality: DirectionalLight.ShadowMapQualityVeryHigh
        }

        PointLight {
            position: Qt.vector3d(1800, 1800, 1800)
            brightness: 0.7
        }

        Model {
            y: -1
            source: "#Rectangle"
            scale: Qt.vector3d(150, 150, 1)
            eulerRotation.x: -90
            materials: [
                DefaultMaterial {
                    diffuseColor: "#e0e0e0"
                }
            ]
        }

        FenceModel {
            id: fence
            position: Qt.vector3d(500, 0, 0)
            entryDamperOpen: root.entryDamperOpen
            exitDamperOpen: root.exitDamperOpen
            doorOpen: root.doorOpen
        }

        ConveyorModel {
            id: entryConveyor
            position: Qt.vector3d(-2000, 0, 0)
            length: 1875
            height: 700
            conveyorController: root.backend ? root.backend.entryConveyor : null
            sensorPositions: [437.5, 1000.0, 1775.0]
        }

        RobotModel {
            id: plantRobot
            position: Qt.vector3d(0, 0, 1000)
            eulerRotation.z: 90

            axis1: root.backend ? root.backend.robot.axis1 : 0
            axis2: root.backend ? root.backend.robot.axis2 : -90
            axis3: root.backend ? root.backend.robot.axis3 : 90
            axis4: root.backend ? root.backend.robot.axis4 : 0
            axis5: root.backend ? root.backend.robot.axis5 : 0
            axis6: root.backend ? root.backend.robot.axis6 : 0
            gripperGripped: root.backend ? root.backend.robot.gripperGripped : false
        }

        ConveyorModel {
            id: exitConveyor
            position: Qt.vector3d(1750, 0, 0)
            length: 1250
            height: 700
            conveyorController: root.backend ? root.backend.exitConveyor : null
            sensorPositions: [250.0, 700.0, 1150.0]
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
