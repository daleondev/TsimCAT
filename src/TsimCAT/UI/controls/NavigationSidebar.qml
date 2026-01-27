import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../icons" as Icons

Rectangle {
    id: root
    
    property alias model: repeater.model
    property int currentIndex: 0
    property string title: "TsimCAT"
    property string footerText: ""

    color: "#2c3e50"
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5

        Text {
            text: root.title
            color: "white"
            font.pixelSize: 28
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 20
            Layout.bottomMargin: 30
        }

        Repeater {
            id: repeater
            
            delegate: Button {
                id: navButton
                Layout.fillWidth: true
                flat: true
                text: modelData.name
                highlighted: root.currentIndex === index
                
                contentItem: RowLayout {
                    spacing: 15
                    
                    Item {
                        width: 24; height: 24
                        Layout.leftMargin: 10
                        
                        Loader {
                            id: iconLoader
                            anchors.centerIn: parent
                            source: "../icons/" + modelData.icon + ".qml"
                            
                            // Define the target color on the loader
                            property color targetColor: navButton.highlighted ? "#3498db" : "white"
                            
                            // Use Binding to safely push the color to the loaded item
                            Binding {
                                target: iconLoader.item
                                property: "color"
                                value: iconLoader.targetColor
                                when: iconLoader.status === Loader.Ready
                            }
                        }
                    }

                    Text {
                        text: navButton.text
                        color: navButton.highlighted ? "#3498db" : "white"
                        font.pixelSize: 16
                        verticalAlignment: Text.AlignVCenter
                        Layout.fillWidth: true
                    }
                }

                background: Rectangle {
                    color: parent.pressed ? "#34495e" : (parent.highlighted ? "#1a252f" : "transparent")
                    radius: 4
                }

                onClicked: root.currentIndex = index
            }
        }

        Item { Layout.fillHeight: true } // Spacer

        Text {
            text: root.footerText
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