import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Page {
    id: mapPage
    allowedOrientations: Orientation.All

    property string currentMapModule: lastMapModule.value
    property string currentTopic: ""
    property var topicsList: []
    property string mapDefinitionHtml: ""
    property var history: []

    // --- Persistent Storage (Module Selection Only) ---
    ConfigurationValue {
        id: lastMapModule
        key: "/apps/harbour-captains-bible/lastMapModule"
        defaultValue: ""
    }

    Component.onCompleted: {
        if (currentMapModule.length > 0) {
            loadTopics()
        }
    }

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
                        lastMapModule.value = moduleName
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

            // Clean Header: Always displays static "Bible Maps" title
            PageHeader {
                title: qsTr("Bible Maps")
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

            // Text and image rendering area
            Label {
                id: mapLabel
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

                    var targetWidth = Math.round(mapLabel.width)
                    var html = mapPage.mapDefinitionHtml

                    // Wrap <img> tags in href links so clicking them opens ImagePage.qml
                    html = html.replace(/<img\b([^>]*)\bsrc=["']([^"']+)["']([^>]*)>/gi, function(match, prefix, src, suffix) {
                        var clean = (prefix + suffix).replace(/\s*(width|height)=["'][^"']*["']/gi, "")
                        return "<a href=\"image:" + src + "\"><img width=\"" + targetWidth + "\" src=\"" + src + "\" " + clean + "></a>"
                    })

                    var css = "<style>" +
                              "a { color: " + Theme.highlightColor + "; text-decoration: underline; }" +
                              "</style>"

                    return css + html
                }

                onLinkActivated: {
                    console.log("[MapPage IMAGE LINK]:", link)
                    if (link.indexOf("image:") === 0) {
                        var imgSrc = link.substring(6)
                        pageStack.push(Qt.resolvedUrl("ImagePage.qml"), {
                            "imageSource": imgSrc,
                            "imageTitle": mapPage.currentTopic
                        })
                    } else {
                        mapPage.handleLinkNavigation(link)
                    }
                }
            }
        }
    }

    // --- JavaScript Helpers ---

    function loadTopics() {
        if (typeof bibleManager !== "undefined" && currentMapModule.length > 0) {
            topicsList = bibleManager.getMapTopics(currentMapModule)
            if (topicsList.length > 0) {
                // Always land on the first/main entry when re-entering or switching modules
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
