import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: aboutPage
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: contentColumn.height + Theme.paddingLarge

        Column {
            id: contentColumn
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("About Captain's Bible")
            }

            // App Icon / Graphic Placeholder
            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: "image://theme/icon-m-about"
                width: Theme.iconSizeSuper
                height: Theme.iconSizeSuper
            }

            // App Title & Version
            Column {
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.paddingExtraSmall

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: "Captain's Bible"
                    font.pixelSize: Theme.fontSizeLarge
                    color: Theme.highlightColor
                }

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Version 1.0.0")
                    font.pixelSize: Theme.fontSizeExtraSmall
                    color: Theme.secondaryColor
                }
            }

            // General Description
            SectionHeader {
                text: qsTr("Description")
            }

            Label {
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.horizontalCenter: parent.horizontalCenter
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.primaryColor
                text: qsTr("Captain's Bible is a native Bible reading and study application built specifically for Sailfish OS. It supports offline MyBible modules, cross-references, and commentaries.")
            }

            // Disclaimer Section
            SectionHeader {
                text: qsTr("Disclaimer")
            }

            Label {
                width: parent.width - (2 * Theme.horizontalPageMargin)
                anchors.horizontalCenter: parent.horizontalCenter
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                textFormat: Text.RichText
                text: qsTr("<b>Notice:</b> This application provides access to various third-party Bible translations, commentaries, and reference modules. The developer does not necessarily endorse, agree with, or validate all content, translations, or interpretations contained within these modules.<br/><br/>Users are encouraged to exercise their own discretion, judgment, and study when reading the Scriptures.")
            }
        }
    }
}
