import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TsimCAT.Backend 1.0 as BackendModule
import "subscreens" as Subscreens

ApplicationWindow {
    id: window
    width: 1280
    height: 720
    visible: true
    title: qsTr("TsimCAT Control Center")

    BackendModule.Backend {
        id: backend
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Navigation Bar
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 240
            color: "#2c3e50"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5

                Text {
                    text: "TsimCAT"
                    color: "white"
                    font.pixelSize: 28
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 20
                    Layout.bottomMargin: 30
                }

                Repeater {
                    model: [
                        { name: "Plant Overview", icon: "dashboard" },
                        { name: "Robot Status", icon: "robot" },
                        { name: "Camera Status", icon: "videocam" },
                        { name: "Laser Status", icon: "flare" },
                        { name: "Gantry Status", icon: "settings_input_component" },
                        { name: "Process Data", icon: "analytics" }
                    ]

                    delegate: Button {
                        Layout.fillWidth: true
                        flat: true
                        text: modelData.name
                        highlighted: contentStack.currentIndex === index
                        
                        contentItem: Text {
                            text: parent.text
                            color: parent.highlighted ? "#3498db" : "white"
                            font.pixelSize: 16
                            leftPadding: 15
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            color: parent.pressed ? "#34495e" : (parent.highlighted ? "#1a252f" : "transparent")
                            radius: 4
                        }

                        onClicked: contentStack.currentIndex = index
                    }
                }

                Item { Layout.fillHeight: true } // Spacer

                Text {
                    text: "QCoro Status: " + backend.asyncTestStatus
                    color: "#bdc3c7"
                    font.pixelSize: 12
                    Layout.alignment: Qt.AlignHCenter
                    Layout.bottomMargin: 10
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        // Main Content Area
        StackLayout {
            id: contentStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0

            Subscreens.PlantOverview {}
            Subscreens.RobotStatus {}
            Subscreens.CameraStatus {}
            Subscreens.LaserStatus {}
            Subscreens.GantryStatus {}
            Subscreens.ProcessData {}
        }
    }
}