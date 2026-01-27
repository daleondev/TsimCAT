import QtQuick
import QtQuick.Controls

Control {
    id: root
    property string title: "Gantry Status"

    Column {
        anchors.centerIn: parent
        spacing: 10
        Text {
            text: root.title
            font.pixelSize: 32
            color: "#333"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Grid {
            columns: 2
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter
            Text { text: "X-Axis:" } Text { text: "124.5 mm"; font.bold: true }
            Text { text: "Y-Axis:" } Text { text: "89.2 mm"; font.bold: true }
            Text { text: "Z-Axis:" } Text { text: "10.0 mm"; font.bold: true }
        }
    }
}
