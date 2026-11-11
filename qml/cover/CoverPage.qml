import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import "../pages"

CoverBackground {
    id: cover

    // Configuration Keys
    ConfigurationValue { id: confModule; key: "/apps/harbour-captains-bible/selectedModule"; defaultValue: "" }
    ConfigurationValue { id: confBookId; key: "/apps/harbour-captains-bible/selectedBookId"; defaultValue: "Gen" }
    ConfigurationValue { id: confBookName; key: "/apps/harbour-captains-bible/selectedBookName"; defaultValue: "Genesis" }
    ConfigurationValue { id: confChapter; key: "/apps/harbour-captains-bible/selectedChapter"; defaultValue: 1 }
    ConfigurationValue { id: confTargetVerse; key: "/apps/harbour-captains-bible/targetVerse"; defaultValue: 0 }

    ConfigurationValue { id: confCoverMode; key: "/apps/harbour-captains-bible/coverMode"; defaultValue: 0 } // 0: VOTD, 1: Bookmarks, 2: Ribbons, 3: Both
    ConfigurationValue { id: confRibbonCount; key: "/apps/harbour-captains-bible/ribbonCount"; defaultValue: 3 }

    // Ribbons Configuration Keys (1 to 8)
    ConfigurationValue { id: confRibbon1; key: "/apps/harbour-captains-bible/ribbon1"; defaultValue: "" }
    ConfigurationValue { id: confRibbon2; key: "/apps/harbour-captains-bible/ribbon2"; defaultValue: "" }
    ConfigurationValue { id: confRibbon3; key: "/apps/harbour-captains-bible/ribbon3"; defaultValue: "" }
    ConfigurationValue { id: confRibbon4; key: "/apps/harbour-captains-bible/ribbon4"; defaultValue: "" }
    ConfigurationValue { id: confRibbon5; key: "/apps/harbour-captains-bible/ribbon5"; defaultValue: "" }
    ConfigurationValue { id: confRibbon6; key: "/apps/harbour-captains-bible/ribbon6"; defaultValue: "" }
    ConfigurationValue { id: confRibbon7; key: "/apps/harbour-captains-bible/ribbon7"; defaultValue: "" }
    ConfigurationValue { id: confRibbon8; key: "/apps/harbour-captains-bible/ribbon8"; defaultValue: "" }

    ConfigurationValue {
        id: confBookmarks
        key: "/apps/harbour-captains-bible/bookmarks"
        defaultValue: "[]"
    }

    // Navigation & Data Properties
    property int daysOffset: 0
    property int currentListIndex: 0
    property var coverItems: []
    property var activeRibbonConfs: [confRibbon1, confRibbon2, confRibbon3, confRibbon4, confRibbon5, confRibbon6, confRibbon7, confRibbon8]
    property var ribbonColors: ["#E63946", "#457B9D", "#2A9D8F", "#E9C46A", "#F4A261", "#9C27B0", "#00BCD4", "#E91E63"]

    // Dynamic date calculation for Mode 0 (VOTD)
    property string currentDateStr: {
        var d = new Date()
        d.setDate(d.getDate() + daysOffset)
        return Qt.formatDate(d, "d MMMM yyyy")
    }

    // Deep-linking helper: Updates persistent state keys and activates window
    function openCurrentPassage() {
        var item = coverItems.length > 0 ? coverItems[currentListIndex] : null
        if (!item) return

        var targetBookId = item.bookId || "Gen"
        var targetBookName = item.bookName || ""
        var targetChapter = item.chapter || 1
        var targetVerse = item.verse || 0

        // Resolve display book name if missing
        if ((!targetBookName || targetBookName === "") && typeof bibleManager !== "undefined" && confModule.value !== "") {
            targetBookName = bibleManager.getBookName(confModule.value, targetBookId) || targetBookId
        }

        // Write directly to configuration storage
        confBookId.value = targetBookId
        confBookName.value = targetBookName
        confChapter.value = targetChapter
        confTargetVerse.value = targetVerse

        // Activate main app window; FirstPage's onStatusChanged will sync state
        if (typeof appWindow !== "undefined" && appWindow) {
            appWindow.activate()
        }
    }

    // Load active items depending on mode
    function loadCoverData() {
        var mode = confCoverMode.value
        if (mode === 0) {
            updateVotd(daysOffset)
            return
        }

        var list = []
        var i = 0

        // 1. Load User Bookmarks from JSON (Mode 1 & Mode 3)
        if (mode === 1 || mode === 3) {
            var parsedBms = []
            try {
                parsedBms = JSON.parse(confBookmarks.value)
            } catch (e) {
                parsedBms = []
            }

            for (i = 0; i < parsedBms.length; i++) {
                var bm = parsedBms[i]
                var bId = bm.bookId || "Gen"
                var bName = bm.bookName || bId
                var ch = bm.chapter !== undefined ? bm.chapter : 1
                var v = bm.verse !== undefined ? bm.verse : 1
                var txt = bm.text || ""

                list.push({
                    type: "bookmark",
                    ref: bName + " " + ch + (v > 0 ? ":" + v : ""),
                    text: txt,
                    bookId: bId,
                    bookName: bName,
                    chapter: parseInt(ch),
                    verse: parseInt(v)
                })
            }
        }

        // 2. Load Active Ribbons (Mode 2 & Mode 3)
        if (mode === 2 || mode === 3) {
            var count = Math.min(Math.max(1, confRibbonCount.value), 8)
            for (i = 0; i < count; i++) {
                var conf = activeRibbonConfs[i]
                if (conf && conf.value && conf.value !== "") {
                    var parts = conf.value.split("|") // Format: "bookId|bookName|chapter"
                    if (parts.length >= 3) {
                        list.push({
                            type: "ribbon",
                            id: i + 1,
                            color: ribbonColors[i],
                            ref: parts[1] + " " + parts[2],
                            bookId: parts[0],
                            bookName: parts[1],
                            chapter: parseInt(parts[2]),
                            verse: 1
                        })
                    }
                }
            }
        }

        coverItems = list
        if (currentListIndex >= coverItems.length) {
            currentListIndex = 0
        }
    }

    function updateVotd(offset) {
        daysOffset = offset
        var modName = confModule.value
        if (typeof bibleManager !== "undefined" && modName && modName.length > 0) {
            var votd = bibleManager.getVerseByOffset(modName, daysOffset)
            if (votd && (votd.reference || votd.text)) {
                var bId = votd.bookId || "Gen"
                var bName = votd.bookName || (typeof bibleManager.getBookName === "function" ? bibleManager.getBookName(modName, bId) : bId)
                var ch = votd.chapter || 1
                var v = votd.verse || 1

                coverItems = [{
                    type: "votd",
                    ref: votd.reference || (bName + " " + ch + ":" + v),
                    text: votd.text || votd.content || votd.verseText || qsTr("No verse text available."),
                    bookId: bId,
                    bookName: bName,
                    chapter: parseInt(ch),
                    verse: parseInt(v)
                }]
            } else {
                coverItems = [{ type: "votd", ref: qsTr("Verse of the Day"), text: qsTr("No verse available.") }]
            }
        } else {
            coverItems = [{ type: "votd", ref: qsTr("Captain's Bible"), text: qsTr("Select a Bible translation from the menu to activate Verse of the Day.") }]
        }
        currentListIndex = 0
    }

    Component.onCompleted: {
        loadCoverData()
    }

    onStatusChanged: {
        if (status === Cover.Active) {
            loadCoverData()
        }
    }

    // Background watermark icon
    Image {
        id: iconOutline
        anchors.centerIn: parent
        width: parent.width * 0.80
        height: width
        source: Qt.resolvedUrl("172icon.png")
        opacity: 0.25
        fillMode: Image.PreserveAspectFit
    }

    Column {
        anchors {
            fill: parent
            margins: Theme.paddingMedium
            bottomMargin: Theme.itemSizeNormal
        }
        spacing: Theme.paddingExtraSmall

        // App Title
        Label {
            width: parent.width
            text: qsTr("Captain's Bible")
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeExtraSmall
            horizontalAlignment: Text.AlignHCenter
        }

        // Subheader (Date for VOTD, Mode title for Bookmarks/Ribbons)
        Label {
            width: parent.width
            text: confCoverMode.value === 0
                  ? currentDateStr
                  : (confCoverMode.value === 1 ? qsTr("Bookmarks")
                     : (confCoverMode.value === 2 ? qsTr("Ribbons") : qsTr("Bookmarks & Ribbons")))
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall - 2
            horizontalAlignment: Text.AlignHCenter
        }

        Separator {
            width: parent.width
            color: Theme.highlightColor
            horizontalAlignment: Qt.AlignHCenter
        }

        // Main Content Area
        Item {
            width: parent.width
            height: parent.height - y

            property var activeItem: coverItems.length > 0 ? coverItems[currentListIndex] : null

            // Empty State (No bookmarks or ribbons set)
            Label {
                anchors.centerIn: parent
                width: parent.width
                text: confCoverMode.value === 0 ? "" : qsTr("No items set")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                visible: !parent.activeItem && confCoverMode.value !== 0
            }

            // Display Container for Active Item
            Column {
                width: parent.width
                spacing: Theme.paddingExtraSmall
                visible: parent.activeItem !== null

                // MODE 2 & 3 (Ribbon Layout): Ribbon Icon + Chapter Reference
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: Theme.paddingMedium
                    visible: parent.parent.activeItem ? parent.parent.activeItem.type === "ribbon" : false

                    RibbonIcon {
                        anchors.verticalCenter: parent.verticalCenter
                        ribbonColor: parent.parent.parent.activeItem && parent.parent.parent.activeItem.type === "ribbon" ? parent.parent.parent.activeItem.color : Theme.highlightColor
                        isSet: true
                        labelText: parent.parent.parent.activeItem && parent.parent.parent.activeItem.type === "ribbon" ? parent.parent.parent.activeItem.id.toString() : "1"
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: parent.parent.parent.activeItem ? parent.parent.parent.activeItem.ref : ""
                        color: Theme.primaryColor
                        font.pixelSize: Theme.fontSizeMedium
                        font.bold: true
                        truncationMode: TruncationMode.Fade
                    }
                }

                // MODE 0 & 1 (VOTD & Bookmarks Header): Standard Book/Chapter:Verse Ref
                Label {
                    width: parent.width
                    text: parent.parent.activeItem ? parent.parent.activeItem.ref : ""
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    truncationMode: TruncationMode.Fade
                    visible: parent.parent.activeItem ? parent.parent.activeItem.type !== "ribbon" : false
                }

                // Verse Text Area (Only rendered for VOTD and Bookmarks)
                Label {
                    width: parent.width
                    text: parent.parent.activeItem && parent.parent.activeItem.text ? parent.parent.activeItem.text : ""
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall - 1
                    wrapMode: Text.Wrap
                    maximumLineCount: 8
                    truncationMode: TruncationMode.Fade
                    font.italic: true
                    horizontalAlignment: Text.AlignHCenter
                    visible: parent.parent.activeItem ? parent.parent.activeItem.type !== "ribbon" : false
                }
            }
        }
    }

    // Dynamic Cover Action Controls (2-Button Sailfish OS Spec)
    CoverActionList {
        id: coverAction
        enabled: confCoverMode.value === 0 || coverItems.length > 0

        // LEFT BUTTON: Step backward in VOTD history or cycle bookmarks/ribbons
        CoverAction {
            iconSource: confCoverMode.value === 0 ? "image://theme/icon-splus-left" : "image://theme/icon-cover-next"

            onTriggered: {
                if (confCoverMode.value === 0) {
                    cover.updateVotd(cover.daysOffset - 1)
                } else if (coverItems.length > 0) {
                    if (currentListIndex < coverItems.length - 1) {
                        currentListIndex++
                    } else {
                        currentListIndex = 0
                    }
                }
            }
        }

        // RIGHT BUTTON: Step forward in VOTD history (if daysOffset < 0) or Open Play Jump
        CoverAction {
            iconSource: {
                if (confCoverMode.value === 0 && cover.daysOffset < 0) {
                    return "image://theme/icon-splus-right"
                }
                return "image://theme/icon-cover-play"
            }

            onTriggered: {
                if (confCoverMode.value === 0) {
                    if (cover.daysOffset < 0) {
                        cover.updateVotd(cover.daysOffset + 1)
                    } else {
                        cover.openCurrentPassage()
                    }
                } else {
                    cover.openCurrentPassage()
                }
            }
        }
    }
}
