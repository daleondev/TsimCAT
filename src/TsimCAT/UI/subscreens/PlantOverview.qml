import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls" as Controls

Control {
    id: root
    property string title: "Plant Overview"
    property var backend: null
    property bool showControls: false

    padding: 0

    contentItem: Item {
        Controls.Plant3DView {
            id: plantView
            anchors.fill: parent
            backend: root.backend
            exitDamperOpen: (root.backend && root.backend.exitConveyor.autoLogic) ? root.backend.exitConveyor.damperOpen : exitDamperToggle.checked
        }

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 20
            color: "#aa000000"
            radius: 8
            width: headerLayout.width + 30
            height: headerLayout.height + 20
            z: 10

            RowLayout {
                id: headerLayout
                anchors.centerIn: parent
                spacing: 15

                Rectangle {
                    Layout.preferredWidth: 12
                    Layout.preferredHeight: 12
                    radius: 6
                    color: "#2ecc71"
                }

                ColumnLayout {
                    spacing: 0
                    Text {
                        text: root.title
                        font.pixelSize: 18
                        font.bold: true
                        color: "white"
                    }
                    Text {
                        text: root.backend && root.backend.usingLocalAdsShadow ? "SIMPLE CELL / LOCAL ADS SHADOW" : "SIMPLE CELL"
                        font.pixelSize: 10
                        font.bold: true
                        color: "#2ecc71"
                    }
                }

                Button {
                    text: root.showControls ? "Hide Controls" : "Show Controls"
                    flat: true
                    palette.buttonText: "white"
                    onClicked: root.showControls = !root.showControls
                }
            }
        }

        Rectangle {
            id: controlPanel
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 20
            width: 340
            color: "#ee1c1c1c"
            radius: 10
            visible: root.showControls
            z: 10

            ScrollView {
                anchors.fill: parent
                clip: true

                ColumnLayout {
                    width: parent.width - 40
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.margins: 20
                    spacing: 14

                    Text { text: "Cell Controls"; color: "white"; font.pixelSize: 18; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                    Rectangle { Layout.fillWidth: true; radius: 8; color: "#1f2b2f"; implicitHeight: modeColumn.implicitHeight + 20
                        ColumnLayout {
                            id: modeColumn
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6
                            Text { text: "Execution"; color: "#d6e4e0"; font.pixelSize: 13; font.bold: true }
                            Text {
                                Layout.fillWidth: true
                                wrapMode: Text.WordWrap
                                text: root.backend && root.backend.usingLocalAdsShadow ? "Stations exchange ADS-shaped data through the in-process shadow. Direct station mode bypasses that station's PLC shadow interface." : "External symbolic links are active."
                                color: "#a9bdba"
                                font.pixelSize: 11
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    CheckBox {
                        id: exitDamperToggle
                        text: "Exit Damper"
                        enabled: !(root.backend && root.backend.exitConveyor.autoLogic)
                        checked: (root.backend && root.backend.exitConveyor.autoLogic) ? root.backend.exitConveyor.damperOpen : false
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    Text {
                        text: "Station Modes"
                        color: "#a0aec0"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Switch {
                        text: "Robot local simulation"
                        checked: root.backend ? root.backend.localRobotMode : false
                        onToggled: if (root.backend)
                            root.backend.localRobotMode = checked
                    }

                    Switch {
                        text: "Rotary table direct local mode"
                        checked: root.backend ? root.backend.localRotaryTableMode : false
                        onToggled: if (root.backend)
                            root.backend.localRotaryTableMode = checked
                    }

                    Switch {
                        text: "Exit conveyor local simulation"
                        checked: root.backend ? root.backend.localExitConveyorMode : false
                        onToggled: if (root.backend)
                            root.backend.localExitConveyorMode = checked
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    Text {
                        text: "Stations"
                        color: "#a0aec0"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Text {
                        text: root.backend && root.backend.rotaryTable
                              ? "Rotary angle: " + Number(root.backend.rotaryTable.angleDegrees).toFixed(1) + " deg"
                              : "Rotary angle: 0.0 deg"
                        color: "#c7d2d9"
                        font.pixelSize: 11
                    }

                    Text {
                        text: root.backend && root.backend.rotaryTable && root.backend.rotaryTable.readyToPick
                              ? "Rotary table ready for pick"
                              : "Rotary table staging"
                        color: root.backend && root.backend.rotaryTable && root.backend.rotaryTable.readyToPick ? "#7fe0a0" : "#d0b06c"
                        font.pixelSize: 11
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: "Queue Type 1"
                            Layout.fillWidth: true
                            onClicked: if (root.backend)
                                root.backend.rotaryTable.queuePart(1)
                        }
                        Button {
                            text: "Queue Type 2"
                            Layout.fillWidth: true
                            onClicked: if (root.backend)
                                root.backend.rotaryTable.queuePart(2)
                        }
                    }

                    Switch {
                        text: "Exit auto logic"
                        checked: root.backend ? root.backend.exitConveyor.autoLogic : false
                        onToggled: if (root.backend)
                            root.backend.exitConveyor.autoLogic = checked
                    }

                    Button {
                        text: "Clear Exit Conveyor"
                        onClicked: if (root.backend)
                            root.backend.exitConveyor.clearParts();
                    }
                }
            }
        }
    }
}
