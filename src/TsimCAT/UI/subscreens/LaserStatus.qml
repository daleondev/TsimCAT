import QtQuick
import QtQuick.Controls

Control {
    id: root
    property string title: "Laser Status"

    Column {
        anchors.centerIn: parent
        spacing: 20
        Text {
            text: root.title
            font.pixelSize: 32
            color: "#333"
            anchors.horizontalCenter: parent.horizontalCenter
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
