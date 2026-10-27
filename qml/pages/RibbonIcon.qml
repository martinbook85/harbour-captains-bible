import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: ribbonIcon
    width: Theme.iconSizeSmall
    height: Theme.iconSizeMedium

    property color ribbonColor: Theme.highlightColor
    property bool isSet: false
    property string labelText: "1"
    property bool isFlashing: false

    // Function to trigger temporary visual feedback
    function flash() {
        flashTimer.restart()
    }

    Timer {
        id: flashTimer
        interval: 350
        onTriggered: ribbonIcon.isFlashing = false
        onRunningChanged: {
            if (running) ribbonIcon.isFlashing = true
        }
    }

    // Main Ribbon Body (Outline border when set, solid fill when flashing)
    Rectangle {
        id: bodyRect
        width: parent.width
        anchors.top: parent.top
        anchors.bottom: notchCanvas.top

        color: ribbonIcon.isFlashing ? ribbonIcon.ribbonColor : "transparent"
        border.color: ribbonIcon.isSet ? ribbonIcon.ribbonColor : Theme.secondaryColor
        border.width: ribbonIcon.isSet ? 2 : 1
        opacity: ribbonIcon.isSet ? 1.0 : 0.3
        radius: 3

        Label {
            anchors.centerIn: parent
            text: ribbonIcon.labelText
            font.pixelSize: Theme.fontSizeSmall
            font.bold: true
            color: ribbonIcon.isFlashing ? Theme.primaryColor
                                          : (ribbonIcon.isSet ? ribbonIcon.ribbonColor : Theme.secondaryColor)
        }
    }

    // Bottom V-Notch (Canvas shape matching border outline)
    Canvas {
        id: notchCanvas
        width: parent.width
        height: Theme.paddingMedium
        anchors.bottom: parent.bottom

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = ribbonIcon.isSet ? ribbonIcon.ribbonColor : Theme.secondaryColor
            ctx.fillStyle = ribbonIcon.isFlashing ? ribbonIcon.ribbonColor : "transparent"
            ctx.lineWidth = ribbonIcon.isSet ? 2 : 1
            ctx.globalAlpha = ribbonIcon.isSet ? 1.0 : 0.3

            ctx.beginPath()
            ctx.moveTo(0, 0)
            ctx.lineTo(width / 2, height)
            ctx.lineTo(width, 0)

            if (ribbonIcon.isFlashing) {
                ctx.closePath()
                ctx.fill()
            }
            ctx.stroke()
        }

        Connections {
            target: ribbonIcon
            onIsSetChanged: notchCanvas.requestPaint()
            onRibbonColorChanged: notchCanvas.requestPaint()
            onIsFlashingChanged: notchCanvas.requestPaint()
        }
    }
}
