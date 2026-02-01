import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Control {
    id: root
    property string title: "Robot Status"
    required property var backend

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20

        Text {
            text: root.title
            font.pixelSize: 32
            color: "#333"
            Layout.alignment: Qt.AlignHCenter
        }

        Rectangle {
            Layout.preferredWidth: 450
            Layout.preferredHeight: 300
            color: "#f8f8f8"
            border.color: "#ddd"
            radius: 8

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15

                // Header
                RowLayout {
                    Text { text: "ADS Link:"; font.bold: true }
                    Text { 
                        text: root.backend.robot.adsStatus
                        color: root.backend.robot.adsStatus === "Connected" ? "green" : "red"
                    }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        width: 12; height: 12; radius: 6
                        color: root.backend.robot.enabled ? "green" : "grey"
                        ToolTip.visible: true
                        ToolTip.text: "Robot Power"
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#eee" }

                // Job Info
                RowLayout {
                    spacing: 30
                    ColumnLayout {
                        Text { text: "Active Job ID"; font.pixelSize: 12; color: "#666" }
                        Text { text: root.backend.robot.jobId; font.pixelSize: 24; font.bold: true; font.family: "Monospace" }
                    }
                    ColumnLayout {
                        Text { text: "Part Type"; font.pixelSize: 12; color: "#666" }
                        Text { text: root.backend.robot.partType; font.pixelSize: 24; font.bold: true; font.family: "Monospace" }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#eee" }

                // Status Bits
                Flow {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    StatusBadge { text: "IN MOTION"; active: root.backend.robot.inMotion; activeColor: "orange" }
                    StatusBadge { text: "IN HOME"; active: root.backend.robot.inHome; activeColor: "blue" }
                    StatusBadge { text: "ENABLED"; active: root.backend.robot.enabled; activeColor: "green" }
                }
            }
        }

        Button {
            text: "Connect to ADS"
            Layout.alignment: Qt.AlignHCenter
            onClicked: root.backend.robot.connectAds()
            enabled: root.backend.robot.adsStatus === "Disconnected"
        }
    }

    component StatusBadge : Rectangle {
        property string text: ""
        property bool active: false
        property color activeColor: "green"
        
        width: label.width + 20
        height: 24
        radius: 12
        color: active ? activeColor : "#e0e0e0"
        
        Text {
            id: label
            anchors.centerIn: parent
            text: parent.text
            color: parent.active ? "white" : "#999"
            font.pixelSize: 10
            font.bold: true
        }
    }
}