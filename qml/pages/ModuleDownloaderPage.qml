import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property string selectedCategory: "Bibles"
    property string filterText: ""
    property bool catalogFetched: false
    property int activeDownloadsTick: 0

    property var groupedCatalog: ({})
    property var groupKeys: []
    property bool isLoading: false

    function refreshModel() {
        isLoading = true
        deferTimer.restart()
    }

    Timer {
        id: deferTimer
        interval: 50
        repeat: false
        onTriggered: {
            var allModules = bibleManager.getAvailableModules("All") || []
            var groups = {}
            var query = filterText.toLowerCase().trim()

            for (var i = 0; i < allModules.length; i++) {
                var item = allModules[i]

                if (!item || !item.category) {
                    continue
                }

                // Match category (case-insensitive check for resilience)
                if (item.category.toLowerCase() !== page.selectedCategory.toLowerCase()) {
                    continue
                }

                if (query.length > 0) {
                    var nameStr = item.name ? item.name.toLowerCase() : ""
                    var descStr = item.description ? item.description.toLowerCase() : ""
                    var groupStr = item.groupTitle ? item.groupTitle.toLowerCase() : ""
                    var langStr = item.language ? item.language.toLowerCase() : ""

                    var matchName = nameStr.indexOf(query) !== -1
                    var matchDesc = descStr.indexOf(query) !== -1
                    var matchGroup = groupStr.indexOf(query) !== -1
                    var matchLang = langStr.indexOf(query) !== -1

                    if (!matchName && !matchDesc && !matchGroup && !matchLang) continue
                }

                var gTitle = item.groupTitle || "Other Modules"
                if (!groups[gTitle]) {
                    groups[gTitle] = []
                }
                groups[gTitle].push(item)
            }

            groupedCatalog = groups
            groupKeys = Object.keys(groups).sort()
            isLoading = false
        }
    }

    Component.onCompleted: refreshModel()

    Connections {
        target: bibleManager

        onDownloadingModulesChanged: {
            page.activeDownloadsTick++
        }

        onCatalogUpdated: {
            page.catalogFetched = true
            page.refreshModel()
        }
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: page.groupKeys

        PullDownMenu {
            MenuItem {
                text: page.catalogFetched ? qsTr("Refresh catalog") : qsTr("Fetch full catalog")
                enabled: !bibleManager.isFetchingCatalog
                onClicked: bibleManager.fetchRemoteCatalog()
            }
        }

        header: Column {
            width: parent.width

            PageHeader {
                title: qsTr("Download Modules")
            }

            ProgressBar {
                width: parent.width
                indeterminate: true
                label: page.isLoading ? qsTr("Building category list...") : qsTr("Updating module catalog...")
                visible: bibleManager.isFetchingCatalog || page.isLoading
            }

            SearchField {
                id: searchField
                width: parent.width
                placeholderText: qsTr("Search module, language...")

                onTextChanged: {
                    page.filterText = text
                    page.refreshModel()
                }
            }

            ComboBox {
                width: parent.width
                label: qsTr("Category")
                currentIndex: 0

                menu: ContextMenu {
                    MenuItem { text: qsTr("Bibles") }
                    MenuItem { text: qsTr("Commentaries") }
                    MenuItem { text: qsTr("Dictionaries") }
                    MenuItem { text: qsTr("Devotions") }
                    MenuItem { text: qsTr("Maps") }
                }

                onCurrentIndexChanged: {
                    var categories = ["Bibles", "Commentaries", "Dictionaries", "Devotions", "Maps"]
                    page.selectedCategory = categories[currentIndex]
                    page.refreshModel()
                }
            }
        }

        delegate: BackgroundItem {
            id: groupDelegate
            width: parent.width
            height: groupColumn.height

            property bool expanded: false
            property var groupItems: expanded ? (page.groupedCatalog[modelData] || []) : []

            Column {
                id: groupColumn
                width: parent.width

                ValueButton {
                    label: modelData ? modelData : ""
                    value: (modelData && page.groupedCatalog[modelData]) ? page.groupedCatalog[modelData].length.toString() : "0"
                    width: parent.width
                    onClicked: groupDelegate.expanded = !groupDelegate.expanded
                }

                Loader {
                    width: parent.width
                    active: groupDelegate.expanded
                    sourceComponent: Component {
                        Column {
                            width: parent.width

                            Repeater {
                                model: groupDelegate.groupItems

                                delegate: ListItem {
                                    id: moduleItem
                                    width: parent.width
                                    contentHeight: itemExpanded ? modColumn.implicitHeight + Theme.paddingMedium * 2 : Theme.itemSizeMedium

                                    property var itemData: modelData
                                    property bool itemExpanded: false

                                    // Säkrad mot null-objekt
                                    property bool isDownloadingThis: {
                                        var dummy = page.activeDownloadsTick
                                        return (itemData && typeof itemData === "object" && itemData.name)
                                                ? bibleManager.isModuleDownloading(itemData.name)
                                                : false
                                    }

                                    // Säkrad rad 163
                                    property bool isInstalledThis: (itemData && typeof itemData === "object" && typeof itemData.isInstalled !== "undefined")
                                                                   ? itemData.isInstalled
                                                                   : false

                                    RemorseItem { id: remorse }

                                    onClicked: {
                                        itemExpanded = !itemExpanded
                                    }

                                    menu: ContextMenu {
                                        enabled: moduleItem.isInstalledThis

                                        MenuItem {
                                            text: qsTr("Remove from local storage")
                                            onClicked: {
                                                if (moduleItem.itemData && moduleItem.itemData.name) {
                                                    var modName = moduleItem.itemData.name
                                                    remorse.execute(moduleItem, qsTr("Deleting %1").arg(modName), function() {
                                                        if (typeof bibleManager !== "undefined") {
                                                            bibleManager.deleteModule(modName)
                                                        }
                                                    })
                                                }
                                            }
                                        }
                                    }

                                    Item {
                                        anchors.fill: parent

                                        Column {
                                            id: modColumn
                                            anchors.left: parent.left
                                            anchors.leftMargin: Theme.horizontalPageMargin
                                            anchors.right: actionContainer.left
                                            anchors.rightMargin: Theme.paddingMedium
                                            anchors.verticalCenter: moduleItem.itemExpanded ? undefined : parent.verticalCenter
                                            y: moduleItem.itemExpanded ? Theme.paddingMedium : 0

                                            Label {
                                                text: (moduleItem.itemData && moduleItem.itemData.description) ? moduleItem.itemData.description : ""
                                                color: moduleItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                                                font.pixelSize: Theme.fontSizeMedium
                                                anchors.left: parent.left
                                                anchors.right: parent.right
                                                truncationMode: moduleItem.itemExpanded ? TruncationMode.None : TruncationMode.Elide
                                                wrapMode: moduleItem.itemExpanded ? Text.Wrap : Text.NoWrap
                                            }

                                            Label {
                                                text: moduleItem.itemData ? ((moduleItem.itemData.name || "") + (moduleItem.itemData.language ? " (" + moduleItem.itemData.language + ")" : "")) : ""
                                                color: moduleItem.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                                                font.pixelSize: Theme.fontSizeExtraSmall
                                            }

                                            Column {
                                                width: parent.width
                                                visible: moduleItem.itemExpanded
                                                spacing: Theme.paddingSmall

                                                Item {
                                                    width: parent.width
                                                    height: Theme.paddingSmall
                                                }

                                                Label {
                                                    visible: !!(moduleItem.itemData && moduleItem.itemData.date && moduleItem.itemData.date.length > 0)
                                                    text: qsTr("Details: ") + (moduleItem.itemData ? moduleItem.itemData.date : "")
                                                    color: Theme.secondaryColor
                                                    font.pixelSize: Theme.fontSizeExtraSmall
                                                }
                                            }
                                        }

                                        Item {
                                            id: actionContainer
                                            width: Theme.itemSizeSmall
                                            height: Theme.itemSizeMedium
                                            anchors.right: parent.right
                                            anchors.rightMargin: Theme.horizontalPageMargin
                                            anchors.top: parent.top

                                            BusyIndicator {
                                                anchors.centerIn: parent
                                                size: BusyIndicatorSize.Small
                                                running: moduleItem.isDownloadingThis
                                                visible: moduleItem.isDownloadingThis
                                            }

                                            IconButton {
                                                anchors.centerIn: parent
                                                visible: !moduleItem.isDownloadingThis

                                                icon.source: moduleItem.isInstalledThis ? "image://theme/icon-m-acknowledge" : "image://theme/icon-m-cloud-download"
                                                icon.color: moduleItem.isInstalledThis ? Theme.highlightColor : Theme.primaryColor

                                                onClicked: {
                                                    if (moduleItem.itemData && moduleItem.itemData.name) {
                                                        if (!moduleItem.isInstalledThis) {
                                                            bibleManager.downloadModule(moduleItem.itemData.name)
                                                        } else {
                                                            moduleItem.openMenu()
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
