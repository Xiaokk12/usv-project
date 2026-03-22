import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 手动操控组件：包含摇杆、键盘控制开关，输出归一化杆量
Column {
    id: root
    spacing: 14
    Layout.fillWidth: true

    // 对外属性
    property real sway: 0       // 原始横向 -1 ~ 1
    property real surge: 0      // 原始纵向 -1 ~ 1（前为负，后为正以匹配原布局）
    property real outSway: 0    // 平滑后的输出
    property real outSurge: 0   // 平滑后的输出
    property real smoothing: 0.25
    property bool keyboardEnabled: false
    property real step: 0.15
    property bool active: true  // 外部控制键盘是否接管焦点

    signal controlChanged(real sway, real surge)
    signal keyboardControlToggled(bool enabled)

    function emitControl(rawSway, rawSurge) {
        const a = smoothing
        outSway = outSway + a * (rawSway - outSway)
        outSurge = outSurge + a * (rawSurge - outSurge)
        controlChanged(outSway, outSurge)
    }

    function setKnobNormalized(nx, ny) {
        const clamp = function(v) { return Math.max(-1, Math.min(1, v)); }
        const cx = clamp(nx)
        const cy = clamp(ny)
        knob.anchors.horizontalCenterOffset = cx * joystickBase.maxR
        knob.anchors.verticalCenterOffset = cy * joystickBase.maxR
        sway = cx
        surge = cy
        emitControl(sway, surge)
    }

    function adjustByStep(dx, dy) {
        setKnobNormalized(sway + dx, surge + dy)
    }

    // 标题
    Text {
        text: "手动操控 Manual Control"
        font.pixelSize: 20
        font.bold: true
    }

    Rectangle {
        id: joystickBase
        width: 2 * 70 + 40
        height: 2 * 70 + 40
        radius: width / 2
        color: "#0B0E13"
        border.color: "#37414F"
        anchors.horizontalCenter: parent.horizontalCenter

        property real knobRadius: 25
        property real centerX: width / 2
        property real centerY: height / 2
        property real maxR: 70

        // 刻度
        Item {
            anchors.fill: parent
            anchors.margins: 12
            Repeater {
                model: 24
                Rectangle {
                    width: 2
                    height: index % 6 === 0 ? 14 : 8
                    color: "#E8EDF5"
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    transform: Rotation {
                        origin.x: 0
                        origin.y: joystickBase.height / 2 - 12
                        angle: index * 15
                    }
                }
            }
        }

        // 方向标记
        Text {
            text: "F"
            color: "white"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 28
            font.pixelSize: 16
            font.bold: true
        }
        Text {
            text: "B"
            color: "white"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 28
            font.pixelSize: 16
            font.bold: true
        }
        Text {
            text: "L"
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 28
            font.pixelSize: 16
            font.bold: true
        }
        Text {
            text: "R"
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 28
            font.pixelSize: 16
            font.bold: true
        }

        // 船体示意
        Canvas {
            id: boatShape
            anchors.centerIn: parent
            width: 40
            height: 70
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.translate(width/2, height/2)
                ctx.fillStyle = "#D94B32"
                ctx.beginPath()
                ctx.moveTo(0, -30)
                ctx.quadraticCurveTo(12, -10, 12, 15)
                ctx.lineTo(10, 25)
                ctx.quadraticCurveTo(0, 30, -10, 25)
                ctx.lineTo(-12, 15)
                ctx.quadraticCurveTo(-12, -10, 0, -30)
                ctx.closePath()
                ctx.fill()
            }
        }

        Rectangle {
            id: knob
            width: 40
            height: 40
            radius: 20
            color: "#F5A623"
            border.color: "#C47B12"
            anchors.verticalCenterOffset: 0
            anchors.horizontalCenterOffset: 0
            anchors.centerIn: parent
        }

        MouseArea {
            anchors.fill: parent
            preventStealing: true
            onPressed: function(mouse) { handleDrag(mouse) }
            onPositionChanged: function(mouse) { handleDrag(mouse) }
            onReleased: function(mouse) { resetKnob() }

            function handleDrag(mouse) {
                let dx = mouse.x - joystickBase.centerX
                let dy = mouse.y - joystickBase.centerY
                const len = Math.sqrt(dx * dx + dy * dy)
                const maxR = joystickBase.maxR
                if (len > maxR) {
                    dx = dx * maxR / len
                    dy = dy * maxR / len
                }
                knob.anchors.horizontalCenterOffset = dx
                knob.anchors.verticalCenterOffset = dy
                sway = dx / maxR
                surge = dy / maxR
                emitControl(sway, surge)
            }

            function resetKnob() {
                knob.anchors.horizontalCenterOffset = 0
                knob.anchors.verticalCenterOffset = 0
                sway = 0
                surge = 0
                outSway = 0
                outSurge = 0
                controlChanged(outSway, outSurge)
            }
        }
    }

    CheckBox {
        text: "启用键盘控制"
        checked: root.keyboardEnabled
        onCheckedChanged: {
            root.keyboardEnabled = checked
            keyboardControlToggled(checked)
        }
    }

    Keys.onPressed: function(event) {
        if (!root.active || !root.keyboardEnabled)
            return
        switch (event.key) {
        case Qt.Key_Left:
            adjustByStep(-step, 0)
            event.accepted = true
            break
        case Qt.Key_Right:
            adjustByStep(step, 0)
            event.accepted = true
            break
        case Qt.Key_Up:
            adjustByStep(0, -step)
            event.accepted = true
            break
        case Qt.Key_Down:
            adjustByStep(0, step)
            event.accepted = true
            break
        default:
            break
        }
    }

    focus: active
    Keys.enabled: active
    Keys.priority: Keys.BeforeItem
}
