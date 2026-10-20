import QtQuick 2.6
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    property var availableModules: []
    property string selectedModule: ""

    DialogHeader {
        title: qsTr("Select Commentary")
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        header: dialogHeader
        model: availableModules

        delegate: BackgroundItem {
            width: parent.width
            onClicked: {
                selectedModule = modelData.moduleName
                dialog.accept()
            }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter

                Label {
                    text: modelData.title
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeMedium
                }

                Label {
                    text: modelData.moduleName
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                }
            }
        }
    }
}
