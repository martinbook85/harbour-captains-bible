import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property var biblesList: []
    property string currentModule: ""
    property string pageTitle: qsTr("Select Translation")

    // DESSA TVÅ RADER LÖSER "Cannot assign to non-existent property"-VARNINGARNA:
    property string selectedCategory: "Bibles"
    property int categoryType: 0 // 0 = Bible, 1 = Commentary, 2 = Maps

    signal moduleSelected(string moduleName)

    // Helper to group flat biblesList into array of { language: "Svenska", modules: [...] }
    property var groupedLanguageList: {
        if (!biblesList || biblesList.length === 0) return []

        var groupsMap = {}
        var order = []

        for (var i = 0; i < biblesList.length; i++) {
            var item = biblesList[i]
            var lang = (typeof item === "object" && item.language) ? item.language : qsTr("Other")

            if (!groupsMap[lang]) {
                groupsMap[lang] = []
                order.push(lang)
            }
            groupsMap[lang].push(item)
        }

        var result = []
        for (var j = 0; j < order.length; j++) {
            var lName = order[j]
            result.push({
                "language": lName,
                "modules": groupsMap[lName]
            })
        }
        return result
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: contentColumn.height + Theme.paddingLarge

        Column {
            id: contentColumn
            width: parent.width

            PageHeader {
                title: page.pageTitle
            }

            // Outer loop: Languages
            Repeater {
                model: page.groupedLanguageList

                delegate: Column {
                    width: parent.width
                    spacing: Theme.paddingSmall

                    property var groupData: modelData

                    // Language Header Box
                    Item {
                        width: parent.width
                        height: langBox.height + Theme.paddingMedium

                        Rectangle {
                            id: langBox
                            anchors.centerIn: parent
                            width: parent.width - (2 * Theme.horizontalPageMargin)
                            height: langText.height + (2 * Theme.paddingSmall)
                            color: Theme.rgba(Theme.highlightBackgroundColor, 0.15)
                            radius: Theme.paddingSmall

                            Label {
                                id: langText
                                anchors.centerIn: parent
                                text: groupData.language || qsTr("Other")
                                color: Theme.highlightColor
                                font.pixelSize: Theme.fontSizeMedium
                                font.bold: true
                            }
                        }
                    }

                    // Inner loop: Modules within this Language
                    Repeater {
                        model: groupData.modules

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
                                truncationMode: TruncationMode.Fade
                            }

                            onClicked: {
                                page.moduleSelected(modName)
                                pageStack.pop()
                            }
                        }
                    }

                    // Bottom spacing between language groups
                    Item {
                        width: parent.width
                        height: Theme.paddingMedium
                    }
                }
            }
        }
    }
}
