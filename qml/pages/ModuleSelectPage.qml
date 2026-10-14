import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property var biblesList: []
    property string currentModule: ""
    property string pageTitle: qsTr("Select Translation")

    signal moduleSelected(string moduleName)

    SilicaListView {
        anchors.fill: parent
        model: page.biblesList

        header: PageHeader {
            title: page.pageTitle
        }

        delegate: BackgroundItem {
            id: delegateItem
            width: parent.width

            property string modName: (typeof modelData === "object" && modelData !== null)
                                     ? (modelData.moduleName || modelData["moduleName"] || "")
                                     : modelData
            property string modTitle: (typeof modelData === "object" && modelData !== null)
                                      ? (modelData.title || modelData["title"] || modName)
                                      : modelData

            property bool isSelected: modName === page.currentModule

            Label {
                x: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - (2 * Theme.horizontalPageMargin)
                text: modTitle
                color: isSelected ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
            }

            onClicked: {
                page.moduleSelected(modName)
                pageStack.pop()
            }
        }
    }
}
