import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property string moduleName: ""
    property string selectedBookId: ""
    property string selectedBookName: ""
    property int selectedChapter: 1

    signal passageSelected(string bookId, string bookName, int chapter)

    property var booksList: []
    property int totalChapters: 0
    property int totalVerses: 0

    Component.onCompleted: {
        if (moduleName !== "") {
            booksList = bibleManager.getBibleBooks(moduleName)
            if (booksList.length > 0) {
                selectBook(booksList[0].bookId, booksList[0].bookName)
            }
        }
    }

    function selectBook(bId, bName) {
        selectedBookId = bId
        selectedBookName = bName
        totalChapters = bibleManager.getChapterCount(moduleName, selectedBookId)
        selectChapter(1)
    }

    function selectChapter(chap) {
        selectedChapter = chap
        totalVerses = bibleManager.getVerseCount(moduleName, selectedBookId, selectedChapter)
    }

    PageHeader {
        id: header
        title: qsTr("Select Passage")
    }

    Row {
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        readonly property real colWidth: width / 3

        // --- KOLUMN 1: BÖCKER ---
        SilicaListView {
            width: parent.colWidth
            height: parent.height
            model: page.booksList
            delegate: ListItem {
                contentHeight: Theme.itemSizeSmall
                onClicked: page.selectBook(modelData.bookId, modelData.bookName)

                Label {
                    anchors.centerIn: parent
                    text: modelData.bookName
                    color: page.selectedBookId === modelData.bookId ? Theme.highlightColor : Theme.primaryColor
                    font.bold: page.selectedBookId === modelData.bookId
                    font.pixelSize: Theme.fontSizeExtraSmall
                    truncationMode: TruncationMode.Fade
                    width: parent.width - Theme.paddingSmall
                    horizontalAlignment: Text.AlignHCenter
                }
            }
            VerticalScrollDecorator {}
        }

        // --- KOLUMN 2: KAPITEL ---
        SilicaListView {
            width: parent.colWidth
            height: parent.height
            model: page.totalChapters
            delegate: ListItem {
                contentHeight: Theme.itemSizeSmall
                onClicked: page.selectChapter(index + 1)

                Label {
                    anchors.centerIn: parent
                    text: (index + 1).toString()
                    color: page.selectedChapter === (index + 1) ? Theme.highlightColor : Theme.primaryColor
                    font.bold: page.selectedChapter === (index + 1)
                    font.pixelSize: Theme.fontSizeMedium
                }
            }
            VerticalScrollDecorator {}
        }

        // --- KOLUMN 3: VERSER ---
        SilicaListView {
            width: parent.colWidth
            height: parent.height
            model: page.totalVerses
            delegate: ListItem {
                contentHeight: Theme.itemSizeSmall
                onClicked: {
                    page.passageSelected(page.selectedBookId, page.selectedBookName, page.selectedChapter)
                    pageStack.pop()
                }

                Label {
                    anchors.centerIn: parent
                    text: (index + 1).toString()
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeMedium
                }
            }
            VerticalScrollDecorator {}
        }
    }
}
