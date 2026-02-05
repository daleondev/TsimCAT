import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls" as Controls

Control {
    id: root
    property string title: "Robot Communication Interface"
    required property var backend

    ScrollView {
        anchors.fill: parent
        contentWidth: mainLayout.width

        ColumnLayout {
            id: mainLayout
            anchors.centerIn: parent
            width: 900
            spacing: 30
            
            Text {
                text: root.title
                font.pixelSize: 28
                Layout.alignment: Qt.AlignHCenter
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 900
                Layout.preferredHeight: 400
                color: "#f8f8f8"
                border.color: "#ddd"
                radius: 8
                clip: true

                Controls.Robot3DView {
                    id: robot3d
                    anchors.fill: parent
                    // Bindings for joint angles will go here, e.g.:
                    // axis1: root.backend.robot.axis1
                }

                Text {
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    anchors.margins: 10
                    text: "Hold Right Click to Rotate | Middle Click to Pan | Scroll to Zoom"
                    font.pixelSize: 10
                    color: "#888"
                }
            }

            RowLayout {
                spacing: 40
                Layout.alignment: Qt.AlignHCenter

                // --- PLC CONTROL PANEL ---
                GroupBox {
                    title: "PLC -> Robot (Control)"
                    Layout.preferredWidth: 400
                    
                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 15

                        ValueRow { label: "Job ID:"; value: root.backend.robot.controlJobId }
                        ValueRow { label: "Part Type:"; value: root.backend.robot.controlPartType }
                        ValueRow { label: "Area Free (Mask):"; value: "0x" + root.backend.robot.areaFreePlc.toString(16).toUpperCase() }

                        Rectangle { Layout.fillWidth: true; height: 1; color: "#ddd" }

                        RowLayout {
                            spacing: 20
                            BitIndicator { label: "MOVE ENABLE"; active: root.backend.robot.controlMoveEnable; activeColor: "green" }
                            BitIndicator { label: "RESET"; active: root.backend.robot.controlReset; activeColor: "blue" }
                        }
                    }
                }

                // --- ROBOT STATUS PANEL ---
                GroupBox {
                    title: "Robot -> PLC (Status)"
                    Layout.preferredWidth: 400

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 10

                        RowLayout {
                            ValueRow { label: "Job Feedback:"; value: root.backend.robot.jobIdFeedback }
                            Item { Layout.fillWidth: true }
                            ValueRow { label: "Part Mirror:"; value: root.backend.robot.partTypeMirrored }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: "#ddd" }

                        // State Bits
                        Text { text: "State Bits"; font.bold: true; font.pixelSize: 11; color: "#666" }
                        Flow {
                            Layout.fillWidth: true; spacing: 8
                            BitIndicator { label: "MOTION"; active: root.backend.robot.inMotion; activeColor: "orange" }
                            BitIndicator { label: "HOME"; active: root.backend.robot.inHome; activeColor: "blue" }
                            BitIndicator { label: "ENABLED"; active: root.backend.robot.enabled }
                            BitIndicator { label: "ERROR"; active: root.backend.robot.error; activeColor: "red" }
                        }

                        // Quality Bits
                        Text { text: "Quality Bits"; font.bold: true; font.pixelSize: 11; color: "#666"; Layout.topMargin: 5 }
                        Flow {
                            Layout.fillWidth: true; spacing: 8
                            BitIndicator { label: "BRAKE TEST"; active: root.backend.robot.brakeTestOk }
                            BitIndicator { label: "MASTERING"; active: root.backend.robot.masteringOk }
                        }

                        // Mode Bits
                        Text { text: "Mode"; font.bold: true; font.pixelSize: 11; color: "#666"; Layout.topMargin: 5 }
                        Flow {
                            Layout.fillWidth: true; spacing: 8
                            BitIndicator { label: "T1"; active: root.backend.robot.inT1; activeColor: "#9c27b0" }
                            BitIndicator { label: "T2"; active: root.backend.robot.inT2; activeColor: "#673ab7" }
                            BitIndicator { label: "AUT"; active: root.backend.robot.inAut; activeColor: "#3f51b5" }
                            BitIndicator { label: "EXT"; active: root.backend.robot.inExt; activeColor: "#2196f3" }
                        }

                        Rectangle { Layout.fillWidth: true; height: 1; color: "#ddd"; Layout.topMargin: 5 }

                        ValueRow { label: "Error Code:"; value: root.backend.robot.errorCode; color: root.backend.robot.error ? "red" : "black" }
                    }
                }
            }

            // Connection Footer
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20
                
                Text { text: "ADS Status:"; font.bold: true }
                Text { 
                    text: root.backend.robot.adsStatus
                    color: root.backend.robot.adsStatus === "Connected" ? "green" : "red"
                }

                Button {
                    text: "Connect to ADS"
                    onClicked: root.backend.robot.connectAds()
                    enabled: root.backend.robot.adsStatus === "Disconnected" || root.backend.robot.adsStatus === "Faulty"
                }
            }
        }
    }

    // --- REUSABLE COMPONENTS ---

    component ValueRow : RowLayout {
        property string label: ""
        property var value: ""
        property color color: "black"
        Text { text: label; font.bold: true; color: "#555" }
        Text { text: value; font.family: "Monospace"; font.pixelSize: 14; color: parent.color }
    }

    component BitIndicator : Rectangle {
        property string label: ""
        property bool active: false
        property color activeColor: "green"
        
        width: bitLabel.width + 16; height: 20; radius: 4
        color: active ? activeColor : "#eee"
        border.color: active ? "transparent" : "#ccc"
        
        Text {
            id: bitLabel
            anchors.centerIn: parent
            text: parent.label
            color: parent.active ? "white" : "#888"
            font.pixelSize: 9; font.bold: true
        }
    }
}