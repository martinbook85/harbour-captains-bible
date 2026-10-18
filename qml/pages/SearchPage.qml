import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property string currentModule: ""
    property string currentBookId: "Gen"
    property int currentScopeIndex: 9 // Default: Entire Bible
    property bool exactMatch: false

    property var searchResults: []
    property int currentOffset: 0
    property int pageLimit: 100
    property bool hasMoreResults: false

    function performSearch(term, isNewSearch) {
        var cleanTerm = term.trim()
        if (cleanTerm.length === 0 || currentModule === "") return

        if (isNewSearch) {
            currentOffset = 0
            searchResults = []
        }

        var newBatch = bibleManager.searchBible(
            currentModule,
            cleanTerm,
            currentScopeIndex,
            currentBookId,
            exactMatch,
            pageLimit,
            currentOffset
        )

        if (isNewSearch) {
            searchResults = newBatch
        } else {
            var combined = searchResults.slice()
            for (var i = 0; i < newBatch.length; i++) {
                combined.push(newBatch[i])
            }
            searchResults = combined
        }

        hasMoreResults = (newBatch.length === pageLimit)
    }

    SilicaListView {
        id: resultsListView
        anchors.fill: parent
        model: page.searchResults

        header: Column {
            width: page.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Search Bible")
                description: page.currentModule
            }

            SearchField {
                id: searchField
                width: parent.width
                placeholderText: qsTr("Enter search term...")

                EnterKey.enabled: text.trim().length > 0
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: {
                    searchField.focus = false
                    page.performSearch(searchField.text, true)
                }
            }

            ComboBox {
                width: parent.width
                label: qsTr("Search Scope")
                currentIndex: page.currentScopeIndex

                menu: ContextMenu {
                    MenuItem { text: qsTr("Current Book") }
                    MenuItem { text: qsTr("Old Testament") }
                    MenuItem { text: qsTr("Pentateuch") }
                    MenuItem { text: qsTr("Historic") }
                    MenuItem { text: qsTr("Poetic") }
                    MenuItem { text: qsTr("Prophets") }
                    MenuItem { text: qsTr("New Testament") }
                    MenuItem { text: qsTr("Gospels") }
                    MenuItem { text: qsTr("Epistles") }
                    MenuItem { text: qsTr("Entire Bible") }
                }

                onCurrentIndexChanged: {
                    page.currentScopeIndex = currentIndex
                    if (searchField.text.trim().length > 0) {
                        page.performSearch(true)
                    }
                }
            }

            TextSwitch {
                text: qsTr("Exact word match")
                checked: page.exactMatch
                onCheckedChanged: {
                    page.exactMatch = checked
                    if (searchField.text.trim().length > 0) {
                        page.performSearch(true)
                    }
                }
            }

            ValueButton {
                label: qsTr("Translation")
                value: page.currentModule
                onClicked: {
                    var selectObj = pageStack.push(Qt.resolvedUrl("ModuleSelectPage.qml"), {
                        "biblesList": bibleManager.getInstalledBibles(),
                        "currentModule": page.currentModule
                    })
                    selectObj.moduleSelected.connect(function(modName) {
                        page.currentModule = modName
                        if (searchField.text.trim().length > 0) {
                            page.performSearch(true)
                        }
                    })
                }
            }

            SectionHeader {
                text: page.searchResults.length > 0
                      ? qsTr("Results (%1)").arg(page.searchResults.length)
                      : qsTr("Results")
            }
        }

        delegate: BackgroundItem {
            id: delegateItem
            width: parent.width
            height: resultColumn.height + Theme.paddingMedium

            Column {
                id: resultColumn
                x: Theme.horizontalPageMargin
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.paddingSmall

                Label {
                    text: modelData.bookName + " " + modelData.chapter + ":" + modelData.verse
                    color: delegateItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                    font.bold: true
                    font.pixelSize: Theme.fontSizeSmall
                }

                Label {
                    width: parent.width
                    text: modelData.snippet
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.secondaryColor
                }
            }

            onClicked: {
                pageStack.push(Qt.resolvedUrl("SearchResultChapterPage.qml"), {
                    "moduleName": page.currentModule,
                    "bookId": modelData.bookId,
                    "bookName": modelData.bookName,
                    "chapter": modelData.chapter,
                    "targetVerse": modelData.verse
                })
            }
        }

        footer: Item {
            width: parent.width
            height: loadMoreButton.visible ? Theme.itemSizeMedium : Theme.paddingLarge

            BackgroundItem {
                id: loadMoreButton
                anchors.fill: parent
                visible: page.hasMoreResults

                Label {
                    anchors.centerIn: parent
                    text: qsTr("Load more results...")
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeMedium
                }

                onClicked: {
                    page.currentOffset += page.pageLimit
                    page.performSearch(false)
                }
            }
        }
    }
}
