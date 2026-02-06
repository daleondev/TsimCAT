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
                    color: "#ccc"
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
                Layout.fillHeight: true

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

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#eee" }

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

                    Item { Layout.fillHeight: true } // Spacer

                    ValueBox { 
                        label: "ERROR CODE"
                        value: root.robot ? root.robot.errorCode : 0
                        textColor: (root.robot && root.robot.error) ? "#e74c3c" : "#2c3e50"
                        Layout.fillWidth: true 
                    }
                }
            }

            // --- CONNECTION FOOTER ---
            Rectangle {
                Layout.fillWidth: true
                height: 60
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
        width: 12; height: 12; radius: 6
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
