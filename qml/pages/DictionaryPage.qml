import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property string initialTopic: ""
    property string activeTopic: confLastTopic.value
    property var dictionaryList: []
    property string definitionHtml: ""
    property var searchResults: []

    // 1. Configuration values owned directly by page root
    ConfigurationValue {
        id: selectedDictionarySetting
        key: "/apps/harbour-captains-bible/lastDictionaryModule"
        defaultValue: ""
    }

    ConfigurationValue {
        id: confLastTopic
        key: "/apps/harbour-captains-bible/lastDictionaryTopic"
        defaultValue: ""
    }

    property string activeModule: selectedDictionarySetting.value

    // 2. Helper functions to update settings safely across contexts
    function updateSelectedModule(modName) {
        page.activeModule = modName
        selectedDictionarySetting.value = modName
    }

    function updateSelectedTopic(topic) {
        page.activeTopic = topic
        confLastTopic.value = topic
    }

    onInitialTopicChanged: {
        if (initialTopic !== "") {
            console.log("[DictionaryPage] Received initialTopic:", initialTopic)
            handleIncomingTopic(initialTopic)
        }
    }

    Component.onCompleted: {
        loadDictionaries()
        if (initialTopic !== "") {
            handleIncomingTopic(initialTopic)
        } else if (activeTopic !== "") {
            fetchDefinition()
        }
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            if (dictionaryList.length === 0) {
                loadDictionaries()
            }
        }
    }

    function handleIncomingTopic(topicCode) {
        var def = bibleManager.getDictionaryDefinition(page.activeModule, topicCode)
        if (def && def !== "") {
            updateSelectedTopic(topicCode)
            page.definitionHtml = def
            return
        }

        var autoMod = bibleManager.findStrongDictionaryModule(topicCode)
        if (autoMod && autoMod !== "") {
            console.log("[DictionaryPage] Switched automatically to module:", autoMod)
            updateSelectedModule(autoMod)
            updateSelectedTopic(topicCode)
            page.fetchDefinition()
        } else {
            updateSelectedTopic(topicCode)
            page.definitionHtml = ""
        }
    }

    function loadDictionaries() {
        var rawList = bibleManager.getDictionaryModules()
        if (rawList && rawList.length > 0) {
            page.dictionaryList = rawList
            if (page.activeModule === "") {
                updateSelectedModule(page.dictionaryList[0].moduleName)
            }
            if (page.activeTopic === "") {
                page.fetchDefinition()
            }
        }
    }

    function fetchDefinition() {
        if (page.activeModule === "") return
        console.log("[DictionaryPage] Fetching definition -> Module:", page.activeModule, "Topic:", page.activeTopic)
        var result = bibleManager.getDictionaryDefinition(page.activeModule, page.activeTopic)
        page.definitionHtml = result ? result : ""
    }

    function getShortModuleName() {
        for (var i = 0; i < page.dictionaryList.length; i++) {
            if (page.dictionaryList[i].moduleName === page.activeModule) {
                var title = page.dictionaryList[i].title || page.activeModule
                return title.length > 20 ? title.substring(0, 18) + "..." : title
            }
        }
        return page.activeModule
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.implicitHeight + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Get More Dictionaries")
                onClicked: pageStack.push(Qt.resolvedUrl("ModuleDownloaderPage.qml"))
            }

            MenuItem {
                text: qsTr("Select Active Dictionary")
                enabled: page.dictionaryList.length > 0
                onClicked: {
                    var pageObj = pageStack.push(Qt.resolvedUrl("ModuleSelectPage.qml"), {
                        "biblesList": page.dictionaryList,
                        "currentModule": page.activeModule,
                        "pageTitle": qsTr("Dictionaries"),
                        "categoryType": 2
                    })

                    var onModuleSelected = function(modName) {
                        page.updateSelectedModule(modName)
                        page.updateSelectedTopic("")
                        page.fetchDefinition()
                        pageObj.moduleSelected.disconnect(onModuleSelected)
                    }
                    pageObj.moduleSelected.connect(onModuleSelected)
                }
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: page.activeTopic !== "" ? page.activeTopic : qsTr("Dictionary")
                description: page.getShortModuleName()
            }

            SearchField {
                id: dictSearchField
                width: parent.width
                placeholderText: qsTr("Search all dictionaries (e.g. H7225)...")
                onTextChanged: {
                    var query = text.trim()
                    if (query.length >= 2) {
                        page.searchResults = bibleManager.searchAllDictionaries(query)
                    } else {
                        page.searchResults = []
                    }
                }
            }

            // Global search result list
            Repeater {
                model: page.searchResults
                delegate: BackgroundItem {
                    id: searchDelegate
                    width: parent.width
                    height: Theme.itemSizeMedium

                    property var itemData: modelData
                    property var rootPage: page

                    onClicked: {
                        console.log("[DictionaryPage] Selected result:", itemData.topic, "Module:", itemData.moduleName)

                        dictSearchField.focus = false
                        dictSearchField.text = ""

                        // Safely invoke functions on rootPage object
                        rootPage.updateSelectedModule(itemData.moduleName)
                        rootPage.updateSelectedTopic(itemData.topic)

                        rootPage.searchResults = []
                        rootPage.fetchDefinition()
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        x: Theme.horizontalPageMargin
                        width: parent.width - (2 * Theme.horizontalPageMargin)

                        Label {
                            text: searchDelegate.itemData ? searchDelegate.itemData.topic : ""
                            color: searchDelegate.highlighted ? Theme.highlightColor : Theme.primaryColor
                            font.pixelSize: Theme.fontSizeMedium
                        }

                        Label {
                            text: searchDelegate.itemData ? searchDelegate.itemData.dictTitle : ""
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                        }
                    }
                }
            }

            // Article definition display
            Label {
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.horizontalCenter: parent.horizontalCenter
                textFormat: Text.RichText
                wrapMode: Text.Wrap
                visible: page.searchResults.length === 0 && page.definitionHtml !== ""
                text: page.definitionHtml
                font.pixelSize: Theme.fontSizeMedium

                onLinkActivated: {
                    var linkStr = link.toString()
                    if (linkStr.indexOf("strong://") === 0) {
                        var code = linkStr.replace("strong://", "")
                        page.handleIncomingTopic(code)
                    } else {
                        page.handleIncomingTopic(linkStr)
                    }
                }
            }

            // Empty state placeholder
            Column {
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.paddingLarge
                visible: page.searchResults.length === 0 && page.definitionHtml === ""

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    color: Theme.secondaryColor
                    text: page.activeTopic !== ""
                          ? qsTr("No definition found for '%1' in installed dictionaries.").arg(page.activeTopic)
                          : qsTr("No dictionary module available.")
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Download Dictionaries")
                    onClicked: pageStack.push(Qt.resolvedUrl("ModuleDownloaderPage.qml"))
                }
            }
        }
    }
}
