import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: imagePage
    allowedOrientations: Orientation.All

    // Disable back-swipe navigation when zoomed in so panning works without triggering page back
    backNavigation: imageTransform.xScale === 1.0

    property string imageSource: ""
    property string imageTitle: qsTr("Map View")

    PageHeader {
        id: header
        title: imagePage.imageTitle
        z: 10
    }

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        anchors.topMargin: header.height
        contentWidth: imageContainer.width * imageTransform.xScale
        contentHeight: imageContainer.height * imageTransform.yScale
        clip: true

        PinchArea {
            id: pinchArea
            width: Math.max(flickable.contentWidth, flickable.width)
            height: Math.max(flickable.contentHeight, flickable.height)

            property real currentScale: 1.0
            property real minScale: 1.0
            property real maxScale: 4.0

            onPinchStarted: {
                currentScale = imageTransform.xScale
            }

            onPinchUpdated: {
                var newScale = currentScale * pinch.scale
                var boundedScale = Math.min(Math.max(newScale, minScale), maxScale)
                imageTransform.xScale = boundedScale
                imageTransform.yScale = boundedScale
            }

            Item {
                id: imageContainer
                width: flickable.width
                height: flickable.height

                Image {
                    id: mapImage
                    anchors.centerIn: parent
                    source: imagePage.imageSource
                    fillMode: Image.PreserveAspectFit
                    width: parent.width
                    height: parent.height

                    transform: Scale {
                        id: imageTransform
                        origin.x: mapImage.width / 2
                        origin.y: mapImage.height / 2
                        xScale: 1.0
                        yScale: 1.0
                    }
                }
            }
        }
    }

    // Double-click to toggle reset zoom
    MouseArea {
        anchors.fill: parent
        onDoubleClicked: {
            imageTransform.xScale = 1.0
            imageTransform.yScale = 1.0
        }
    }
}
