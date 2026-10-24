import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import Nemo.KeepAlive 1.2

Page {
    id: page
    allowedOrientations: Orientation.All

    // Persistent state storage
    ConfigurationValue { id: confModule; key: "/apps/harbour-captains-bible/selectedModule"; defaultValue: "" }
    ConfigurationValue { id: confBookId; key: "/apps/harbour-captains-bible/selectedBookId"; defaultValue: "Gen" }
    ConfigurationValue { id: confBookName; key: "/apps/harbour-captains-bible/selectedBookName"; defaultValue: "Genesis" }
    ConfigurationValue { id: confChapter; key: "/apps/harbour-captains-bible/selectedChapter"; defaultValue: 1 }

    // Persistent bookmarks storage
    ConfigurationValue { id: confBookmarks; key: "/apps/harbour-captains-bible/bookmarks"; defaultValue: "[]" }
    ConfigurationValue { id: confTargetVerse; key: "/apps/harbour-captains-bible/targetVerse"; defaultValue: 0 }

    ConfigurationValue { id: confRefPlacement; key: "/apps/harbour-captains-bible/refPlacement"; defaultValue: 0 }
    ConfigurationValue { id: confShowVerseNums; key: "/apps/harbour-captains-bible/showVerseNums"; defaultValue: true }
    ConfigurationValue { id: confDelimiter; key: "/apps/harbour-captains-bible/refDelimiter"; defaultValue: ":" }

    ConfigurationValue {
        id: confKeepScreenOn
        key: "/apps/harbour-captains-bible/keepScreenOn"
        defaultValue: false
    }

    DisplayBlanking {
        // Prevents screen from blanking when setting is used AND page is active
        preventBlanking: confKeepScreenOn.value && page.status === PageStatus.Active
    }

    property string selectedModule: confModule.value
    property string selectedBookId: confBookId.value
    property string selectedBookName: confBookName.value
    property int selectedChapter: confChapter.value

    property var biblesList: []

    // Array holding the 3 visual slot datasets: slot 0, slot 1, slot 2
    property var bufferSlots: [
        { bookId: "", bookName: "", chapter: 0, verses: [] },
        { bookId: "", bookName: "", chapter: 0, verses: [] },
        { bookId: "", bookName: "", chapter: 0, verses: [] }
    ]

    property bool isUpdatingBuffer: false

    onSelectedModuleChanged: confModule.value = selectedModule
    onSelectedBookIdChanged: confBookId.value = selectedBookId
    onSelectedBookNameChanged: confBookName.value = selectedBookName
    onSelectedChapterChanged: confChapter.value = selectedChapter

    onStatusChanged: {
        if (status === PageStatus.Active) {
            if (confBookId.value !== selectedBookId || confChapter.value !== selectedChapter) {
                selectedBookId = confBookId.value
                selectedBookName = confBookName.value
                selectedChapter = confChapter.value
            }
            bibleManager.refreshModules()
            loadBibles()
        }
    }

    Component.onCompleted: {
        loadBibles()
    }

    function loadBibles() {
        biblesList = bibleManager.getInstalledBibles()
        if (biblesList && biblesList.length > 0) {
            if (selectedModule === "") {
                var firstItem = biblesList[0]
                selectedModule = (typeof firstItem === "object")
                                 ? (firstItem.moduleName || firstItem["moduleName"] || "")
                                 : firstItem
            }
            initializeBuffer()
        }
    }

    function getAdjacentChapterRef(bookId, bookName, chapter, delta) {
        if (!selectedModule || !bookId) return null

        var targetChapter = chapter + delta
        var totalChapters = bibleManager.getChapterCount(selectedModule, bookId)

        if (targetChapter >= 1 && targetChapter <= totalChapters) {
            return { bookId: bookId, bookName: bookName, chapter: targetChapter }
        }

        var books = bibleManager.getBibleBooks(selectedModule)
        for (var i = 0; i < books.length; i++) {
            if (books[i].bookId === bookId) {
                if (delta > 0 && i + 1 < books.length) {
                    return { bookId: books[i + 1].bookId, bookName: books[i + 1].bookName, chapter: 1 }
                } else if (delta < 0 && i - 1 >= 0) {
                    var prevBook = books[i - 1]
                    var prevTotal = bibleManager.getChapterCount(selectedModule, prevBook.bookId)
                    return { bookId: prevBook.bookId, bookName: prevBook.bookName, chapter: prevTotal > 0 ? prevTotal : 1 }
                }
                break
            }
        }
        return null
    }

    // Called on fresh passage jump or app load
    function initializeBuffer() {
        if (selectedModule === "" || selectedBookId === "") return

        isUpdatingBuffer = true

        var currVerses = bibleManager.getChapterVerses(selectedModule, selectedBookId, selectedChapter)
        var currData = { bookId: selectedBookId, bookName: selectedBookName, chapter: selectedChapter, verses: currVerses }

        var prevRef = getAdjacentChapterRef(selectedBookId, selectedBookName, selectedChapter, -1)
        var prevData = prevRef ? { bookId: prevRef.bookId, bookName: prevRef.bookName, chapter: prevRef.chapter, verses: bibleManager.getChapterVerses(selectedModule, prevRef.bookId, prevRef.chapter) }
                               : { bookId: "", bookName: "", chapter: 0, verses: [] }

        var nextRef = getAdjacentChapterRef(selectedBookId, selectedBookName, selectedChapter, 1)
        var nextData = nextRef ? { bookId: nextRef.bookId, bookName: nextRef.bookName, chapter: nextRef.chapter, verses: bibleManager.getChapterVerses(selectedModule, nextRef.bookId, nextRef.chapter) }
                               : { bookId: "", bookName: "", chapter: 0, verses: [] }

        // Slot 0 = Previous, Slot 1 = Current, Slot 2 = Next
        bufferSlots = [prevData, currData, nextData]
        slideshow.currentIndex = 1

        isUpdatingBuffer = false
    }

    // Called after a swipe gesture completes to populate the off-screen slot
    function updateBufferCircular(direction) {
        if (isUpdatingBuffer) return
        isUpdatingBuffer = true

        var activeSlot = slideshow.currentIndex // 0, 1, or 2
        var prevSlot = (activeSlot + 2) % 3
        var nextSlot = (activeSlot + 1) % 3

        // Declared once at function top to prevent QML static analyzer warning
        var updated = [bufferSlots[0], bufferSlots[1], bufferSlots[2]]

        // Load new outer boundary in background
        if (direction > 0) { // Moved Next
            var nextRef = getAdjacentChapterRef(selectedBookId, selectedBookName, selectedChapter, 1)
            var nextData = nextRef ? { bookId: nextRef.bookId, bookName: nextRef.bookName, chapter: nextRef.chapter, verses: bibleManager.getChapterVerses(selectedModule, nextRef.bookId, nextRef.chapter) }
                                   : { bookId: "", bookName: "", chapter: 0, verses: [] }

            updated[nextSlot] = nextData
            bufferSlots = updated
        } else if (direction < 0) { // Moved Previous
            var prevRef = getAdjacentChapterRef(selectedBookId, selectedBookName, selectedChapter, -1)
            var prevData = prevRef ? { bookId: prevRef.bookId, bookName: prevRef.bookName, chapter: prevRef.chapter, verses: bibleManager.getChapterVerses(selectedModule, prevRef.bookId, prevRef.chapter) }
                                   : { bookId: "", bookName: "", chapter: 0, verses: [] }

            updated[prevSlot] = prevData
            bufferSlots = updated
        }

        isUpdatingBuffer = false
    }

    function isBookmarked(vData) {
        if (!vData || typeof vData.verse === "undefined") return false
        var list = []
        try { list = JSON.parse(confBookmarks.value) } catch (e) { list = [] }
        for (var i = 0; i < list.length; i++) {
            if (list[i].module === selectedModule &&
                list[i].bookId === selectedBookId &&
                list[i].chapter === selectedChapter &&
                list[i].verse === vData.verse) {
                return true
            }
        }
        return false
    }

    function toggleBookmark(vData) {
        if (!vData || typeof vData.verse === "undefined") return
        var list = []
        try { list = JSON.parse(confBookmarks.value) } catch (e) { list = [] }
        var vNum = vData.verse
        var foundIndex = -1

        for (var i = 0; i < list.length; i++) {
            if (list[i].module === selectedModule &&
                list[i].bookId === selectedBookId &&
                list[i].chapter === selectedChapter &&
                list[i].verse === vNum) {
                foundIndex = i
                break
            }
        }

        if (foundIndex >= 0) {
            list.splice(foundIndex, 1)
        } else {
            list.push({
                "module": selectedModule,
                "bookId": selectedBookId,
                "bookName": selectedBookName,
                "chapter": selectedChapter,
                "verse": vNum,
                "text": vData.text || ""
            })
        }
        confBookmarks.value = JSON.stringify(list)
    }

    // Formats MyBible Strong's markup variants into clickable QML links
    function formatStrongLinks(rawText) {
        if (!rawText) return ""

        var formatted = rawText

        // 1. Matches <S>H7225</S>, <S>G746</S>, or <S>7225</S>
        formatted = formatted.replace(/<S>([HG]?\d+)<\/S>/gi, function(match, code) {
            return ' <a href="strong://' + code + '" style="text-decoration:none;"><font color="' + Theme.highlightColor + '"><sup>' + code + '</sup></font></a>'
        })

        // 2. Matches <w S="H7225">word</w>
        formatted = formatted.replace(/<w\s+S="([HG]?\d+)">([^<]+)<\/w>/gi, function(match, code, word) {
            return word + ' <a href="strong://' + code + '" style="text-decoration:none;"><font color="' + Theme.highlightColor + '"><sup>' + code + '</sup></font></a>'
        })

        // 3. Matches <f>S:H7225</f>
        formatted = formatted.replace(/<f>S:([HG]?\d+)<\/f>/gi, function(match, code) {
            return ' <a href="strong://' + code + '" style="text-decoration:none;"><font color="' + Theme.highlightColor + '"><sup>' + code + '</sup></font></a>'
        })

        return formatted
    }

    SlideshowView {
        id: slideshow
        anchors.fill: parent
        model: 3
        currentIndex: 1
        clip: true

        itemWidth: width
        itemHeight: height

        property int activeIndex: 1

        onMovementEnded: {
            if (isUpdatingBuffer) return

            if (currentIndex !== activeIndex) {
                var delta = (currentIndex - activeIndex)
                if (delta === -2) delta = 1
                if (delta === 2) delta = -1

                var targetRef = getAdjacentChapterRef(selectedBookId, selectedBookName, selectedChapter, delta)
                if (targetRef) {
                    selectedBookId = targetRef.bookId
                    selectedBookName = targetRef.bookName
                    selectedChapter = targetRef.chapter
                    activeIndex = currentIndex
                    updateBufferCircular(delta)
                } else {
                    slideshow.currentIndex = activeIndex
                }
            }
        }

        delegate: SilicaFlickable {
            width: slideshow.width
            height: slideshow.height

            property var pageData: page.bufferSlots[index] ? page.bufferSlots[index] : { bookId: "", bookName: "", chapter: 0, verses: [] }

            contentHeight: contentColumn.height + Theme.paddingLarge

            PullDownMenu {
                MenuItem {
                    text: qsTr("Get more modules")
                    onClicked: pageStack.push(Qt.resolvedUrl("ModuleDownloaderPage.qml"))
                }

                MenuItem {
                    text: qsTr("Select Bible translation")
                    enabled: page.biblesList.length > 0
                    onClicked: {
                        var pageObj = pageStack.push(Qt.resolvedUrl("ModuleSelectPage.qml"), {
                            "biblesList": page.biblesList,
                            "currentModule": page.selectedModule
                        })
                        pageObj.moduleSelected.connect(function(modName) {
                            page.selectedModule = modName
                            page.initializeBuffer()
                        })
                    }
                }

                MenuItem {
                    text: qsTr("Bookmarks")
                    onClicked: pageStack.push(Qt.resolvedUrl("BookmarksPage.qml"))
                }

                MenuItem {
                    text: qsTr("Search")
                    enabled: page.selectedModule !== ""
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("SearchPage.qml"), {
                            "currentModule": page.selectedModule,
                            "currentBookId": page.selectedBookId
                        })
                    }
                }

                MenuItem {
                    text: qsTr("Choose passage")
                    enabled: page.selectedModule !== ""
                    onClicked: {
                        var dialog = pageStack.push(Qt.resolvedUrl("SelectPage.qml"), {
                            "moduleName": page.selectedModule
                        })
                        dialog.passageSelected.connect(function(bookId, bookName, chapter) {
                            page.selectedBookId = bookId
                            page.selectedBookName = bookName
                            page.selectedChapter = chapter
                            page.initializeBuffer()
                        })
                    }
                }
            }

            PushUpMenu {
                MenuItem {
                    text: qsTr("Commentary")
                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("CommentaryPage.qml"), {
                            "selectedBibleModule": page.selectedModule, // Pass active Bible module
                            "bookId": page.selectedBookId,
                            "bookName": page.selectedBookName,
                            "chapter": page.selectedChapter
                        })
                    }
                }

                MenuItem {
                    text: qsTr("Dictionary")
                    onClicked: pageStack.push(Qt.resolvedUrl("DictionaryPage.qml"))
                }

                MenuItem {
                    text: qsTr("Bible Maps")
                    onClicked: pageStack.push(Qt.resolvedUrl("MapPage.qml"))
                }

                MenuItem {
                        text: qsTr("Study Notes")
                        onClicked: pageStack.push(Qt.resolvedUrl("StudyNotesPage.qml"))
                    }

                MenuItem {
                    text: qsTr("Settings")
                    onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
                }
            }

            Column {
                id: contentColumn
                width: parent.width
                spacing: Theme.paddingMedium

                PageHeader {
                    title: pageData.bookName !== ""
                           ? (pageData.bookName + " " + pageData.chapter)
                           : qsTr("Captain's Bible")
                }

                Label {
                    width: parent.width - (2 * Theme.horizontalPageMargin)
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: !pageData.verses || pageData.verses.length === 0
                    text: "<p style='color:gray; text-align:center;'>" + qsTr("No text available or select a passage from the menu.") + "</p>"
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                    font.pixelSize: Theme.fontSizeMedium
                }

                Column {
                    width: parent.width - (2 * Theme.horizontalPageMargin)
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: Theme.paddingMedium

                    Repeater {
                        model: pageData.verses ? pageData.verses : []

                        delegate: ListItem {
                            id: verseDelegate
                            width: parent.width
                            contentHeight: verseCol.implicitHeight + Theme.paddingSmall

                            property bool expanded: false
                            property var vData: (modelData && typeof modelData === "object") ? modelData : ({})
                            property bool hasNote: !!(vData && vData.hasFootnote)
                            property bool bookmarked: page.isBookmarked(vData)

                            menu: ContextMenu {
                                id: verseContextMenu

                                // Quick action icon row
                                Row {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    spacing: Theme.paddingLarge
                                    height: Theme.itemSizeSmall

                                    IconButton {
                                        icon.source: verseDelegate.bookmarked
                                                     ? "image://theme/icon-m-attach"
                                                     : "image://theme/icon-m-attach?color=" + Theme.secondaryColor
                                        onClicked: {
                                            page.toggleBookmark(verseDelegate.vData)
                                            verseDelegate.bookmarked = page.isBookmarked(verseDelegate.vData)
                                            verseContextMenu.close()
                                        }
                                    }

                                    IconButton {
                                        icon.source: "image://theme/icon-m-clipboard"
                                        onClicked: {
                                            var vNum = verseDelegate.vData.verse
                                            var formattedStr = bibleManager.formatVersesForClipboard(
                                                page.selectedModule,
                                                page.selectedBookId,
                                                page.selectedBookName,
                                                page.selectedChapter,
                                                vNum,
                                                vNum,
                                                confRefPlacement.value,
                                                confShowVerseNums.value,
                                                confDelimiter.value,
                                                "–"
                                            )
                                            Clipboard.text = formattedStr
                                            verseContextMenu.close()
                                        }
                                    }

                                    IconButton {
                                        icon.source: "image://theme/icon-m-edit"
                                        onClicked: {
                                            var vNum = verseDelegate.vData.verse
                                            var existingNote = bibleManager.getNoteForVerse(page.selectedBookId, page.selectedChapter, vNum)

                                            pageStack.push(Qt.resolvedUrl("NoteEditorPage.qml"), {
                                                "bookId": page.selectedBookId,
                                                "bookName": page.selectedBookName,
                                                "chapter": page.selectedChapter,
                                                "verseStart": vNum,
                                                "verseEnd": vNum,
                                                "existingNote": existingNote
                                            })
                                            verseContextMenu.close()
                                        }
                                    }
                                }

                                MenuItem {
                                    text: qsTr("Verse commentaries")
                                    onClicked: {
                                        pageStack.push(Qt.resolvedUrl("VerseCommentaryPage.qml"), {
                                            "selectedBibleModule": page.selectedModule,
                                            "bookId": page.selectedBookId,
                                            "bookName": page.selectedBookName,
                                            "chapter": page.selectedChapter,
                                            "verse": model.verse !== undefined ? model.verse : (index + 1)
                                        })
                                    }
                                }
                            }

                            onClicked: {
                                if (verseDelegate.hasNote) {
                                    verseDelegate.expanded = !verseDelegate.expanded
                                }
                            }

                            Column {
                                id: verseCol
                                width: parent.width
                                anchors.top: parent.top
                                anchors.topMargin: Theme.paddingExtraSmall
                                spacing: Theme.paddingExtraSmall

                                Label {
                                    id: verseLabel
                                    width: parent.width
                                    wrapMode: Text.Wrap
                                    textFormat: Text.RichText
                                    font.pixelSize: Theme.fontSizeMedium
                                    color: verseDelegate.bookmarked ? Theme.highlightColor : Theme.primaryColor

                                    text: {
                                        if (!verseDelegate.vData || typeof verseDelegate.vData.verse === "undefined") return ""

                                        var vNum = verseDelegate.vData.verse
                                        var vNumFormatted = verseDelegate.hasNote
                                                            ? "<u><b>" + vNum + "</b></u>"
                                                            : "<b>" + vNum + "</b>"

                                        var formattedText = page.formatStrongLinks(verseDelegate.vData.text || "")

                                        // FELSÖKNING: Skriv ut den genererade HTML-texten för vers 1
                                        if (vNum === 1) {
                                            console.log("[FirstPage] Formatterad text för vers 1:", formattedText)
                                        }

                                        return vNumFormatted + " " + formattedText
                                    }

                                    onLinkActivated: {
                                        var linkStr = link.toString()
                                        console.log("[FirstPage] LINK ACTIVATED! URL:", linkStr)

                                        if (linkStr.indexOf("strong://") === 0) {
                                            var strongCode = linkStr.replace("strong://", "")
                                            console.log("[FirstPage] Navigating to DictionaryPage with code:", strongCode)
                                            pageStack.push(Qt.resolvedUrl("DictionaryPage.qml"), {
                                                "initialTopic": strongCode
                                            })
                                        }
                                    }
                                }

                                Label {
                                    id: noteLabel
                                    width: parent.width
                                    wrapMode: Text.Wrap
                                    textFormat: Text.RichText
                                    font.pixelSize: Theme.fontSizeExtraSmall
                                    font.italic: true
                                    color: Theme.secondaryColor
                                    visible: verseDelegate.expanded && verseDelegate.hasNote
                                    text: (verseDelegate.vData && verseDelegate.vData.footnote) ? verseDelegate.vData.footnote : ""
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
