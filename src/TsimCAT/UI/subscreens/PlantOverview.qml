import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls" as Controls

Control {
    id: root
    property string title: "Plant Overview"
    property var backend: null

    padding: 0

    contentItem: Item {
        // --- 3D SCENE ---
        Controls.Plant3DView {
            id: plantView
            anchors.fill: parent
            backend: root.backend
        }

        // --- OVERLAYS ---
        
        // Header
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 20
            color: "#aa000000"
            radius: 8
            width: headerLayout.width + 30
            height: headerLayout.height + 20

            RowLayout {
                id: headerLayout
                anchors.centerIn: parent
                spacing: 15

                Rectangle {
                    width: 12; height: 12; radius: 6
                    color: "#2ecc71"
                    
                    PropertyAnimation on opacity {
                        from: 1.0; to: 0.4; duration: 1000
                        loops: Animation.Infinite; running: true
                    }
                }

                ColumnLayout {
                    spacing: 0
                    Text { text: root.title; font.pixelSize: 18; font.bold: true; color: "white" }
                    Text { text: "SYSTEM OPERATIONAL"; font.pixelSize: 10; font.bold: true; color: "#2ecc71" }
                }
            }
        }

        // Legend / Help
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 20
            color: "#aa000000"
            radius: 6
            width: legendText.width + 20
            height: legendText.height + 10

            Text {
                id: legendText
                anchors.centerIn: parent
                text: "Right Click: Orbit | Left Click: Pan | Scroll: Zoom"
                color: "#ccc"
                font.pixelSize: 11
            }
        }
    }
}