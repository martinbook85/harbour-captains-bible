import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Page {
    id: noteEditorPage
    allowedOrientations: Orientation.All

    property string bookId: ""
    property string bookName: ""
    property int chapter: 0
    property int verseStart: 0
    property int verseEnd: 0
    property var existingNote: null

    property string passagePreviewText: ""
    property var availableTopics: []

    ConfigurationValue {
        id: confModule
        key: "/apps/harbour-captains-bible/selectedModule"
        defaultValue: ""
    }

    Component.onCompleted: {
        availableTopics = bibleManager.getNoteTopics()

        if (bookId !== "" && chapter > 0 && verseStart > 0) {
            var activeMod = confModule.value
            if (activeMod !== "") {
                var verses = bibleManager.getChapterVerses(activeMod, bookId, chapter)
                var compiledText = ""
                for (var i = 0; i < verses.length; i++) {
                    var v = verses[i]
                    var vNum = v.verse || (i + 1)
                    if (vNum >= verseStart && vNum <= verseEnd) {
                        var cleanText = (v.text || "").replace(/<[^>]*>/g, "").trim()
                        compiledText += (compiledText === "" ? "" : " ") + "<b>" + vNum + "</b> " + cleanText
                    }
                }
                passagePreviewText = compiledText
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: contentColumn.height + Theme.paddingLarge

        Column {
            id: contentColumn
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: existingNote && existingNote.id ? qsTr("Edit Note") : qsTr("New Study Note")
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: bookName !== "" && chapter > 0
                text: bookName + " " + chapter + ":" + (verseStart === verseEnd ? verseStart : (verseStart + "–" + verseEnd))
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeMedium
                font.bold: true
            }

            Column {
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.horizontalCenter: parent.horizontalCenter
                visible: passagePreviewText !== ""
                spacing: Theme.paddingExtraSmall

                Separator {
                    width: parent.width
                    color: Theme.highlightColor
                }

                Label {
                    width: parent.width
                    text: passagePreviewText
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    font.italic: true
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                }

                Separator {
                    width: parent.width
                    color: Theme.highlightColor
                }
            }

            // Topic / Folder Field
            Column {
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.horizontalCenter: parent.horizontalCenter

                TextField {
                    id: topicField
                    width: parent.width
                    label: qsTr("Topic / Folder")
                    placeholderText: qsTr("Create or choose topic")

                    // THIS IS THE UPDATED TEXT PROPERTY
                    text: (existingNote && existingNote.topic)
                          ? existingNote.topic
                          : (bookId !== "" ? "Verse Notes" : "")
                }

                Column {
                    width: parent.width
                    visible: topicField.text.length > 0 && topicField.focus

                    Repeater {
                        model: {
                            var query = topicField.text.toLowerCase()
                            var matches = []
                            for (var i = 0; i < availableTopics.length; i++) {
                                var top = availableTopics[i]
                                if (top.toLowerCase().indexOf(query) >= 0 && top.toLowerCase() !== query) {
                                    matches.push(top)
                                }
                            }
                            return matches.slice(0, 4)
                        }

                        delegate: BackgroundItem {
                            width: parent.width
                            height: Theme.itemSizeExtraSmall

                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: Theme.paddingMedium
                                text: modelData
                                color: Theme.highlightColor
                                font.pixelSize: Theme.fontSizeSmall
                            }

                            onClicked: {
                                topicField.text = modelData
                                topicField.focus = false
                            }
                        }
                    }
                }
            }

            TextField {
                id: titleField
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.horizontalCenter: parent.horizontalCenter
                label: qsTr("Note Title")
                text: (existingNote && existingNote.title)
                      ? existingNote.title
                      : (bookName !== "" ? (bookName + " " + chapter + ":" + (verseStart === verseEnd ? verseStart : (verseStart + "–" + verseEnd))) : "")
                placeholderText: qsTr("General Note Title")
            }

            TextArea {
                id: bodyArea
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.horizontalCenter: parent.horizontalCenter
                label: qsTr("Study Note")
                placeholderText: qsTr("Write your personal notes or comments here...")
                text: (existingNote && existingNote.body) ? existingNote.body : ""
                height: Math.max(implicitHeight, Theme.itemSizeHuge * 2)
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Save Note")
                onClicked: {
                    var topicVal = topicField.text.trim() !== "" ? topicField.text.trim() : "General Notes"
                    var titleVal = titleField.text.trim() !== "" ? titleField.text.trim() : "Untitled Note"
                    var bodyVal = bodyArea.text.trim()

                    if (bodyVal !== "") {
                        bibleManager.saveNote(
                            bookId !== "" ? bookId : "GENERAL",
                            chapter,
                            verseStart,
                            verseEnd,
                            topicVal,
                            titleVal,
                            bodyVal
                        )
                        pageStack.pop()
                    }
                }
            }
        }
    }
}
