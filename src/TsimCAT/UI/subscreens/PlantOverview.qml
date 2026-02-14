import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls" as Controls

Control {
    id: root
    property string title: "Plant Overview"
    property var backend: null
    property bool showControls: false

    padding: 0

    // Helper to safely access robot properties
    readonly property var robot: backend ? backend.robot : null

    function updateJoints(index, value) {
        if (!robot)
            return;
        let joints = [robot.axis1, robot.axis2, robot.axis3, robot.axis4, robot.axis5, robot.axis6];
        joints[index] = value;
        robot.setJoints(joints[0], joints[1], joints[2], joints[3], joints[4], joints[5]);
    }

    function updateTcp(axis, value) {
        if (!robot)
            return;
        let x = robot.tcpX, y = robot.tcpY, z = robot.tcpZ;
        let r = robot.tcpRoll, p = robot.tcpPitch, w = robot.tcpYaw;
        if (axis === "x")
            x = value;
        else if (axis === "y")
            y = value;
        else if (axis === "z")
            z = value;
        else if (axis === "r")
            r = value;
        else if (axis === "p")
            p = value;
        else if (axis === "w")
            w = value;
        robot.setTcp(x, y, z, r, p, w);
    }

    contentItem: Item {
        // --- 3D SCENE ---
        Controls.Plant3DView {
            id: plantView
            anchors.fill: parent
            backend: root.backend
            entryDamperOpen: (root.backend && root.backend.entryConveyor.autoLogic) ? root.backend.entryConveyor.damperOpen : entryDamperToggle.checked
            exitDamperOpen: (root.backend && root.backend.transferConveyor.autoLogic) ? root.backend.transferConveyor.damperOpen : exitDamperToggle.checked
            doorOpen: doorToggle.checked
            gantryX: (root.backend && root.backend.gantry) ? root.backend.gantry.xPos : gantryXSlider.value
            gantryZ: (root.backend && root.backend.gantry) ? root.backend.gantry.zPos : gantryZSlider.value
        }

        // --- OVERLAYS ---

        // Header
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

                    PropertyAnimation on opacity {
                        from: 1.0
                        to: 0.4
                        duration: 1000
                        loops: Animation.Infinite
                        running: true
                    }
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
                        text: "SYSTEM OPERATIONAL"
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
            }
        }

        // Control Panel Overlay
        Rectangle {
            id: controlPanel
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 20
            width: 300
            color: "#ee1c1c1c"
            radius: 10
            visible: root.showControls
            z: 10

            ScrollView {
                anchors.fill: parent
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                ColumnLayout {
                    width: parent.width - 40
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.margins: 20
                    spacing: 15

                    Text {
                        text: "Manual Controls"
                        color: "white"
                        font.pixelSize: 18
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    CheckBox {
                        id: entryDamperToggle
                        text: ""
                        indicator.width: 20
                        indicator.height: 20
                        enabled: !(root.backend && root.backend.entryConveyor.autoLogic)
                        checked: (root.backend && root.backend.entryConveyor.autoLogic) ? root.backend.entryConveyor.damperOpen : false
                        Label {
                            text: "Guillotine Damper"
                            color: "white"
                            anchors.left: parent.right
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    CheckBox {
                        id: exitDamperToggle
                        text: ""
                        indicator.width: 20
                        indicator.height: 20
                        enabled: !(root.backend && root.backend.transferConveyor.autoLogic)
                        checked: (root.backend && root.backend.transferConveyor.autoLogic) ? root.backend.transferConveyor.damperOpen : false
                        Label {
                            text: "Exit Guillotine Damper"
                            color: "white"
                            anchors.left: parent.right
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    CheckBox {
                        id: doorToggle
                        text: ""
                        indicator.width: 20
                        indicator.height: 20
                        Label {
                            text: "Safety Door"
                            color: "white"
                            anchors.left: parent.right
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    Text {
                        text: "Conveyor Simulation"
                        color: "#a0aec0"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Text {
                        text: "Internal Cell Flow"
                        color: "#a0aec0"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Switch {
                        text: "Robot local simulation"
                        checked: root.backend ? root.backend.localRobotMode : false
                        onToggled: if (root.backend)
                            root.backend.localRobotMode = checked
                    }

                    Switch {
                        text: "Laser local simulation"
                        checked: root.backend ? root.backend.localLaserMode : false
                        onToggled: if (root.backend)
                            root.backend.localLaserMode = checked
                    }

                    Switch {
                        text: "Entry conveyor local simulation"
                        checked: root.backend ? root.backend.localEntryConveyorMode : false
                        onToggled: if (root.backend)
                            root.backend.localEntryConveyorMode = checked
                    }

                    Switch {
                        text: "Exit conveyor local simulation"
                        checked: root.backend ? root.backend.localExitConveyorMode : false
                        onToggled: if (root.backend)
                            root.backend.localExitConveyorMode = checked
                    }

                    Switch {
                        text: "Gantry local simulation"
                        checked: root.backend ? root.backend.localGantryMode : false
                        onToggled: if (root.backend)
                            root.backend.localGantryMode = checked
                    }

                    Switch {
                        id: internalFlowSwitch
                        text: "Run full local sequence"
                        checked: root.backend ? root.backend.internalCellFlowRunning : false
                        enabled: root.backend ? (root.backend.localRobotMode && root.backend.localLaserMode && root.backend.localEntryConveyorMode && root.backend.localExitConveyorMode) : false
                        onToggled: {
                            if (!root.backend)
                                return;
                            if (checked) {
                                root.backend.startInternalCellFlow();
                            } else {
                                root.backend.stopInternalCellFlow();
                            }
                        }
                    }

                    Text {
                        text: root.backend ? ("State: " + root.backend.internalCellFlowStatus) : "State: N/A"
                        color: "white"
                        font.pixelSize: 11
                    }

                    Text {
                        text: root.backend ? ("Modes  R:" + (root.backend.localRobotMode ? "L" : "E") + "  L:" + (root.backend.localLaserMode ? "L" : "E") + "  C1:" + (root.backend.localEntryConveyorMode ? "L" : "E") + "  C2:" + (root.backend.localExitConveyorMode ? "L" : "E")) : "Modes: N/A"
                        color: "#cbd5e1"
                        font.pixelSize: 10
                    }

                    CheckBox {
                        id: autoLogicToggle
                        text: ""
                        checked: root.backend ? root.backend.entryConveyor.autoLogic : false
                        onToggled: if (root.backend)
                            root.backend.entryConveyor.autoLogic = checked
                        Label {
                            text: "Independent Seq. Logic"
                            color: "#3498db"
                            font.bold: true
                            anchors.left: parent.right
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    CheckBox {
                        id: autoLogicTransferToggle
                        text: ""
                        checked: root.backend ? root.backend.transferConveyor.autoLogic : false
                        onToggled: if (root.backend)
                            root.backend.transferConveyor.autoLogic = checked
                        Label {
                            text: "Transfer Conveyor Auto Logic"
                            color: "#3498db"
                            font.bold: true
                            anchors.left: parent.right
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    CheckBox {
                        id: beltRunToggle
                        text: ""
                        enabled: !autoLogicToggle.checked
                        checked: root.backend ? root.backend.entryConveyor.isRunning : true
                        onToggled: if (root.backend)
                            root.backend.entryConveyor.isRunning = checked
                        Label {
                            text: "Entry Belt Run"
                            color: "white"
                            anchors.left: parent.right
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    CheckBox {
                        id: autoSpawnToggle
                        text: ""
                        checked: root.backend ? root.backend.entryConveyor.autoSpawn : false
                        onToggled: if (root.backend)
                            root.backend.entryConveyor.autoSpawn = checked
                        Label {
                            text: "Auto-Spawn Parts"
                            color: "white"
                            anchors.left: parent.right
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    CheckBox {
                        id: gantryAutoToggle
                        text: ""
                        checked: root.backend ? root.backend.gantry.autoTransfer : false
                        onToggled: if (root.backend)
                            root.backend.gantry.autoTransfer = checked
                        Label {
                            text: "Gantry Auto Transfer"
                            color: "white"
                            anchors.left: parent.right
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Button {
                            text: "Spawn Type 1"
                            Layout.fillWidth: true
                            onClicked: if (root.backend)
                                root.backend.entryConveyor.spawnPart(1)
                        }
                        Button {
                            text: "Spawn Type 2"
                            Layout.fillWidth: true
                            onClicked: if (root.backend)
                                root.backend.entryConveyor.spawnPart(2)
                        }
                    }

                    Button {
                        text: "Clear All Parts"
                        Layout.fillWidth: true
                        palette.button: "#e74c3c"
                        palette.buttonText: "white"
                        onClicked: {
                            if (root.backend) {
                                root.backend.entryConveyor.clearParts();
                                root.backend.exitConveyor.clearParts();
                                root.backend.transferConveyor.clearParts();
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    Text {
                        text: "Gantry Axis (mm)"
                        color: "#a0aec0"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    ColumnLayout {
                        spacing: 15
                        Layout.fillWidth: true
                        EditableValue {
                            id: gantryXSlider
                            label: "X-Axis"
                            value: 0
                            min: -800
                            max: 800
                            Layout.fillWidth: true
                            onUpdated: v => value = v
                        }
                        EditableValue {
                            id: gantryZSlider
                            label: "Z-Axis"
                            value: 80
                            min: 0
                            max: 300
                            Layout.fillWidth: true
                            onUpdated: v => value = v
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    CheckBox {
                        id: gripperToggle
                        text: ""
                        indicator.width: 20
                        indicator.height: 20
                        checked: root.robot ? root.robot.gripperGripped : false
                        onToggled: {
                            if (root.robot) {
                                root.robot.gripperGripped = checked;
                            }
                        }
                        Label {
                            text: "Robot Gripper"
                            color: "white"
                            anchors.left: parent.right
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    Text {
                        text: "Robot Joints (Deg)"
                        color: "#a0aec0"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    ColumnLayout {
                        spacing: 15
                        Layout.fillWidth: true
                        EditableValue {
                            label: "A1"
                            value: root.robot ? root.robot.axis1 : 0
                            min: -180
                            max: 180
                            Layout.fillWidth: true
                            onUpdated: v => root.updateJoints(0, v)
                        }
                        EditableValue {
                            label: "A2"
                            value: root.robot ? root.robot.axis2 : 0
                            min: -180
                            max: 180
                            Layout.fillWidth: true
                            onUpdated: v => root.updateJoints(1, v)
                        }
                        EditableValue {
                            label: "A3"
                            value: root.robot ? root.robot.axis3 : 0
                            min: -180
                            max: 180
                            Layout.fillWidth: true
                            onUpdated: v => root.updateJoints(2, v)
                        }
                        EditableValue {
                            label: "A4"
                            value: root.robot ? root.robot.axis4 : 0
                            min: -180
                            max: 180
                            Layout.fillWidth: true
                            onUpdated: v => root.updateJoints(3, v)
                        }
                        EditableValue {
                            label: "A5"
                            value: root.robot ? root.robot.axis5 : 0
                            min: -180
                            max: 180
                            Layout.fillWidth: true
                            onUpdated: v => root.updateJoints(4, v)
                        }
                        EditableValue {
                            label: "A6"
                            value: root.robot ? root.robot.axis6 : 0
                            min: -360
                            max: 360
                            Layout.fillWidth: true
                            onUpdated: v => root.updateJoints(5, v)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    Text {
                        text: "Robot TCP (mm/deg)"
                        color: "#a0aec0"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    ColumnLayout {
                        spacing: 15
                        Layout.fillWidth: true
                        EditableValue {
                            label: "X"
                            value: root.robot ? root.robot.tcpX : 0
                            min: -2000
                            max: 2000
                            Layout.fillWidth: true
                            onUpdated: v => root.updateTcp("x", v)
                        }
                        EditableValue {
                            label: "Y"
                            value: root.robot ? root.robot.tcpY : 0
                            min: -2000
                            max: 2000
                            Layout.fillWidth: true
                            onUpdated: v => root.updateTcp("y", v)
                        }
                        EditableValue {
                            label: "Z"
                            value: root.robot ? root.robot.tcpZ : 0
                            min: 0
                            max: 3000
                            Layout.fillWidth: true
                            onUpdated: v => root.updateTcp("z", v)
                        }
                        EditableValue {
                            label: "R"
                            value: root.robot ? root.robot.tcpRoll : 0
                            min: -180
                            max: 180
                            Layout.fillWidth: true
                            onUpdated: v => root.updateTcp("r", v)
                        }
                        EditableValue {
                            label: "P"
                            value: root.robot ? root.robot.tcpPitch : 0
                            min: -180
                            max: 180
                            Layout.fillWidth: true
                            onUpdated: v => root.updateTcp("p", v)
                        }
                        EditableValue {
                            label: "W"
                            value: root.robot ? root.robot.tcpYaw : 0
                            min: -180
                            max: 180
                            Layout.fillWidth: true
                            onUpdated: v => root.updateTcp("w", v)
                        }
                    }

                    Item {
                        Layout.preferredHeight: 20
                    } // Bottom spacer
                }
            }
        }

        // Legend / Help
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 20
            color: "#aa000000"
            radius: 6
            width: legendText.width + 20
            height: legendText.height + 10
            visible: !root.showControls

            Text {
                id: legendText
                anchors.centerIn: parent
                text: "Right Click: Orbit | Left Click: Pan | Scroll: Zoom"
                color: "#cccccc"
                font.pixelSize: 11
            }
        }
    }

    component EditableValue: Rectangle {
        id: editableValueRoot
        property string label: ""
        property var value: 0
        property real min: 0
        property real max: 100
        signal updated(var newValue)

        height: 65
        color: "#2a2a2a"
        border.color: input.activeFocus ? "#3498db" : "#3a3a3a"
        radius: 6

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 2

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: editableValueRoot.label
                    font.pixelSize: 9
                    font.bold: true
                    color: "#a0aec0"
                }
                Item {
                    Layout.fillWidth: true
                }
                TextInput {
                    id: input
                    text: Number(editableValueRoot.value).toFixed(2)
                    font.family: "Monospace"
                    font.pixelSize: 11
                    font.bold: true
                    color: "white"
                    selectByMouse: true
                    horizontalAlignment: Text.AlignRight
                    onAccepted: {
                        editableValueRoot.updated(parseFloat(text));
                        focus = false;
                    }
                    Connections {
                        target: editableValueRoot
                        function onValueChanged() {
                            if (!input.activeFocus) {
                                input.text = Number(editableValueRoot.value).toFixed(2);
                            }
                        }
                    }
                }
            }

            Slider {
                Layout.fillWidth: true
                Layout.preferredHeight: 20
                from: editableValueRoot.min
                to: editableValueRoot.max
                value: editableValueRoot.value
                onMoved: editableValueRoot.updated(value)

                background: Rectangle {
                    x: parent.leftPadding
                    y: parent.topPadding + parent.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 4
                    width: parent.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: "#444"

                    Rectangle {
                        width: parent.visualPosition * parent.width
                        height: parent.height
                        color: "#3498db"
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                    y: parent.topPadding + parent.availableHeight / 2 - height / 2
                    implicitWidth: 12
                    implicitHeight: 12
                    radius: 6
                    color: parent.pressed ? "#f0f0f0" : "#ffffff"
                    border.color: "#3498db"
                }
            }
        }
    }
}
