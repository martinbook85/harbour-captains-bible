import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: studyNotesPage
    allowedOrientations: Orientation.All

    property var topicFolders: []

    function refreshTopics() {
        topicFolders = bibleManager.getTopicFolders()
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            refreshTopics()
        }
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: studyNotesPage.topicFolders

        // 1. DITT URSPRUNGLIGA PULLDOWNMENU BEHÅLLS
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
            enabled: !studyNotesPage.topicFolders || studyNotesPage.topicFolders.length === 0
            text: qsTr("No study notes saved yet")
            hintText: qsTr("Pull down to add a note or long-press any verse in Bible view")
        }

        delegate: BackgroundItem {
            id: topicDelegate
            width: parent.width
            height: folderColumn.implicitHeight + Theme.paddingMedium

            property bool expanded: false
            property string topicName: modelData.topicName || ""
            property var topicNotes: []

            function loadNotes() {
                topicNotes = bibleManager.getNotesForTopic(topicName)
            }

            onClicked: {
                expanded = !expanded
                if (expanded) {
                    loadNotes()
                }
            }

            Column {
                id: folderColumn
                width: parent.width
                spacing: Theme.paddingSmall

                // MAPP-RUBRIK (Mapp-ikon + Ämnesnamn + Antal)
                Row {
                    width: parent.width - (2 * Theme.horizontalPageMargin)
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: Theme.paddingMedium
                    height: Theme.itemSizeSmall

                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        source: topicDelegate.expanded
                                ? "image://theme/icon-m-folder-open"
                                : "image://theme/icon-m-folder"
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: topicDelegate.topicName === "" ? qsTr("Verse Notes") : topicDelegate.topicName
                        font.pixelSize: Theme.fontSizeMedium
                        color: topicDelegate.highlighted ? Theme.highlightColor : Theme.primaryColor
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "(" + (modelData.noteCount || 0) + ")"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.secondaryColor
                    }
                }

                // UNERMAPP: ANTECKNINGAR I DENNA MAPP
                Column {
                    width: parent.width
                    visible: topicDelegate.expanded
                    spacing: Theme.paddingExtraSmall

                    Repeater {
                        model: topicDelegate.topicNotes

                        delegate: ListItem {
                            id: noteListItem
                            width: parent.width
                            contentHeight: noteCol.height + (Theme.paddingSmall * 2)

                            // 2. DITT URSPRUNGLIGA DELETE-MENYVAL BEHÅLLS
                            menu: ContextMenu {
                                MenuItem {
                                    text: qsTr("Delete")
                                    onClicked: {
                                        bibleManager.deleteNote(modelData.id)
                                        topicDelegate.loadNotes()
                                        studyNotesPage.refreshTopics()
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

                            // 3. DIN URSPRUNGLIGA KOD FÖR RUBRIK OCH BRÖDTEXT BEHÅLLS
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
            }
        }
    }
}
