import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Page {
    id: bookmarksPage
    allowedOrientations: Orientation.All

    ConfigurationValue {
        id: confBookmarks
        key: "/apps/harbour-captains-bible/bookmarks"
        defaultValue: "[]"
    }

    ConfigurationValue { id: confBookId; key: "/apps/harbour-captains-bible/selectedBookId"; defaultValue: "Gen" }
    ConfigurationValue { id: confBookName; key: "/apps/harbour-captains-bible/selectedBookName"; defaultValue: "Genesis" }
    ConfigurationValue { id: confChapter; key: "/apps/harbour-captains-bible/selectedChapter"; defaultValue: 1 }
    ConfigurationValue { id: confTargetVerse; key: "/apps/harbour-captains-bible/targetVerse"; defaultValue: 0 }

    property var bookmarksList: []

    function refresh() {
        try {
            bookmarksList = JSON.parse(confBookmarks.value)
        } catch (e) {
            bookmarksList = []
        }
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            refresh()
        }
    }

    SilicaListView {
        anchors.fill: parent
        model: bookmarksList

        header: PageHeader {
            title: qsTr("Bookmarks")
        }

        ViewPlaceholder {
            enabled: !bookmarksList || bookmarksList.length === 0
            text: qsTr("No bookmarks saved yet")
            hintText: qsTr("Press and hold a verse to add a bookmark")
        }

        delegate: ListItem {
            width: parent.width
            contentHeight: col.height + (Theme.paddingSmall * 2)

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Remove")
                    onClicked: {
                        var list = JSON.parse(confBookmarks.value)
                        list.splice(index, 1)
                        confBookmarks.value = JSON.stringify(list)
                        bookmarksPage.refresh()
                    }
                }
            }

            onClicked: {
                confBookId.value = modelData.bookId
                confBookName.value = modelData.bookName
                confChapter.value = modelData.chapter
                confTargetVerse.value = modelData.verse || 0
                pageStack.pop()
            }

            Column {
                id: col
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.centerIn: parent
                spacing: Theme.paddingExtraSmall

                Label {
                    width: parent.width
                    text: modelData.bookName + " " + modelData.chapter + ":" + modelData.verse
                    color: Theme.highlightColor
                    font.bold: true
                    font.pixelSize: Theme.fontSizeSmall
                }

                Label {
                    width: parent.width
                    text: modelData.text || ""
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    truncationMode: TruncationMode.Fade
                }
            }
        }
    }
}
