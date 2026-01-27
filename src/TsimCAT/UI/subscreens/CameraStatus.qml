import QtQuick
import QtQuick.Controls

Control {
    id: root
    property string title: "Camera Status"

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
            width: 320
            height: 240
            color: "black"
            Text {
                text: "LIVE FEED"
                color: "white"
                anchors.centerIn: parent
            }
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
