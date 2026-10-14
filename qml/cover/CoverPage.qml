import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

CoverBackground {
    id: cover

    ConfigurationValue {
        id: confModule
        key: "/apps/harbour-captains-bible/selectedModule"
        defaultValue: ""
    }

    property int daysOffset: 0
    property string verseRef: ""
    property string verseText: qsTr("Loading...")

    // Beräknar dynamiskt datumet baserat på offset
    property string currentDateStr: {
        var d = new Date()
        d.setDate(d.getDate() + daysOffset)
        return Qt.formatDate(d, "d MMMM yyyy")
    }

    function updateContent(offset) {
        var modName = confModule.value
        if (typeof bibleManager !== "undefined" && modName && modName.length > 0) {
            daysOffset = offset;
            var votd = bibleManager.getVerseByOffset(modName, daysOffset);

            if (votd && (votd.reference || votd.text)) {
                verseRef = votd.reference || qsTr("Verse of the Day");
                // Om texten saknas i '.text', kollar vi om den ligger i '.content' eller '.verseText'
                verseText = votd.text || votd.content || votd.verseText || qsTr("No verse text available for this date.");
            } else {
                verseRef = qsTr("Verse of the Day");
                verseText = qsTr("No verse available.");
            }
        } else {
            verseRef = qsTr("Captain's Bible");
            verseText = qsTr("Select a Bible translation from the menu to activate Verse of the Day.");
        }
    }

    onStatusChanged: {
        if (status === Cover.Active) {
            updateContent(0);
        }
    }

    // Bakgrundsbild: 172icon.png placerad i qml/cover/
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
            bottomMargin: Theme.itemSizeSmall
        }
        spacing: Theme.paddingExtraSmall

        Label {
            width: parent.width
            text: qsTr("Captain's Bible")
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeExtraSmall
            horizontalAlignment: Text.AlignHCenter
        }

        // Dynamiskt datum som uppdateras när man klickar bakåt/framåt
        Label {
            width: parent.width
            text: currentDateStr
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall - 2
            horizontalAlignment: Text.AlignHCenter
        }

        Separator {
            width: parent.width
            color: Theme.highlightColor
            horizontalAlignment: Qt.AlignHCenter
        }

        // Tätt packad referens och vers utan dubblering
        Column {
            width: parent.width
            spacing: Theme.paddingExtraSmall

            Label {
                id: refLabel
                width: parent.width
                text: verseRef
                // Primary color for full-opacity contrast on any ambiance
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                truncationMode: TruncationMode.Fade
            }

            Label {
                id: verseLabel
                width: parent.width
                height: implicitHeight
                text: verseText
                // Changed from secondaryColor to primaryColor for sharp readability
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
                maximumLineCount: 6
                font.italic: true
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    CoverActionList {
        id: coverAction

        CoverAction {
            iconSource: "image://theme/icon-s-left"
            onTriggered: cover.updateContent(cover.daysOffset - 1)
        }

        CoverAction {
            iconSource: cover.daysOffset < 0 ? "image://theme/icon-s-right" : "image://theme/icon-s-sync"
            onTriggered: {
                if (cover.daysOffset < 0) {
                    cover.updateContent(cover.daysOffset + 1)
                } else {
                    cover.updateContent(0)
                }
            }
        }
    }
}
