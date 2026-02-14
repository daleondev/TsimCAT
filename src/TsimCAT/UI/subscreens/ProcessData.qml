pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls

Control {
    id: root
    property string title: "Process Data"

    ListView {
        anchors.fill: parent
        anchors.margins: 40
        header: Text { text: root.title; font.pixelSize: 32; bottomPadding: 20 }
        model: 20
        delegate: ItemDelegate {
            id: delegate
            required property int index
            width: ListView.view ? ListView.view.width : 0
            text: "Data point #" + delegate.index + ": " + (Math.random() * 100).toFixed(2)
        }
    }
}
