import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: mapPage
    allowedOrientations: Orientation.All

    property string currentMapModule: ""
    property string currentTopic: ""
    property var topicsList: []
    property string mapDefinitionHtml: ""
    property var history: []

    SilicaFlickable {
        id: mapFlickable
        anchors.fill: parent
        contentHeight: contentColumn.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Select Map Module")
                onClicked: {
                    var page = pageStack.push(Qt.resolvedUrl("ModuleSelectPage.qml"), {
                        "biblesList": bibleManager.getInstalledMaps(),
                        "selectedCategory": "Maps",
                        "categoryType": 2,
                        "pageTitle": qsTr("Select Map Module"),
                        "currentModule": mapPage.currentMapModule
                    })

                    page.moduleSelected.connect(function(moduleName) {
                        mapPage.currentMapModule = moduleName
                        mapPage.history = []
                        mapPage.loadTopics()
                    })
                }
            }

            MenuItem {
                text: qsTr("Back to main page")
                visible: mapPage.history.length > 0 || (mapPage.topicsList.length > 0 && mapPage.currentTopic !== mapPage.topicsList[0])
                onClicked: {
                    mapPage.history = []
                    if (mapPage.topicsList.length > 0) {
                        topicComboBox.currentIndex = 0
                        mapPage.currentTopic = mapPage.topicsList[0]
                        mapPage.loadDefinition()
                    }
                }
            }

            MenuItem {
                text: qsTr("Back to previous")
                visible: mapPage.history.length > 0
                onClicked: {
                    var prevTopic = mapPage.history.pop()
                    mapPage.historyChanged()

                    var foundIdx = -1
                    for (var i = 0; i < mapPage.topicsList.length; i++) {
                        if (mapPage.topicsList[i] === prevTopic) {
                            foundIdx = i
                            break
                        }
                    }

                    if (foundIdx !== -1) {
                        topicComboBox.currentIndex = foundIdx
                    }
                    mapPage.currentTopic = prevTopic
                    mapPage.loadDefinition()
                }
            }
        }

        Column {
            id: contentColumn
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: mapPage.currentMapModule.length > 0
                       ? mapPage.currentMapModule
                       : qsTr("Bible Maps")
            }

            ComboBox {
                id: topicComboBox
                width: parent.width
                label: qsTr("Map / Entry")
                visible: mapPage.topicsList.length > 0

                menu: ContextMenu {
                    Repeater {
                        model: mapPage.topicsList
                        delegate: MenuItem {
                            text: modelData
                        }
                    }
                }

                onCurrentIndexChanged: {
                    if (currentIndex >= 0 && currentIndex < mapPage.topicsList.length) {
                        var newTopic = mapPage.topicsList[currentIndex]
                        if (mapPage.currentTopic && mapPage.currentTopic !== newTopic) {
                            mapPage.history.push(mapPage.currentTopic)
                            mapPage.historyChanged()
                        }
                        mapPage.currentTopic = newTopic
                        mapPage.loadDefinition()
                    }
                }
            }

            // Text- och bildområde
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - (2 * Theme.horizontalPageMargin)
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
                textFormat: Text.RichText

                text: {
                    if (!mapPage.mapDefinitionHtml) {
                        return mapPage.currentMapModule.length > 0
                               ? qsTr("Select a map entry above.")
                               : qsTr("Pull down to select a map module.")
                    }
                    var css = "<style>" +
                              "img { max-width: 100%; height: auto; display: block; margin: 12px 0; }" +
                              "a { color: " + Theme.highlightColor + "; text-decoration: underline; }" +
                              "</style>"
                    return css + mapPage.mapDefinitionHtml
                }

                onLinkActivated: {
                    mapPage.handleLinkNavigation(link)
                }
            }
        }
    }

    // --- JavaScript Helpers ---

    function loadTopics() {
        if (typeof bibleManager !== "undefined" && currentMapModule.length > 0) {
            topicsList = bibleManager.getMapTopics(currentMapModule)
            if (topicsList.length > 0) {
                currentTopic = topicsList[0]
                topicComboBox.currentIndex = 0
                loadDefinition()
            } else {
                mapDefinitionHtml = qsTr("No map topics found in this module.")
            }
        }
    }

    function loadDefinition() {
        if (typeof bibleManager !== "undefined" && currentMapModule.length > 0 && currentTopic.length > 0) {
            mapDefinitionHtml = bibleManager.getMapDefinition(currentMapModule, currentTopic)
        }
    }

    function handleLinkNavigation(link) {
        var target = link.trim()

        if (target.indexOf("#") !== -1) {
            target = target.split("#")[1]
        }

        target = decodeURIComponent(target)

        if (currentTopic && currentTopic.length > 0) {
            history.push(currentTopic)
            historyChanged()
        }

        var foundIndex = -1
        for (var i = 0; i < topicsList.length; i++) {
            if (topicsList[i] === target || topicsList[i].indexOf(target) !== -1) {
                foundIndex = i
                break
            }
        }

        if (foundIndex !== -1) {
            topicComboBox.currentIndex = foundIndex
            currentTopic = topicsList[foundIndex]
            loadDefinition()
            return
        }

        if (link.indexOf("http://") === 0 || link.indexOf("https://") === 0) {
            Qt.openUrlExternally(link)
            return
        }

        if (typeof bibleManager !== "undefined") {
            var directDef = bibleManager.getMapDefinition(currentMapModule, target)
            if (directDef && directDef.length > 0) {
                mapDefinitionHtml = directDef
                currentTopic = target
            }
        }
    }
}
