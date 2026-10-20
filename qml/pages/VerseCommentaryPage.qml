import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property string selectedBibleModule: ""
    property string bookId: "Gen"
    property string bookName: "Genesis"
    property int chapter: 1
    property int verse: 1

    property var commentaryList: []
    property string commentaryHtml: ""

    // Persistent storage for user's commentary choice across sessions and page navigations
    ConfigurationValue {
        id: selectedCommentarySetting
        key: "/apps/harbour-captains-bible/lastVerseCommentaryModule"
        defaultValue: ""
    }

    property string activeCommentaryModule: selectedCommentarySetting.value

    Component.onCompleted: {
        loadCommentaries()
    }

    function loadCommentaries() {
        var rawList = bibleManager.getCommentariesForVerse(bookId, chapter, verse)

        if (rawList && rawList.length > 0) {
            commentaryList = rawList

            // If no module was previously selected, default to the first available module
            if (activeCommentaryModule === "") {
                activeCommentaryModule = commentaryList[0].moduleName
                selectedCommentarySetting.value = activeCommentaryModule
            }

            loadText()
        }
    }

    function loadText() {
        if (activeCommentaryModule === "" || bookId === "") return

        var result = bibleManager.getVerseCommentaryText(
            activeCommentaryModule,
            selectedBibleModule,
            bookId,
            chapter,
            verse
        )

        commentaryHtml = result
    }

    function getShortCommentaryName() {
        if (!activeCommentaryModule) return ""

        var displayName = activeCommentaryModule

        for (var i = 0; i < commentaryList.length; i++) {
            var item = commentaryList[i]
            if (typeof item === "object" && item.moduleName === activeCommentaryModule) {
                displayName = item.title || item.moduleName
                break
            }
        }

        if (displayName.length > 18) {
            return displayName.substring(0, 16) + "..."
        }
        return displayName
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        PullDownMenu {
            MenuItem {
                text: qsTr("Select Commentary")
                enabled: page.commentaryList.length > 0
                onClicked: {
                    var pageObj = pageStack.push(Qt.resolvedUrl("ModuleSelectPage.qml"), {
                        "biblesList": page.commentaryList,
                        "currentModule": page.activeCommentaryModule,
                        "pageTitle": qsTr("Verse Commentaries")
                    })

                    pageObj.moduleSelected.connect(function(modName) {
                        page.activeCommentaryModule = modName
                        selectedCommentarySetting.value = modName
                        page.loadText()
                    })
                }
            }
        }

        Column {
            id: column
            width: page.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: page.bookName + " " + page.chapter + ":" + page.verse
                description: getShortCommentaryName()
            }

            Label {
                id: textDisplay
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.horizontalCenter: parent.horizontalCenter
                textFormat: Text.RichText
                wrapMode: Text.Wrap
                text: page.commentaryHtml !== ""
                      ? page.commentaryHtml
                      : "<p style='color:gray; text-align:center;'>" + qsTr("No comment in this commentary for this particular verse.") + "</p>"
                font.pixelSize: Theme.fontSizeMedium
            }
        }
    }
}
