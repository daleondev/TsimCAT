import QtQuick
import QtQuick.Controls

Control {
    id: root
    property string title: "Plant Overview"

    Column {
        anchors.centerIn: parent
        spacing: 20
        Text {
            text: root.title
            font.pixelSize: 32
            color: "#333"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Row {
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter
            Rectangle {
                width: 20; height: 20; color: "green"; radius: 10
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: "System healthy"
                font.pixelSize: 18
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
