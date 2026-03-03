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
            entryDamperOpen: (root.backend && root.backend.entryConveyor.autoLogic) ? root.backend.entryConveyor.damperOpen : entryDamperToggle.checked
            exitDamperOpen: (root.backend && root.backend.exitConveyor.autoLogic) ? root.backend.exitConveyor.damperOpen : exitDamperToggle.checked
            doorOpen: doorToggle.checked
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
                        text: "SIMPLE CELL"
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

                    Text {
                        text: "Manual Controls"
                        color: "white"
                        font.pixelSize: 18
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#444"
                    }

                    CheckBox {
                        id: entryDamperToggle
                        text: "Entry Damper"
                        enabled: !(root.backend && root.backend.entryConveyor.autoLogic)
                        checked: (root.backend && root.backend.entryConveyor.autoLogic) ? root.backend.entryConveyor.damperOpen : false
                    }

                    CheckBox {
                        id: exitDamperToggle
                        text: "Exit Damper"
                        enabled: !(root.backend && root.backend.exitConveyor.autoLogic)
                        checked: (root.backend && root.backend.exitConveyor.autoLogic) ? root.backend.exitConveyor.damperOpen : false
                    }

                    CheckBox {
                        id: doorToggle
                        text: "Safety Door"
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
                        text: "Entry conveyor local simulation"
                        checked: root.backend ? root.backend.localEntryConveyorMode : false
                        onToggled: if (root.backend)
                            root.backend.localEntryConveyorMode = checked
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
                        text: "Conveyors"
                        color: "#a0aec0"
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Switch {
                        text: "Entry auto logic"
                        checked: root.backend ? root.backend.entryConveyor.autoLogic : false
                        onToggled: if (root.backend)
                            root.backend.entryConveyor.autoLogic = checked
                    }

                    Switch {
                        text: "Exit auto logic"
                        checked: root.backend ? root.backend.exitConveyor.autoLogic : false
                        onToggled: if (root.backend)
                            root.backend.exitConveyor.autoLogic = checked
                    }

                    Button {
                        text: "Spawn Part on Entry"
                        onClicked: if (root.backend)
                            root.backend.entryConveyor.spawnPart(1)
                    }

                    Button {
                        text: "Clear All Parts"
                        onClicked: if (root.backend) {
                            root.backend.entryConveyor.clearParts();
                            root.backend.exitConveyor.clearParts();
                        }
                    }
                }
            }
        }
    }
}
