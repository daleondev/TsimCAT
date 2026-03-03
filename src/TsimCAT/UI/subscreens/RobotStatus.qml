import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls" as Controls

Control {
    id: root
    property string title: "Robot Communication Interface"
    required property var backend

    padding: 20

    // Helper to safely access robot properties
    readonly property var robot: backend ? backend.robot : null

    function updateJoints(index, value) {
        if (!robot) return;
        let joints = [robot.axis1, robot.axis2, robot.axis3, robot.axis4, robot.axis5, robot.axis6];
        joints[index] = value;
        robot.setJoints(joints[0], joints[1], joints[2], joints[3], joints[4], joints[5]);
    }

    function updateTcp(axis, value) {
        if (!robot) return;
        let x = robot.tcpX, y = robot.tcpY, z = robot.tcpZ;
        let r = robot.tcpRoll, p = robot.tcpPitch, w = robot.tcpYaw;
        if (axis === "x") x = value;
        else if (axis === "y") y = value;
        else if (axis === "z") z = value;
        else if (axis === "r") r = value;
        else if (axis === "p") p = value;
        else if (axis === "w") w = value;
        robot.setTcp(x, y, z, r, p, w);
    }

    contentItem: RowLayout {
        spacing: 20

        // --- LEFT SIDE: 3D VISUALIZATION ---
        Rectangle {
            id: viewContainer
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 400
            color: "#1e1e1e"
            radius: 12
            clip: true

            Controls.Robot3DView {
                id: robot3d
                anchors.fill: parent
                axis1: root.robot ? root.robot.axis1 : 0
                axis2: root.robot ? root.robot.axis2 : -90
                axis3: root.robot ? root.robot.axis3 : 90
                axis4: root.robot ? root.robot.axis4 : 0
                axis5: root.robot ? root.robot.axis5 : 0
                axis6: root.robot ? root.robot.axis6 : 0
            }

            // Title Overlay
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 15
                color: "#aa000000"
                radius: 6
                width: titleText.width + 20
                height: titleText.height + 10

                Text {
                    id: titleText
                    anchors.centerIn: parent
                    text: root.title
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                }
            }

            // Legend Overlay
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                anchors.margins: 15
                color: "#aa000000"
                radius: 6
                width: legendText.width + 20
                height: legendText.height + 10

                Text {
                    id: legendText
                    anchors.centerIn: parent
                    text: "Right Click: Rotate | Left Click: Pan | Scroll: Zoom"
                    color: "#cccccc"
                    font.pixelSize: 11
                }
            }
        }

        // --- RIGHT SIDE: CONTROL & STATUS SIDEBAR ---
        ColumnLayout {
            id: sidebarContainer
            Layout.preferredWidth: 420
            Layout.fillWidth: false
            Layout.fillHeight: true
            spacing: 15

            // --- PLC CONTROL PANEL ---
            GroupBox {
                title: "PLC COMMANDS"
                Layout.fillWidth: true
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    RowLayout {
                        ValueBox { label: "JOB ID"; value: root.robot ? root.robot.controlJobId : 0; Layout.fillWidth: true }
                        ValueBox { label: "PART TYPE"; value: root.robot ? root.robot.controlPartType : 0; Layout.fillWidth: true }
                    }

                    ValueBox { 
                        label: "AREA FREE (PLC)"
                        value: root.robot ? "0x" + root.robot.areaFreePlc.toString(16).toUpperCase() : "0x00"
                        Layout.fillWidth: true 
                    }

                    RowLayout {
                        spacing: 15
                        BitIndicator { label: "MOVE ENABLE"; active: root.robot ? root.robot.controlMoveEnable : false; activeColor: "#2ecc71" }
                        BitIndicator { label: "RESET"; active: root.robot ? root.robot.controlReset : false; activeColor: "#3498db" }
                    }
                }
            }

            // --- ROBOT STATUS PANEL ---
            GroupBox {
                title: "ROBOT STATUS"
                Layout.fillWidth: true
                // Removed Layout.fillHeight: true to allow more boxes below

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    RowLayout {
                        ValueBox { label: "FEEDBACK ID"; value: root.robot ? root.robot.jobIdFeedback : 0; Layout.fillWidth: true }
                        ValueBox { label: "PART ECHO"; value: root.robot ? root.robot.partTypeMirrored : 0; Layout.fillWidth: true }
                    }

                    // State Grid
                    GridLayout {
                        columns: 2
                        columnSpacing: 10
                        rowSpacing: 10
                        Layout.fillWidth: true

                        BitIndicator { label: "MOTION"; active: root.robot ? root.robot.inMotion : false; activeColor: "#f1c40f"; Layout.fillWidth: true }
                        BitIndicator { label: "HOME"; active: root.robot ? root.robot.inHome : false; activeColor: "#3498db"; Layout.fillWidth: true }
                        BitIndicator { label: "ENABLED"; active: root.robot ? root.robot.enabled : false; Layout.fillWidth: true }
                        BitIndicator { label: "ERROR"; active: root.robot ? root.robot.error : false; activeColor: "#e74c3c"; Layout.fillWidth: true }
                    }

                    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#eeeeee" }

                    // Quality & Modes
                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        BitIndicator { label: "BRAKE TEST"; active: root.robot ? root.robot.brakeTestOk : false }
                        BitIndicator { label: "MASTERING"; active: root.robot ? root.robot.masteringOk : false }
                        BitIndicator { label: "T1"; active: root.robot ? root.robot.inT1 : false; activeColor: "#9b59b6" }
                        BitIndicator { label: "T2"; active: root.robot ? root.robot.inT2 : false; activeColor: "#8e44ad" }
                        BitIndicator { label: "AUT"; active: root.robot ? root.robot.inAut : false; activeColor: "#2980b9" }
                        BitIndicator { label: "EXT"; active: root.robot ? root.robot.inExt : false; activeColor: "#3498db" }
                    }

                    ValueBox { 
                        label: "ERROR CODE"
                        value: root.robot ? root.robot.errorCode : 0
                        textColor: (root.robot && root.robot.error) ? "#e74c3c" : "#2c3e50"
                        Layout.fillWidth: true 
                    }
                }
            }

            // --- SIMULATED JOBS ---
            GroupBox {
                title: "SIMULATED JOBS"
                Layout.fillWidth: true

                Flow {
                    anchors.fill: parent
                    spacing: 8
                    
                    Button { text: "HOME"; onClicked: if (root.robot) root.robot.triggerJob(1); flat: true }
                    Button { text: "PICK ENTRY"; onClicked: if (root.robot) root.robot.triggerJob(2); flat: true }
                    Button { text: "PLACE EXIT"; onClicked: if (root.robot) root.robot.triggerJob(7); flat: true }
                }
            }

            // --- KINEMATICS CONTROL ---
            GroupBox {
                title: "KINEMATICS"
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    // Tabs or sections for Joints/TCP
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        
                        // Joints Column
                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "JOINTS (DEG)"; font.pixelSize: 10; font.bold: true; color: "#718096" }
                            EditableValue { label: "A1"; value: root.robot ? root.robot.axis1 : 0; Layout.fillWidth: true; onUpdated: v => root.updateJoints(0, v) }
                            EditableValue { label: "A2"; value: root.robot ? root.robot.axis2 : 0; Layout.fillWidth: true; onUpdated: v => root.updateJoints(1, v) }
                            EditableValue { label: "A3"; value: root.robot ? root.robot.axis3 : 0; Layout.fillWidth: true; onUpdated: v => root.updateJoints(2, v) }
                            EditableValue { label: "A4"; value: root.robot ? root.robot.axis4 : 0; Layout.fillWidth: true; onUpdated: v => root.updateJoints(3, v) }
                            EditableValue { label: "A5"; value: root.robot ? root.robot.axis5 : 0; Layout.fillWidth: true; onUpdated: v => root.updateJoints(4, v) }
                            EditableValue { label: "A6"; value: root.robot ? root.robot.axis6 : 0; Layout.fillWidth: true; onUpdated: v => root.updateJoints(5, v) }
                        }

                        // TCP Column
                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "TCP (MM / DEG)"; font.pixelSize: 10; font.bold: true; color: "#718096" }
                            EditableValue { label: "X"; value: root.robot ? root.robot.tcpX : 0; Layout.fillWidth: true; onUpdated: v => root.updateTcp("x", v) }
                            EditableValue { label: "Y"; value: root.robot ? root.robot.tcpY : 0; Layout.fillWidth: true; onUpdated: v => root.updateTcp("y", v) }
                            EditableValue { label: "Z"; value: root.robot ? root.robot.tcpZ : 0; Layout.fillWidth: true; onUpdated: v => root.updateTcp("z", v) }
                            EditableValue { label: "R"; value: root.robot ? root.robot.tcpRoll : 0; Layout.fillWidth: true; onUpdated: v => root.updateTcp("r", v) }
                            EditableValue { label: "P"; value: root.robot ? root.robot.tcpPitch : 0; Layout.fillWidth: true; onUpdated: v => root.updateTcp("p", v) }
                            EditableValue { label: "W"; value: root.robot ? root.robot.tcpYaw : 0; Layout.fillWidth: true; onUpdated: v => root.updateTcp("w", v) }
                        }
                    }
                }
            }

            // --- CONNECTION FOOTER ---
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                color: "#f8f9fa"
                radius: 8
                border.color: "#e9ecef"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 15

                    StatusDot { active: root.robot ? root.robot.adsStatus === "Connected" : false }
                    
                    ColumnLayout {
                        spacing: 2
                        Text { text: "ADS INTERFACE"; font.pixelSize: 9; font.bold: true; color: "#6c757d" }
                        Text { 
                            text: root.robot ? root.robot.adsStatus : "Disconnected"
                            font.pixelSize: 14
                            font.bold: true
                            color: (root.robot && root.robot.adsStatus === "Connected") ? "#2ecc71" : "#e74c3c"
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "RECONNECT"
                        onClicked: if (root.robot) root.robot.connectAds()
                        enabled: root.robot ? root.robot.adsStatus !== "Connected" : false
                        flat: true
                    }
                }
            }
        }
    }

    // --- REUSABLE STYLED COMPONENTS ---

    component ValueBox : Rectangle {
        id: valueBoxRoot
        property string label: ""
        property var value: ""
        property color textColor: "#2c3e50"
        
        height: 45
        color: "#fdfdfd"
        border.color: "#edf2f7"
        radius: 6

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 0
            Text { text: valueBoxRoot.label; font.pixelSize: 9; font.bold: true; color: "#a0aec0" }
            Text { text: valueBoxRoot.value !== undefined ? valueBoxRoot.value : ""; font.family: "Monospace"; font.pixelSize: 16; font.bold: true; color: valueBoxRoot.textColor }
        }
    }

    component EditableValue : Rectangle {
        id: editableValueRoot
        property string label: ""
        property var value: 0
        property string suffix: ""
        signal updated(var newValue)

        height: 40
        color: "#fdfdfd"
        border.color: input.activeFocus ? "#3498db" : "#edf2f7"
        radius: 6

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 0
            Text { text: editableValueRoot.label; font.pixelSize: 8; font.bold: true; color: "#a0aec0" }
            TextInput {
                id: input
                Layout.fillWidth: true
                text: Number(editableValueRoot.value).toFixed(2)
                font.family: "Monospace"
                font.pixelSize: 12
                font.bold: true
                color: "#2c3e50"
                selectByMouse: true
                onAccepted: {
                    editableValueRoot.updated(parseFloat(text))
                    focus = false
                }
                // When not editing, follow the property
                Connections {
                    target: editableValueRoot
                    function onValueChanged() {
                        if (!input.activeFocus) {
                            input.text = Number(editableValueRoot.value).toFixed(2)
                        }
                    }
                }
            }
        }
    }

    component BitIndicator : Rectangle {
        id: bitIndicatorRoot
        property string label: ""
        property bool active: false
        property color activeColor: "#2ecc71"
        
        height: 28
        radius: 4
        implicitWidth: 100
        
        color: active ? activeColor : "#f7fafc"
        border.color: active ? "transparent" : "#e2e8f0"
        
        Behavior on color { ColorAnimation { duration: 200 } }

        Text {
            anchors.centerIn: parent
            text: bitIndicatorRoot.label
            color: bitIndicatorRoot.active ? "white" : "#718096"
            font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.5
        }
    }

    component StatusDot : Rectangle {
        id: statusDotRoot
        property bool active: false
        implicitWidth: 12; implicitHeight: 12; radius: 6
        color: active ? "#2ecc71" : "#e74c3c"
        
        Rectangle {
            anchors.fill: parent
            radius: 6
            color: statusDotRoot.color
            opacity: 0.4
            scale: pulseAnimation.running ? 1.5 : 1.0
            
            PropertyAnimation on scale {
                id: pulseAnimation
                from: 1.0; to: 2.0; duration: 1000
                running: statusDotRoot.active; loops: Animation.Infinite
            }
            PropertyAnimation on opacity {
                from: 0.4; to: 0.0; duration: 1000
                running: statusDotRoot.active; loops: Animation.Infinite
            }
        }
    }
}
