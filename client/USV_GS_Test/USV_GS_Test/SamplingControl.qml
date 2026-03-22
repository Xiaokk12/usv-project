import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 独立的采样控制组件，方便前后端调用
Column {
    id: samplingControl
    spacing: 10
    Layout.fillWidth: true

    // 对外属性
    property alias bottleNumber: sampleBottleSpin.value
    property alias volumeLiters: sampleVolumeField.text
    property alias depthMeters: sampleDepthField.text
    property alias site: sampleSiteCombo.currentText
    property alias speedLimitMetersPerSec: speedLimitSlider.value
    property string statusText: "状态: 待命"
    property var routePoints: []              // 当前航线点，录入任务时使用
    property var samplingSettings: ({})       // 用于写入任务文件的采样配置
    property var recordedSamples: []          // 手动记录的点位

    // 对外信号
    signal startSamplingRequested(var params)
    signal stopSamplingRequested()

    function setStatus(text) { statusText = text }

    function normalizedRoutePoints() {
        const pts = routePoints || []
        return pts.map(function(p) {
            const lng = isFinite(p.lng) ? p.lng : p.lon
            const lat = p.lat
            return { lng: lng, lat: lat }
        }).filter(function(p) { return isFinite(p.lng) && isFinite(p.lat) })
    }

    function recordCurrentSample() {
        const volume = parseFloat(sampleVolumeField.text)
        const depth = parseFloat(sampleDepthField.text)
        const siteNum = parseInt(sampleSiteCombo.currentText)
        if (!isFinite(volume) || !isFinite(depth) || !isFinite(siteNum)) {
            setStatus("状态: 请输入有效的采样数据")
            return
        }
        const routeIdx = siteNum - 1
        const pts = normalizedRoutePoints()
        const matched = (routeIdx >= 0 && routeIdx < pts.length) ? pts[routeIdx] : null
        const rec = {
            order: recordedSamples.length + 1,
            bottle: sampleBottleSpin.value,
            volume: volume,
            depth: depth,
            site: siteNum,
            pointIndex: siteNum,
            time: new Date().toISOString()
        }
        if (matched) {
            rec.lng = matched.lng
            rec.lat = matched.lat
        }
        recordedSamples = recordedSamples.concat([rec])
        setStatus("状态: 已记录 " + recordedSamples.length + " 个点")
    }

    function saveRecordsAsTask() {
        const pts = normalizedRoutePoints()
        if (!pts.length) {
            setStatus("状态: 需先加载/规划航线，才能录入任务")
            return
        }
        if (!recordedSamples.length) {
            setStatus("状态: 请先记录至少 1 个点")
            return
        }
        const samplingPts = recordedSamples.map(function(r) {
            return {
                order: r.order,
                pointIndex: r.pointIndex,
                site: r.site,
                bottle: r.bottle,
                volume: r.volume,
                depth: r.depth,
                lng: r.lng,
                lat: r.lat,
                time: r.time
            }
        })
        const payload = {
            name: "手动采样任务",
            points: pts,
            samplingPoints: samplingPts,
            samplingSettings: samplingSettings || {}
        }
        const stamp = new Date().toISOString().replace(/[:.]/g, "-")
        const taskName = "手动采样-" + stamp
        const ok = RouteSaver.saveTask(taskName, JSON.stringify(payload))
        if (ok) {
            setStatus("状态: 任务已保存（" + recordedSamples.length + " 个点）")
            recordedSamples = []
        } else {
            setStatus("状态: 保存任务失败")
        }
    }

    GroupBox {
        title: "速度"
        Layout.fillWidth: true
        Column {
            spacing: 6
            Row {
                spacing: 6
                Text { text: "0 m/s" }
                Slider {
                    id: speedLimitSlider
                    from: 0
                    to: 3
                    stepSize: 0.1
                    value: 1.5
                    Layout.fillWidth: true
                }
                Text { text: "3 m/s" }
            }
            Text { text: "当前速度: " + speedLimitSlider.value.toFixed(1) + " m/s" }
        }
    }

    GroupBox {
        title: "手动采样（预留接口）"
        Layout.fillWidth: true
        Column {
            spacing: 8

            Row {
                spacing: 8
                Text { text: "采样瓶号:"; width: 90 }
                SpinBox {
                    id: sampleBottleSpin
                    from: 1
                    to: 12
                    value: 1
                    editable: true
                    Layout.fillWidth: true
                }
            }

            Row {
                spacing: 8
                Text { text: "采样容量(升):"; width: 90 }
                TextField {
                    id: sampleVolumeField
                    text: "0.5"
                    Layout.fillWidth: true
                }
            }

            Row {
                spacing: 8
                Text { text: "采样深度(米):"; width: 90 }
                TextField {
                    id: sampleDepthField
                    text: "0.5"
                    Layout.fillWidth: true
                }
            }

            Row {
                spacing: 8
                Text { text: "采样位点:"; width: 90 }
                ComboBox {
                    id: sampleSiteCombo
                    model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10"]
                    Layout.fillWidth: true
                }
            }

            Row {
                spacing: 10
                Button {
                    text: "开始采样"
                    onClicked: {
                        const params = {
                            bottleNumber: sampleBottleSpin.value,
                            volumeLiters: parseFloat(sampleVolumeField.text),
                            depthMeters: parseFloat(sampleDepthField.text),
                            site: sampleSiteCombo.currentText,
                            speedLimitMetersPerSec: speedLimitSlider.value
                        }
                        samplingControl.startSamplingRequested(params)
                        samplingControl.setStatus("状态: 采样中...")
                        if (typeof SamplingTaskBridge !== "undefined" && SamplingTaskBridge.requestManualStart) {
                            SamplingTaskBridge.requestManualStart(
                                        sampleBottleSpin.value,
                                        parseFloat(sampleVolumeField.text),
                                        parseFloat(sampleDepthField.text),
                                        parseInt(sampleSiteCombo.currentText))
                        }
                    }
                }
                Button {
                    text: "停止采样"
                    onClicked: {
                        samplingControl.stopSamplingRequested()
                        samplingControl.setStatus("状态: 已停止")
                        if (typeof SamplingTaskBridge !== "undefined" && SamplingTaskBridge.requestManualStop) {
                            SamplingTaskBridge.requestManualStop()
                        }
                    }
                }
            }

            Text {
                id: statusLabel
                text: samplingControl.statusText
                color: "#555"
            }

            Text {
                text: "已记录点位: " + recordedSamples.length
                color: "#555"
            }

            Row {
                spacing: 10
                Button {
                    text: "记录点位数据"
                    onClicked: recordCurrentSample()
                }
                Button {
                    text: "录入为任务"
                    enabled: recordedSamples.length > 0
                    onClicked: saveRecordsAsTask()
                }
            }
        }
    }
}
