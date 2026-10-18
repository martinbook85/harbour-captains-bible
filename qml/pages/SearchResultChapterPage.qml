import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property string moduleName: ""
    property string bookId: ""
    property string bookName: ""
    property int chapter: 1
    property int targetVerse: 1

    property var versesList: []

    Component.onCompleted: {
        loadChapter()
    }

    function loadChapter() {
        if (!moduleName || !bookId) return

        // Fetch raw verses array from C++
        versesList = bibleManager.getChapterVerses(moduleName, bookId, chapter)

        // Target scroll index calculation
        var scrollIndex = 0
        for (var i = 0; i < versesList.length; i++) {
            var item = versesList[i]
            var vNum = (typeof item === "object") ? item.verse : (i + 1)
            if (vNum === targetVerse) {
                scrollIndex = i
                break
            }
        }

        // Center view on the requested verse
        Qt.callLater(function() {
            verseListView.positionViewAtIndex(scrollIndex, ListView.Center)
        })
    }

    SilicaListView {
        id: verseListView
        anchors.fill: parent
        model: page.versesList

        header: PageHeader {
            title: page.bookName + " " + page.chapter
            description: page.moduleName
        }

        delegate: BackgroundItem {
            id: delegateItem
            width: parent.width
            height: verseColumn.height + Theme.paddingMedium

            property int currentVerseNum: (typeof modelData === "object") ? modelData.verse : (index + 1)
            property string currentVerseText: (typeof modelData === "object") ? modelData.text : modelData
            property bool isTargetVerse: currentVerseNum === page.targetVerse

            Column {
                id: verseColumn
                x: Theme.horizontalPageMargin
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.paddingExtraSmall

                Label {
                    width: parent.width
                    text: "<b>" + currentVerseNum + ".</b> " + currentVerseText
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeMedium
                    color: isTargetVerse ? Theme.highlightColor : Theme.primaryColor
                }
            }
        }
    }
}
