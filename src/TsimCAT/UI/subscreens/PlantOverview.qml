import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls" as Controls

Control {
    id: root
    property string title: "Plant Overview"
    property var backend: null
    property bool showControls: false
    property bool showRobotManual: false
    property int manualRobotMode: 0

    readonly property var robot: backend ? backend.robot : null

    padding: 0

    function updateJoint(index, value) {
        if (!robot)
            return;

        let joints = [robot.axis1, robot.axis2, robot.axis3, robot.axis4, robot.axis5, robot.axis6];
        joints[index] = value;
        robot.setJoints(joints[0], joints[1], joints[2], joints[3], joints[4], joints[5]);
    }

    function updateTcp(axis, value) {
        if (!robot)
            return;

        let x = robot.tcpX;
        let y = robot.tcpY;
        let z = robot.tcpZ;
        let roll = robot.tcpRoll;
        let pitch = robot.tcpPitch;
        let yaw = robot.tcpYaw;

        if (axis === "x")
            x = value;
        else if (axis === "y")
            y = value;
        else if (axis === "z")
            z = value;
        else if (axis === "roll")
            roll = value;
        else if (axis === "pitch")
            pitch = value;
        else if (axis === "yaw")
            yaw = value;

        robot.setTcp(x, y, z, roll, pitch, yaw);
    }

    contentItem: Item {
        Controls.Plant3DView {
            id: plantView
            anchors.fill: parent
            backend: root.backend
            exitDamperOpen: root.backend ? root.backend.exitConveyor.damperOpen : false
        }

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 20
            color: "#aa000000"
            radius: 8
            width: headerLayout.width + 30
            height: headerLayout.height + 20
            z: 10

            RowLayout {
                id: headerLayout
                anchors.centerIn: parent
                spacing: 15

                Rectangle {
                    Layout.preferredWidth: 12
                    Layout.preferredHeight: 12
                    radius: 6
                    color: "#2ecc71"
                }

                ColumnLayout {
                    spacing: 0
                    Text {
                        text: root.title
                        font.pixelSize: 18
                        font.bold: true
                        color: "white"
                    }
                    Text {
                        text: root.backend && root.backend.usingLocalAdsShadow ? "SIMPLE CELL / LOCAL ADS SHADOW" : "SIMPLE CELL"
                        font.pixelSize: 10
                        font.bold: true
                        color: "#2ecc71"
                    }
                }

                Button {
                    text: root.showControls ? "Hide Controls" : "Show Controls"
                    flat: true
                    palette.buttonText: "white"
                    onClicked: root.showControls = !root.showControls
                }

                Button {
                    text: root.showRobotManual ? "Hide Robot" : "Manual Robot"
                    flat: true
                    palette.buttonText: "white"
                    onClicked: root.showRobotManual = !root.showRobotManual
                }
            }
        }

        Rectangle {
            id: controlPanel
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 20
            width: 340
            color: "#ee1c1c1c"
            radius: 10
            visible: root.showControls
            z: 10

            ScrollView {
                anchors.fill: parent
                clip: true

                ColumnLayout {
                    width: parent.width - 40
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.margins: 20
                    spacing: 14

                    Text { text: "Cell Controls"; color: "#f4f7f8"; font.pixelSize: 18; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                    Rectangle { Layout.fillWidth: true; radius: 8; color: "#1f2b2f"; implicitHeight: modeColumn.implicitHeight + 20
                        ColumnLayout {
                            id: modeColumn
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6
                            Text { text: "Execution"; color: "#eef5f3"; font.pixelSize: 13; font.bold: true }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: root.backend && root.backend.usingLocalAdsShadow ? "Stations exchange ADS-shaped data through the in-process shadow. Direct station mode bypasses that station's PLC shadow interface." : "External symbolic links are active."
                                color: "#d6e3e0"
                                font.pixelSize: 11
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    Text {
                        text: "Simulation"
                        color: "#e5edf2"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Switch {
                        text: "Enable local simulation"
                        palette.windowText: "#eef5f3"
                        checked: root.backend ? root.backend.localSimulationEnabled : true
                        onToggled: if (root.backend)
                            root.backend.localSimulationEnabled = checked
                    }

                    Switch {
                        text: "Enable table simulation"
                        palette.windowText: "#eef5f3"
                        enabled: root.backend ? root.backend.localSimulationEnabled : false
                        checked: root.backend ? root.backend.localTableSimulationEnabled : true
                        onToggled: if (root.backend)
                            root.backend.localTableSimulationEnabled = checked
                    }

                    Switch {
                        text: "Enable robot simulation"
                        palette.windowText: "#eef5f3"
                        enabled: root.backend ? root.backend.localSimulationEnabled : false
                        checked: root.backend ? root.backend.localRobotSimulationEnabled : true
                        onToggled: if (root.backend)
                            root.backend.localRobotSimulationEnabled = checked
                    }

                    Switch {
                        text: "Enable laser simulation"
                        palette.windowText: "#eef5f3"
                        enabled: root.backend ? root.backend.localSimulationEnabled : false
                        checked: root.backend ? root.backend.localLaserSimulationEnabled : true
                        onToggled: if (root.backend)
                            root.backend.localLaserSimulationEnabled = checked
                    }

                    Switch {
                        text: "Enable conveyor simulation"
                        palette.windowText: "#eef5f3"
                        enabled: root.backend ? root.backend.localSimulationEnabled : false
                        checked: root.backend ? root.backend.localConveyorSimulationEnabled : true
                        onToggled: if (root.backend)
                            root.backend.localConveyorSimulationEnabled = checked
                    }

                    Switch {
                        text: "Auto spawn parts"
                        palette.windowText: "#eef5f3"
                        enabled: root.backend ? root.backend.localSimulationEnabled : false
                        checked: root.backend ? root.backend.autoSpawnPartsEnabled : true
                        onToggled: if (root.backend)
                            root.backend.autoSpawnPartsEnabled = checked
                    }

                    Button {
                        text: "Spawn part"
                        palette.buttonText: "#eef5f3"
                        Layout.fillWidth: true
                        enabled: root.backend
                                 ? (root.backend.localSimulationEnabled && !root.backend.autoSpawnPartsEnabled && root.backend.rotaryTable && root.backend.rotaryTable.atLoadPosition && !root.backend.rotaryTable.partPresent)
                                 : false
                        onClicked: if (root.backend)
                            root.backend.spawnTablePart()
                    }

                    Switch {
                        text: "Auto despawn parts"
                        palette.windowText: "#eef5f3"
                        enabled: root.backend ? (root.backend.localSimulationEnabled && root.backend.localConveyorSimulationEnabled) : false
                        checked: root.backend ? root.backend.autoDespawnPartsEnabled : true
                        onToggled: if (root.backend)
                            root.backend.autoDespawnPartsEnabled = checked
                    }

                    Button {
                        text: "Despawn part"
                        palette.buttonText: "#eef5f3"
                        Layout.fillWidth: true
                        enabled: root.backend
                                 ? (root.backend.localSimulationEnabled && !root.backend.autoDespawnPartsEnabled && root.backend.exitConveyor && root.backend.exitConveyor.partAtEndSensor)
                                 : false
                        onClicked: if (root.backend)
                            root.backend.despawnExitPart()
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    Text {
                        text: "Stations"
                        color: "#e5edf2"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Text {
                        text: root.backend && root.backend.rotaryTable
                              ? "Rotary angle: " + Number(root.backend.rotaryTable.angleDegrees).toFixed(1) + " deg"
                              : "Rotary angle: 0.0 deg"
                        color: "#edf3f6"
                        font.pixelSize: 11
                    }

                    Text {
                        text: root.backend && root.backend.rotaryTable && root.backend.rotaryTable.readyToPick
                              ? "Rotary table ready for pick"
                              : "Rotary table staging"
                        color: root.backend && root.backend.rotaryTable && root.backend.rotaryTable.readyToPick ? "#93f0b2" : "#f0cd83"
                        font.pixelSize: 11
                    }

                    Text {
                        text: root.backend && root.backend.exitConveyor
                              ? "Exit parts: " + root.backend.exitConveyor.parts.length
                              : "Exit parts: 0"
                        color: "#edf3f6"
                        font.pixelSize: 11
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    Text {
                        text: "Debugging"
                        color: "#e5edf2"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Button {
                        text: "Capture Snapshot"
                        palette.buttonText: "#eef5f3"
                        Layout.fillWidth: true
                        enabled: root.backend && root.backend.screenshotProvider
                        onClicked: if (root.backend && root.backend.screenshotProvider)
                            root.backend.screenshotProvider.requestCapture()
                    }

                    Switch {
                        text: "Auto capture"
                        palette.windowText: "#eef5f3"
                        checked: root.backend && root.backend.screenshotProvider ? root.backend.screenshotProvider.autoCaptureEnabled : false
                        onToggled: if (root.backend && root.backend.screenshotProvider)
                            root.backend.screenshotProvider.autoCaptureEnabled = checked
                    }

                    Text {
                        visible: root.backend && root.backend.screenshotProvider
                        text: root.backend && root.backend.screenshotProvider
                              ? "Captures: " + root.backend.screenshotProvider.captureCount
                              : "Captures: 0"
                        color: "#a0b0b8"
                        font.pixelSize: 10
                    }
                }
            }
        }

        Rectangle {
            id: robotManualPanel
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 20
            anchors.topMargin: 140
            anchors.bottomMargin: 20
            width: 420
            color: "#ee1b1d20"
            radius: 10
            visible: root.showRobotManual
            z: 10
            opacity: root.backend && root.backend.localSimulationEnabled && !root.backend.localRobotSimulationEnabled ? 1.0 : 0.72

            ScrollView {
                anchors.fill: parent
                anchors.margins: 16
                clip: true

                ColumnLayout {
                    width: robotManualPanel.width - 48
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: "Robot Manual"
                            color: "white"
                            font.pixelSize: 18
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            text: "Close"
                            flat: true
                            palette.buttonText: "white"
                            onClicked: root.showRobotManual = false
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: root.backend && root.backend.localSimulationEnabled && !root.backend.localRobotSimulationEnabled
                              ? "Use sliders to set the simulated robot joints or TCP pose directly."
                            : "Enable local simulation and disable robot simulation to drive the robot manually."
                        color: "#d6e3e0"
                        font.pixelSize: 11
                    }

                    TabBar {
                        id: manualModeTabs
                        Layout.fillWidth: true
                        currentIndex: root.manualRobotMode
                        onCurrentIndexChanged: root.manualRobotMode = currentIndex

                        TabButton { text: "Joints" }
                        TabButton { text: "Cartesian" }
                    }

                    StackLayout {
                        Layout.fillWidth: true
                        currentIndex: root.manualRobotMode
                        enabled: root.backend && root.backend.localSimulationEnabled && !root.backend.localRobotSimulationEnabled

                        ColumnLayout {
                            spacing: 8

                            ManualSlider {
                                label: "A1"
                                from: -180
                                to: 180
                                currentValue: root.robot ? root.robot.axis1 : 0
                                onCommitted: function(value) { root.updateJoint(0, value) }
                            }
                            ManualSlider {
                                label: "A2"
                                from: -150
                                to: 150
                                currentValue: root.robot ? root.robot.axis2 : 0
                                onCommitted: function(value) { root.updateJoint(1, value) }
                            }
                            ManualSlider {
                                label: "A3"
                                from: -150
                                to: 150
                                currentValue: root.robot ? root.robot.axis3 : 0
                                onCommitted: function(value) { root.updateJoint(2, value) }
                            }
                            ManualSlider {
                                label: "A4"
                                from: -180
                                to: 180
                                currentValue: root.robot ? root.robot.axis4 : 0
                                onCommitted: function(value) { root.updateJoint(3, value) }
                            }
                            ManualSlider {
                                label: "A5"
                                from: -125
                                to: 125
                                currentValue: root.robot ? root.robot.axis5 : 0
                                onCommitted: function(value) { root.updateJoint(4, value) }
                            }
                            ManualSlider {
                                label: "A6"
                                from: -180
                                to: 180
                                currentValue: root.robot ? root.robot.axis6 : 0
                                onCommitted: function(value) { root.updateJoint(5, value) }
                            }
                        }

                        ColumnLayout {
                            spacing: 8

                            ManualSlider {
                                label: "X"
                                from: 0
                                to: 1500
                                currentValue: root.robot ? root.robot.tcpX : 0
                                onCommitted: function(value) { root.updateTcp("x", value) }
                            }
                            ManualSlider {
                                label: "Y"
                                from: -1200
                                to: 1200
                                currentValue: root.robot ? root.robot.tcpY : 0
                                onCommitted: function(value) { root.updateTcp("y", value) }
                            }
                            ManualSlider {
                                label: "Z"
                                from: 200
                                to: 1500
                                currentValue: root.robot ? root.robot.tcpZ : 0
                                onCommitted: function(value) { root.updateTcp("z", value) }
                            }
                            ManualSlider {
                                label: "Roll"
                                from: -180
                                to: 180
                                currentValue: root.robot ? root.robot.tcpRoll : 0
                                onCommitted: function(value) { root.updateTcp("roll", value) }
                            }
                            ManualSlider {
                                label: "Pitch"
                                from: -180
                                to: 180
                                currentValue: root.robot ? root.robot.tcpPitch : 0
                                onCommitted: function(value) { root.updateTcp("pitch", value) }
                            }
                            ManualSlider {
                                label: "Yaw"
                                from: -180
                                to: 180
                                currentValue: root.robot ? root.robot.tcpYaw : 0
                                onCommitted: function(value) { root.updateTcp("yaw", value) }
                            }
                        }
                    }
                }
            }
        }
    }

    component ManualSlider : Rectangle {
        id: manualSliderRoot
        property string label: ""
        property real from: 0
        property real to: 100
        property real currentValue: 0
        signal committed(real value)

        Layout.fillWidth: true
        implicitHeight: 52
        color: "#22272b"
        border.color: "#394047"
        radius: 8

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 2

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: manualSliderRoot.label
                    color: "#d6e4e0"
                    font.pixelSize: 11
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: Number(slider.value).toFixed(1)
                    color: "white"
                    font.pixelSize: 11
                }
            }

            Slider {
                id: slider
                Layout.fillWidth: true
                from: manualSliderRoot.from
                to: manualSliderRoot.to
                value: manualSliderRoot.currentValue

                onMoved: manualSliderRoot.committed(value)

                Connections {
                    target: manualSliderRoot
                    function onCurrentValueChanged() {
                        if (!slider.pressed) {
                            slider.value = manualSliderRoot.currentValue
                        }
                    }
                }
            }
        }
    }
}
