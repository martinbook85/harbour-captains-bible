import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property string bookId: "Gen"
    property string bookName: "Genesis"
    property int chapter: 1

    property var commentaryList: []
    property string selectedCommentaryModule: ""
    property string commentaryHtml: ""

    Component.onCompleted: {
        loadCommentaries()
    }

    function loadCommentaries() {
        commentaryList = bibleManager.getInstalledCommentaries()
        if (commentaryList && commentaryList.length > 0) {
            if (selectedCommentaryModule === "") {
                var firstItem = commentaryList[0]
                selectedCommentaryModule = (typeof firstItem === "object")
                                           ? (firstItem.moduleName || firstItem["moduleName"] || "")
                                           : firstItem
            }
            loadText()
        }
    }

    function loadText() {
        if (selectedCommentaryModule === "" || bookId === "") return
        commentaryHtml = bibleManager.renderCommentaryText(selectedCommentaryModule, bookId, chapter)
    }

    // Hjälpfunktion för att hämta och förkorta namnet på kommentar-modulen
    function getShortCommentaryName() {
        if (!selectedCommentaryModule) return ""

        var displayName = selectedCommentaryModule

        // Sök igenom commentaryList efter en matchande titel
        for (var i = 0; i < commentaryList.length; i++) {
            var item = commentaryList[i]
            if (typeof item === "object" && item.moduleName === selectedCommentaryModule) {
                displayName = item.title || item.moduleName
                break
            }
        }

        // Trimm texten om den är längre än 18 tecken för snyggare header
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
                        "currentModule": page.selectedCommentaryModule
                    })

                    pageObj.moduleSelected.connect(function(modName) {
                        page.selectedCommentaryModule = modName
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
                title: page.bookName + " " + page.chapter
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
                      : "<p style='color:gray; text-align:center;'>" + qsTr("No commentary installed or available for this chapter.") + "</p>"
                font.pixelSize: Theme.fontSizeMedium
            }
        }
    }
}
