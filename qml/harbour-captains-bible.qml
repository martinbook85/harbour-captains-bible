import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import "pages"

ApplicationWindow {
    id: appWindow

    initialPage: Component { FirstPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations

    // Configuration values for active passage
    ConfigurationValue { id: confBookId; key: "/apps/harbour-captains-bible/selectedBookId"; defaultValue: "Gen" }
    ConfigurationValue { id: confBookName; key: "/apps/harbour-captains-bible/selectedBookName"; defaultValue: "Genesis" }
    ConfigurationValue { id: confChapter; key: "/apps/harbour-captains-bible/selectedChapter"; defaultValue: 1 }
    ConfigurationValue { id: confTargetVerse; key: "/apps/harbour-captains-bible/targetVerse"; defaultValue: 0 }

    // Navigation bridge called from CoverPage when the play button is pressed
    function jumpToPassage(bookId, bookName, chapter, verse) {
        // 1. Bring application window to foreground
        appWindow.activate()

        // 2. Set persistent configuration keys (acting as fallback/sync)
        if (bookId) confBookId.value = bookId
        if (bookName) confBookName.value = bookName
        if (chapter !== undefined) confChapter.value = chapter
        confTargetVerse.value = verse || 0

        // 3. Pop all pushed sub-pages back to root FirstPage immediately
        pageStack.pop(null, PageStackAction.Immediate)

        // 4. Force FirstPage to execute navigation directly with explicit arguments
        var firstPage = pageStack.firstPage
        if (firstPage && typeof firstPage.navigateToPassage === "function") {
            firstPage.navigateToPassage(bookId, bookName, chapter, verse || 1)
        }
    }
}
