import QtQuick
import QtQuick.Controls

Control {
    id: root
    property string title: "Robot Status"

    Column {
        anchors.centerIn: parent
        spacing: 20
        Text {
            text: root.title
            font.pixelSize: 32
            color: "#333"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        ProgressBar {
            value: 0.75
            width: 200
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            text: "Payload: 75%"
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
