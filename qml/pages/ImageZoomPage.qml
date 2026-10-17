import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: zoomPage
    allowedOrientations: Orientation.All

    property string imageSource: ""

    SilicaFlickable {
        anchors.fill: parent
        contentWidth: Math.max(imageItem.width * zoomItem.scale, width)
        contentHeight: Math.max(imageItem.height * zoomItem.scale, height)
        clip: true

        PinchArea {
            id: pinchArea
            width: parent.contentWidth
            height: parent.contentHeight

            property real currentScale: 1.0

            onPinchStarted: {
                currentScale = zoomItem.scale
            }

            onPinchUpdated: {
                var newScale = currentScale * pinch.scale
                zoomItem.scale = Math.min(Math.max(newScale, 1.0), 5.0)
            }

            Item {
                id: zoomItem
                width: zoomPage.width
                height: zoomPage.height
                transformOrigin: Item.Center

                Image {
                    id: imageItem
                    anchors.centerIn: parent
                    width: zoomPage.width
                    height: zoomPage.height
                    fillMode: Image.PreserveAspectFit
                    source: zoomPage.imageSource
                    asynchronous: true
                }
            }
        }
    }
}
