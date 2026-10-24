import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: studyNotesPage
    allowedOrientations: Orientation.All

    property var notesList: []

    function refreshNotes() {
        notesList = bibleManager.getAllNotes()
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            refreshNotes()
        }
    }

    SilicaListView {
        anchors.fill: parent
        model: notesList

        PullDownMenu {
            MenuItem {
                text: qsTr("Add new study note")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("NoteEditorPage.qml"), {
                        "bookId": "",
                        "bookName": "",
                        "chapter": 0,
                        "verseStart": 0,
                        "verseEnd": 0,
                        "existingNote": null
                    })
                }
            }
        }

        header: PageHeader {
            title: qsTr("Study Notes")
        }

        ViewPlaceholder {
            enabled: !notesList || notesList.length === 0
            text: qsTr("No study notes saved yet")
            hintText: qsTr("Pull down to add a note or long-press any verse in Bible view")
        }

        delegate: ListItem {
            width: parent.width
            contentHeight: noteCol.height + (Theme.paddingSmall * 2)

            menu: ContextMenu {
                MenuItem {
                    text: qsTr("Delete")
                    onClicked: {
                        bibleManager.deleteNote(modelData.id)
                        studyNotesPage.refreshNotes()
                    }
                }
            }

            onClicked: {
                pageStack.push(Qt.resolvedUrl("NoteEditorPage.qml"), {
                    "bookId": modelData.bookId,
                    "bookName": modelData.bookId,
                    "chapter": modelData.chapter,
                    "verseStart": modelData.verseStart,
                    "verseEnd": modelData.verseEnd,
                    "existingNote": modelData
                })
            }

            Column {
                id: noteCol
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.centerIn: parent
                spacing: Theme.paddingExtraSmall

                Row {
                    width: parent.width
                    spacing: Theme.paddingSmall

                    Label {
                        text: modelData.title || ""
                        color: Theme.highlightColor
                        font.bold: true
                        font.pixelSize: Theme.fontSizeSmall
                    }

                    Label {
                        text: "[" + (modelData.topic || "Verse Notes") + "]"
                        color: Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeExtraSmall
                    }
                }

                Label {
                    width: parent.width
                    text: modelData.body || ""
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    wrapMode: Text.Wrap
                    maximumLineCount: 3
                    truncationMode: TruncationMode.Fade
                }
            }
        }
    }
}
