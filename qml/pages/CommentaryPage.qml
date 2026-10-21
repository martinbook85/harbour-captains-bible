import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Page {
    id: page
    allowedOrientations: Orientation.All

    property string bookId: "Gen"
    property string bookName: "Genesis"
    property int chapter: 1

    property var commentaryList: []
    property string commentaryHtml: ""

    // Spara senast valda kommentar-modul permanent i Sailfish dconf/settings
    ConfigurationValue {
        id: selectedCommentarySetting
        key: "/apps/harbour-captains-bible/lastCommentaryModule"
        defaultValue: ""
    }

    property string selectedCommentaryModule: selectedCommentarySetting.value

    Component.onCompleted: {
        loadCommentaries()
    }

    function loadCommentaries() {
        var rawList = bibleManager.getInstalledCommentaries()
        var enrichedList = []

        if (rawList && rawList.length > 0) {
            for (var i = 0; i < rawList.length; i++) {
                var item = rawList[i]
                var modName = ""
                var modTitle = ""

                if (typeof item === "object") {
                    modName = item.moduleName || item["moduleName"] || ""
                    modTitle = item.title || modName
                } else {
                    modName = item
                    modTitle = item
                }

                // Query C++ helper for the number of comments in this chapter
                var count = bibleManager.getCommentaryCount(modName, bookId, chapter)

                enrichedList.push({
                    "moduleName": modName,
                    "title": modTitle,
                    "commentCount": count,
                    // Format display title with a speech bubble indicator if comments exist
                    "displayTitle": count > 0
                                    ? modTitle + "  💬 (" + count + ")"
                                    : modTitle
                })
            }

            commentaryList = enrichedList

            // Om ingen modul är sparad sedan tidigare, välj den första i listan
            if (selectedCommentaryModule === "") {
                selectedCommentaryModule = commentaryList[0].moduleName
                selectedCommentarySetting.value = selectedCommentaryModule
            }
            loadText()
        }
    }

    function loadText() {
        if (selectedCommentaryModule === "" || bookId === "") return
        commentaryHtml = bibleManager.renderCommentaryText(selectedCommentaryModule, bookId, chapter)
    }

    // Helper function to fetch and truncate the active commentary module name for header display
    function getShortCommentaryName() {
        if (!selectedCommentaryModule) return ""

        var displayName = selectedCommentaryModule

        // Search commentaryList for a matching module title
        for (var i = 0; i < commentaryList.length; i++) {
            var item = commentaryList[i]
            if (typeof item === "object" && item.moduleName === selectedCommentaryModule) {
                displayName = item.title || item.moduleName
                break
            }
        }

        // Trim text if longer than 18 characters for a cleaner Sailfish PageHeader layout
        if (displayName.length > 18) {
            return displayName.substring(0, 16) + "..."
        }
        return displayName
    }

    SilicaFlickable {
        anchors.fill: parent
        // Använder implicitHeight för att säkerställa perfekt scrolling vid långa kommentarer
        contentHeight: column.implicitHeight + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: qsTr("Select Commentary")
                enabled: page.commentaryList.length > 0
                onClicked: {
                    var pageObj = pageStack.push(Qt.resolvedUrl("ModuleSelectPage.qml"), {
                        "biblesList": page.commentaryList,
                        "currentModule": page.selectedCommentaryModule,
                        "pageTitle": qsTr("Commentaries")
                    })

                    // Engångs-anslutning för att undvika minnesläckor eller dubblerade anrop
                    var onModuleSelected = function(modName) {
                        page.selectedCommentaryModule = modName
                        selectedCommentarySetting.value = modName // Spara valet permanent!
                        page.loadText()
                        pageObj.moduleSelected.disconnect(onModuleSelected)
                    }
                    pageObj.moduleSelected.connect(onModuleSelected)
                }
            }
        }

        Column {
            id: column
            width: page.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: page.bookName + " " + page.chapter
                description: page.getShortCommentaryName()
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
