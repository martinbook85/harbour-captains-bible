import QtQuick 2.6
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0

Page {
    id: settingsPage
    allowedOrientations: Orientation.All

    ConfigurationValue {
        id: confRefPlacement
        key: "/apps/harbour-captains-bible/refPlacement"
        defaultValue: 0 // 0 = Header, 1 = Footer
    }

    ConfigurationValue {
        id: confShowVerseNums
        key: "/apps/harbour-captains-bible/showVerseNums"
        defaultValue: true
    }

    ConfigurationValue {
        id: confDelimiter
        key: "/apps/harbour-captains-bible/refDelimiter"
        defaultValue: ":" // ":" for English standard, "," for European standard
    }

    ConfigurationValue {
        id: confKeepScreenOn
        key: "/apps/harbour-captains-bible/keepScreenOn"
        defaultValue: false
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: contentColumn.height + Theme.paddingLarge

        Column {
            id: contentColumn
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Settings")
            }

            SectionHeader {
                text: qsTr("Verse Copying Format")
            }

            ComboBox {
                width: parent.width
                label: qsTr("Reference Placement")
                currentIndex: confRefPlacement.value
                menu: ContextMenu {
                    MenuItem { text: qsTr("Before text (Header: John 3:16)") }
                    MenuItem { text: qsTr("After text (Footer: — John 3:16)") }
                }
                onCurrentIndexChanged: confRefPlacement.value = currentIndex
            }

            ComboBox {
                width: parent.width
                label: qsTr("Chapter / Verse Separator")
                currentIndex: confDelimiter.value === "," ? 1 : 0
                menu: ContextMenu {
                    MenuItem { text: qsTr("Colon (John 3:16 - English/Intl)") }
                    MenuItem { text: qsTr("Comma (Joh 3,16 - European)") }
                }
                onCurrentIndexChanged: confDelimiter.value = (currentIndex === 1 ? "," : ":")
            }

            TextSwitch {
                width: parent.width
                text: qsTr("Include verse numbers")
                description: qsTr("Add verse numbers before each copied verse")
                checked: confShowVerseNums.value
                onCheckedChanged: confShowVerseNums.value = checked
            }

            TextSwitch {
                width: parent.width
                text: qsTr("Keep screen on in Bible view")
                description: qsTr("Prevents the display from turning off while reading scripture")
                checked: confKeepScreenOn.value
                onCheckedChanged: confKeepScreenOn.value = checked
            }

            SectionHeader {
                text: qsTr("About Application")
            }

            ValueButton {
                label: qsTr("Captain's Bible")
                value: qsTr("About")
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
        }
    }
}
