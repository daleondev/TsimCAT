import QtQuick
import QtQuick.Controls
import TsimCAT.Backend 1.0 as BackendModule

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("TsimCAT")

    BackendModule.Backend {
        id: backend
    }

    Column {
        anchors.centerIn: parent
        spacing: 10

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Hello from TsimCAT Frontend!")
            font.pixelSize: 24
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: backend.welcomeMessage
            font.pixelSize: 18
            color: "gray"
        }
    }
}
