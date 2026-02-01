import QtQuick
import QtQuick.Controls

Control {
    id: root
    property string title: "Laser Status"
    required property var backend

    Column {
        anchors.centerIn: parent
        spacing: 20
        Text {
            text: root.title
            font.pixelSize: 32
            color: "#333"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        
        Rectangle {
            width: 400
            height: 200
            color: "#f0f0f0"
            border.color: "#ccc"
            radius: 5
            anchors.horizontalCenter: parent.horizontalCenter
            
            Column {
                anchors.centerIn: parent
                spacing: 10
                
                Text {
                    text: "TCP Server Status:"
                    font.bold: true
                }
                Text {
                    text: root.backend.tcpStatus
                    color: root.backend.tcpStatus.includes("Connected") ? "green" : "red"
                    font.pixelSize: 16
                }
                
                Text {
                    text: "Last Message:"
                    font.bold: true
                    topPadding: 10
                }
                Text {
                    text: root.backend.lastMessage
                    font.family: "Monospace"
                    color: "blue"
                }
            }
        }

        Button {
            text: "Start TCP Server"
            anchors.horizontalCenter: parent.horizontalCenter
            enabled: root.backend.tcpStatus === "Disconnected" || root.backend.tcpStatus.startsWith("Start Failed")
            onClicked: root.backend.startTcpServer()
        }

        Switch {
            text: "Laser Armed"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            text: "Power: 4500W"
            font.pixelSize: 18
            color: "red"
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}