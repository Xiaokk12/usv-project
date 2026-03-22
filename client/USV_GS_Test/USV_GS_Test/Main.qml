import QtQuick
import QtQuick.Window
import QtWebEngine
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtWebChannel
import QtPositioning
import "."
import "RouteAutoPlanner.js" as Planner

Window {
    id: root
    width: 1500
    height: 900
    visible: true
    title: "USV Ground Station"

    property bool pageReady: false
    property real sidebarWidth: 320
    property real sidebarMinWidth: 240
    property real sidebarMaxWidth: 640

    // WebChannel：把 C++ 的 RouteSaver 通过 QML 包装后暴露给 HTML
    WebChannel {
        id: webChannel
        registeredObjects: [ routeSaverProxy, routePlannerProxy, nativeLocator, boatLocationProxy, samplingBridgeProxy, envDataProxy ]
    }

    // 原生定位，用于触发 macOS 定位弹窗并把坐标推给 HTML
    PositionSource {
        id: nativePos
        active: false
        preferredPositioningMethods: PositionSource.AllPositioningMethods
        updateInterval: 2000

        onPositionChanged: {
            const lng = position.coordinate.longitude
            const lat = position.coordinate.latitude
            console.log("[QML] Native location:", lng, lat)
            pushLocationToMap(lng, lat)
            nativeLocator.locationUpdated(lng, lat)
        }

        onSourceErrorChanged: {
            console.warn("[QML] Native location error:", sourceError)
        }
    }

    // 采样任务桥接代理
    QtObject {
        id: samplingBridgeProxy
        objectName: "SamplingTaskBridge"
        WebChannel.id: "SamplingTaskBridge"
        function updateAutoStatus(bottle, volume, depth, site, status) {
            if (SamplingTaskBridge && SamplingTaskBridge.updateAutoStatus)
                SamplingTaskBridge.updateAutoStatus(bottle, volume, depth, site, status)
        }
    }

    QtObject {
        id: envDataProxy
        objectName: "EnvironmentData"
        WebChannel.id: "EnvironmentData"
        function generateRandom() {
            if (EnvironmentDataProvider && EnvironmentDataProvider.generateRandom)
                return EnvironmentDataProvider.generateRandom()
            return ({})
        }
        function normalize(data) {
            if (EnvironmentDataProvider && EnvironmentDataProvider.normalize)
                return EnvironmentDataProvider.normalize(data || {})
            return (data || {})
        }
    }

    function requestNativeLocation() {
        // 激活一次定位，会触发系统定位授权弹窗
        nativePos.active = true
        nativePos.update()
    }

    function pushLocationToMap(lng, lat) {
        const js = `
            if (window.updateLocationMarker) {
                updateLocationMarker(${lng}, ${lat});
                if (typeof debug === "function") {
                    debug("QML 原生定位 lng=${lng}, lat=${lat}");
                }
            }
        `;
        webmap.runJavaScript(js)
    }

    // 包装一层，避免直接引用上下文对象导致崩溃
    QtObject {
        id: routeSaverProxy
        objectName: "RouteSaver"
        WebChannel.id: "RouteSaver" // 显式指定暴露给 JS 的名称
        function saveRoute(json) { return RouteSaver.saveRoute(json); }
    }

    QtObject {
        id: routePlannerProxy
        objectName: "RoutePlanner"
        WebChannel.id: "RoutePlanner"
        function addPoint(lon, lat) {
            routePlanner.receivePoint(lon, lat)
        }
        function setSamplingPoints(pointsJson) {
            routePlanner.updateSamplingPointsFromMap(pointsJson)
        }
        function updateRouteFromMap(pointsJson) {
            if (routePlanner && routePlanner.updateRouteFromMap)
                routePlanner.updateRouteFromMap(pointsJson)
        }
    }

    // 原生定位桥接，供 HTML 侧调用
    QtObject {
        id: nativeLocator
        objectName: "NativeLocator"
        WebChannel.id: "NativeLocator"
        signal locationUpdated(real lng, real lat)
        function requestLocation() {
            requestNativeLocation()
        }
        function stop() {
            nativePos.active = false
        }
    }

    // 无人船定位桥接，暴露给 WebChannel
    QtObject {
        id: boatLocationProxy
        objectName: "BoatLocation"
        WebChannel.id: "BoatLocation"
        signal locationUpdated(real lon, real lat, real heading)
        signal statusChanged(string status)
        function emitLocation(lon, lat, heading) {
            if (BoatLocation && BoatLocation.emitLocation)
                BoatLocation.emitLocation(lon, lat, heading)
        }
        function requestLocation() {
            if (BoatLocation && BoatLocation.requestLocation)
                BoatLocation.requestLocation()
        }
        function updateStatus(status) {
            if (BoatLocation && BoatLocation.updateStatus)
                BoatLocation.updateStatus(status)
        }
    }

    // 转发 C++ 无人船定位信号给 WebChannel
    Connections {
        target: BoatLocation
        function onLocationUpdated(lon, lat, heading) {
            boatLocationProxy.locationUpdated(lon, lat, heading)
            boatStatus.update(lon, lat, heading)
        }
        function onStatusChanged(status) {
            boatStatus.statusText = status || "待命"
            boatLocationProxy.statusChanged(status)
        }
    }

    // 自动采样任务状态（预留与后端对接）
    QtObject {
        id: autoTaskState
        property int bottle: SamplingTaskBridge ? SamplingTaskBridge.missionBottle : 1
        property real volume: SamplingTaskBridge ? SamplingTaskBridge.missionVolume : 0.5
        property real depth: SamplingTaskBridge ? SamplingTaskBridge.missionDepth : 0.5
        property int site: SamplingTaskBridge ? SamplingTaskBridge.missionSite : 1
        property string status: SamplingTaskBridge ? SamplingTaskBridge.statusSummary : "待命"
        property string systemState: SamplingTaskBridge ? SamplingTaskBridge.systemState : "BOOT"
        property string missionState: SamplingTaskBridge ? SamplingTaskBridge.missionState : "NONE"
        property int currentWaypoint: SamplingTaskBridge ? SamplingTaskBridge.currentWaypoint : 0
        property int routePointCount: SamplingTaskBridge ? SamplingTaskBridge.routePointCount : 0
        property string lastError: SamplingTaskBridge ? SamplingTaskBridge.lastError : ""
        property real batterySoc: SamplingTaskBridge ? SamplingTaskBridge.batterySoc : 1.0
        property bool gnssOk: SamplingTaskBridge ? SamplingTaskBridge.gnssOk : true
        property bool motorOk: SamplingTaskBridge ? SamplingTaskBridge.motorOk : true
        property bool rudderOk: SamplingTaskBridge ? SamplingTaskBridge.rudderOk : true
        property bool samplerOk: SamplingTaskBridge ? SamplingTaskBridge.samplerOk : true
        property bool winchReady: SamplingTaskBridge ? SamplingTaskBridge.winchReady : false
        property bool pumpReady: SamplingTaskBridge ? SamplingTaskBridge.pumpReady : false
        property bool rinsing: SamplingTaskBridge ? SamplingTaskBridge.rinsing : false
        property bool sampling: SamplingTaskBridge ? SamplingTaskBridge.sampling : false
        property string healthSummary: SamplingTaskBridge ? SamplingTaskBridge.healthSummary : "GNSS OK | MOTOR OK | RUDDER OK | SAMPLER OK"
        property string samplerSummary: SamplingTaskBridge ? SamplingTaskBridge.samplerSummary : "WINCH WAIT | PUMP OFF | RINSE IDLE | SAMPLE IDLE"
    }

    function runMapCommand(script) {
        if (!root.pageReady) {
            console.warn("[QML] map not ready")
            return
        }
        webmap.runJavaScript(script)
    }

    function pushAutoSpeedValue() {
        if (!root.pageReady)
            return
        runMapCommand(`if (typeof updateSimSpeed === "function") { updateSimSpeed(${autoSpeedSlider.value}); }`)
    }

    function syncBackendStatusText() {
        if (BoatLocation && BoatLocation.updateStatus)
            BoatLocation.updateStatus(autoTaskState.status)
    }

    function syncRoutePointCount() {
        if (SamplingTaskBridge && SamplingTaskBridge.setRoutePointCount)
            SamplingTaskBridge.setRoutePointCount(Math.max(1, routePlanner.route.length))
    }

    function startSystemFlow() {
        if (SamplingTaskBridge && SamplingTaskBridge.requestSystemStart)
            SamplingTaskBridge.requestSystemStart()
        if (SamplingTaskBridge && SamplingTaskBridge.updateHealthStatus)
            SamplingTaskBridge.updateHealthStatus(true, true, true, true)
        if (SamplingTaskBridge && SamplingTaskBridge.updateSamplerStatus)
            SamplingTaskBridge.updateSamplerStatus(false, false, false, false)
        if (BoatSimulator && BoatSimulator.stop)
            BoatSimulator.stop()
        runMapCommand("if (typeof ensureBoatInitialized === 'function') { ensureBoatInitialized(); }")
        syncBackendStatusText()
    }

    function stopSystemFlow() {
        if (SamplingTaskBridge && SamplingTaskBridge.requestSystemStop)
            SamplingTaskBridge.requestSystemStop()
        if (SamplingTaskBridge && SamplingTaskBridge.updateSamplerStatus)
            SamplingTaskBridge.updateSamplerStatus(false, false, false, false)
        if (BoatSimulator && BoatSimulator.stop)
            BoatSimulator.stop()
        runMapCommand("if (typeof stopAllBoatSimulation === 'function') { stopAllBoatSimulation(); }")
        syncBackendStatusText()
    }

    function enterAutoNavFlow() {
        syncRoutePointCount()
        if (SamplingTaskBridge && SamplingTaskBridge.systemState === "BOOT")
            SamplingTaskBridge.requestSystemStart()
        if (SamplingTaskBridge && SamplingTaskBridge.requestSetAuto)
            SamplingTaskBridge.requestSetAuto()
        if (BoatSimulator && BoatSimulator.stop)
            BoatSimulator.stop()
        runMapCommand("if (typeof startSimFromCurrentRoute === 'function') { startSimFromCurrentRoute(); }")
        syncBackendStatusText()
    }

    function startMissionFlow() {
        syncRoutePointCount()
        if (SamplingTaskBridge && SamplingTaskBridge.systemState === "BOOT")
            SamplingTaskBridge.requestSystemStart()
        if (SamplingTaskBridge
                && SamplingTaskBridge.systemState !== "AUTO_NAV"
                && SamplingTaskBridge.systemState !== "MISSION") {
            SamplingTaskBridge.requestSetAuto()
        }
        if (SamplingTaskBridge && SamplingTaskBridge.requestStartMission)
            SamplingTaskBridge.requestStartMission()
        if (BoatSimulator && BoatSimulator.stop)
            BoatSimulator.stop()
        runMapCommand("if (typeof startSimFromCurrentRoute === 'function') { startSimFromCurrentRoute(); }")
        syncBackendStatusText()
    }

    function pauseMissionFlow() {
        if (SamplingTaskBridge && SamplingTaskBridge.requestPause)
            SamplingTaskBridge.requestPause()
        runMapCommand("if (typeof pauseSimFollowRoute === 'function') { pauseSimFollowRoute(); }")
        syncBackendStatusText()
    }

    function resumeMissionFlow() {
        if (SamplingTaskBridge && SamplingTaskBridge.requestResume)
            SamplingTaskBridge.requestResume()
        runMapCommand("if (typeof resumeSimFollowRoute === 'function') { resumeSimFollowRoute(); }")
        syncBackendStatusText()
    }

    function abortMissionFlow() {
        if (SamplingTaskBridge && SamplingTaskBridge.requestAbort)
            SamplingTaskBridge.requestAbort()
        runMapCommand("if (typeof startAutoReturnToRouteEnd === 'function') { startAutoReturnToRouteEnd(); }")
        syncBackendStatusText()
    }

    Connections {
        target: SamplingTaskBridge
        function onSnapshotChanged() {
            syncBackendStatusText()
            if (SamplingTaskBridge.systemState === "SHUTDOWN" || SamplingTaskBridge.systemState === "FAULT") {
                if (BoatSimulator && BoatSimulator.stop)
                    BoatSimulator.stop()
                runMapCommand("if (typeof stopAllBoatSimulation === 'function') { stopAllBoatSimulation(); }")
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ============================
        // 顶部选择框
        // ============================
        Rectangle {
            id: topBar
            Layout.fillWidth: true
            height: 50
            color: "#F7F9FB"
            border.color: "#DDE1E6"

            Row {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 12
                Text {
                    text: "功能选择"
                    font.pixelSize: 16
                    font.bold: true
                }
                ComboBox {
                    id: sectionSelector
                    model: ["自动操纵", "手动操控", "隐藏面板"]
                    currentIndex: 0
                    onCurrentIndexChanged: {
                        if (currentIndex === 1)
                            manualCtrl.forceActiveFocus()
                    }
                }
            }
        }

        // ============================
        // 中间区域：左侧菜单 + 地图
        // ============================
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // 左侧控制栏（由顶部选择框控制显示）
            Rectangle {
                id: leftPanel
                visible: sectionSelector.currentIndex !== 2
                Layout.preferredWidth: visible ? sidebarWidth : 0
                Layout.minimumWidth: visible ? sidebarMinWidth : 0
                Layout.maximumWidth: visible ? sidebarMaxWidth : 0
                Layout.fillHeight: true
                color: "#F0F2F5"
                focus: sectionSelector.currentIndex === 1

                ScrollView {
                    anchors.fill: parent
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded
                    contentWidth: leftPanel.width

                    Item {
                        width: leftPanel.width
                        implicitHeight: leftStack.implicitHeight + 30

                        StackLayout {
                            id: leftStack
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.margins: 15
                            currentIndex: sectionSelector.currentIndex === 1 ? 1 : 0

                            // 控制面板页
                            Column {
                                spacing: 15
                                Layout.fillWidth: true

                                Text {
                                    text: "自动操纵 Auto Control"
                                    font.pixelSize: 20
                                    font.bold: true
                                }

                                GroupBox {
                                    title: "后端状态与任务控制"
                                    Layout.fillWidth: true
                                    Column {
                                        spacing: 8

                                        GroupBox {
                                            title: "速度"
                                            Layout.fillWidth: true
                                            Column {
                                                spacing: 6
                                                Row {
                                                    spacing: 6
                                                    Text { text: "0 m/s" }
                                                    Slider {
                                                        id: autoSpeedSlider
                                                        from: 0
                                                        to: 3
                                                        stepSize: 0.1
                                                        value: 1.5
                                                        Layout.fillWidth: true
                                                        onValueChanged: pushAutoSpeedValue()
                                                        Component.onCompleted: pushAutoSpeedValue()
                                                    }
                                                    Text { text: "3 m/s" }
                                                }
                                                Text { text: "当前速度: " + autoSpeedSlider.value.toFixed(1) + " m/s" }
                                            }
                                        }

                                        Row {
                                            spacing: 8
                                            Text { text: "系统状态:"; width: 90 }
                                            Text { text: autoTaskState.systemState }
                                        }
                                        Row {
                                            spacing: 8
                                            Text { text: "任务状态:"; width: 90 }
                                            Text { text: autoTaskState.missionState }
                                        }
                                        Row {
                                            spacing: 8
                                            Text { text: "当前航点:"; width: 90 }
                                            Text { text: autoTaskState.currentWaypoint + " / " + Math.max(1, autoTaskState.routePointCount) }
                                        }
                                        Row {
                                            spacing: 8
                                            Text { text: "电量 SoC:"; width: 90 }
                                            Text { text: (autoTaskState.batterySoc * 100).toFixed(0) + "%" }
                                        }
                                        Row {
                                            spacing: 8
                                            Text { text: "采样瓶号:"; width: 90 }
                                            Text { text: autoTaskState.bottle }
                                        }
                                        Row {
                                            spacing: 8
                                            Text { text: "采样容量(升):"; width: 90 }
                                            Text { text: autoTaskState.volume.toFixed(2) }
                                        }
                                        Row {
                                            spacing: 8
                                            Text { text: "采样深度(米):"; width: 90 }
                                            Text { text: autoTaskState.depth.toFixed(2) }
                                        }
                                        Row {
                                            spacing: 8
                                            Text { text: "采样位点:"; width: 90 }
                                            Text { text: autoTaskState.site }
                                        }
                                        Row {
                                            spacing: 10
                                            Button {
                                                text: "启动无人船"
                                                onClicked: startSystemFlow()
                                            }
                                            Button {
                                                text: "停止无人船"
                                                onClicked: stopSystemFlow()
                                            }
                                        }
                                        Row {
                                            spacing: 10
                                            Button {
                                                text: "自动返航"
                                                onClicked: autoReturnDialog.open()
                                            }
                                            Button {
                                                text: "导入航线"
                                                onClicked: {
                                                    refreshRoutes()
                                                    importDialog.open()
                                                }
                                            }
                                        }
                                        Row {
                                            spacing: 10
                                            Button {
                                                text: "进入自动航行"
                                                onClicked: enterAutoNavFlow()
                                            }
                                            Button {
                                                text: "开始任务"
                                                onClicked: startMissionFlow()
                                            }
                                        }
                                        Row {
                                            spacing: 10
                                            Button {
                                                text: "暂停任务"
                                                onClicked: pauseMissionFlow()
                                            }
                                            Button {
                                                text: "继续任务"
                                                onClicked: resumeMissionFlow()
                                            }
                                        }
                                        Row {
                                            spacing: 10
                                            Button {
                                                text: "中止任务"
                                                onClicked: abortMissionFlow()
                                            }
                                            Button {
                                                text: "刷新任务状态"
                                                onClicked: {
                                                    if (SamplingTaskBridge && SamplingTaskBridge.requestAutoRefresh)
                                                        SamplingTaskBridge.requestAutoRefresh()
                                                }
                                            }
                                        }
                                        Text { text: "摘要: " + autoTaskState.status; color: "#555" }
                                        Text {
                                            text: "最近错误: " + (autoTaskState.lastError.length ? autoTaskState.lastError : "none")
                                            color: autoTaskState.lastError.length ? "#B3261E" : "#555"
                                            wrapMode: Text.Wrap
                                        }
                                    }
                                }

                        GroupBox {
                            title: "航线规划"
                            Layout.fillWidth: true
                            Column {
                                spacing: 8
                                Text {
                                    text: routePlanner.statusText
                                    wrapMode: Text.Wrap
                                    color: "#333"
                                }
                                Row {
                                    spacing: 8
                                    Button {
                                        text: "进入航线规划"
                                        onClicked: routePlanner.startPlanning()
                                    }
                                    Button {
                                        text: "确认点位数并规划"
                                        enabled: routePlanner.selectedCount === 4
                                        onClicked: routePlanner.requestPlan()
                                    }
                                    Button {
                                        text: "保存航线"
                                        enabled: routePlanner.route.length > 0
                                        onClicked: routePlanner.savePlannedRoute()
                                    }
                                }
                                Row {
                                    spacing: 8
                                    Text { text: "点位数:" }
                                    SpinBox {
                                        id: waypointCountSpin
                                        from: 2
                                        to: 50
                                        value: 6
                                    }
                                }
                                Text {
                                    text: "起点 = 第 1 个选点，终点 = 第 4 个选点，自动按序号生成航线。"
                                    wrapMode: Text.Wrap
                                    color: "#444"
                                }
                                Text {
                                    text: routePlanner.pointsText
                                    wrapMode: Text.Wrap
                                    color: "#666"
                                }
                            }
                        }

                        GroupBox {
                            title: "任务管理"
                            Layout.fillWidth: true
                            Column {
                                spacing: 8
                                TextField {
                                    id: taskNameInput
                                    Layout.fillWidth: true
                                    placeholderText: "任务名称（必填）"
                                    text: routePlanner.taskName
                                    onTextChanged: routePlanner.taskName = text
                                }
                                Row {
                                    spacing: 6
                                    Text { text: "地点(重心):"; color: "#555" }
                                    Text { text: routePlanner.taskLocation; color: "#333"; font.bold: true }
                                }
                                Row {
                                    spacing: 6
                                    Text { text: "时间:"; color: "#555" }
                                    Text { text: routePlanner.taskTime; color: "#333"; font.bold: true }
                                }
                                Row {
                                    spacing: 10
                                    Button {
                                        text: "设置采样点"
                                        enabled: routePlanner.route.length > 0
                                        onClicked: routePlanner.startSamplingSelection()
                                    }
                                    Button {
                                        text: "清空采样点"
                                        enabled: routePlanner.samplingPoints.length > 0
                                        onClicked: routePlanner.clearSamplingPoints()
                                    }
                                }
                                Text {
                                    text: routePlanner.samplingText
                                    wrapMode: Text.Wrap
                                    color: "#444"
                                }
                                Column {
                                    spacing: 4
                                    Button {
                                        text: "统一采样任务设置"
                                        onClicked: samplingSettingsDialog.openWith(routePlanner.samplingSettings)
                                    }
                                    Text {
                                        text: routePlanner.samplingSettingsText
                                        wrapMode: Text.Wrap
                                        color: "#555"
                                        font.pixelSize: 12
                                    }
                                }
                                Row {
                                    spacing: 10
                                    Button {
                                        text: "保存为任务"
                                        enabled: routePlanner.route.length > 0 && taskNameInput.text.length > 0
                                        onClicked: routePlanner.saveTask(taskNameInput.text)
                                    }
                                    Button {
                                        text: "任务列表"
                                        onClicked: {
                                            refreshTasks()
                                            taskDialog.open()
                                        }
                                    }
                                }
                                Row {
                                    spacing: 10
                                    Button {
                                        text: "导出任务"
                                        enabled: routePlanner.route.length > 0 && taskNameInput.text.length > 0
                                        onClicked: exportTaskDialog.open()
                                    }
                                }
                                Text {
                                    text: "保存格式：任务名-地点-时间，航线点数据与航线保存一致。"
                                    wrapMode: Text.Wrap
                                    color: "#666"
                                    font.pixelSize: 12
                                }
                            }
                        }

                            }

                            // 手动操控页
                            Column {
                                spacing: 14
                                Layout.fillWidth: true

                                ManualControl {
                                    id: manualCtrl
                                    Layout.fillWidth: true
                                    active: sectionSelector.currentIndex === 1
                                    onControlChanged: function(sway, surge) {
                                        // 手动杆量直接驱动前端模拟船
                                        const speed = samplingControl.speedLimitMetersPerSec
                                        if (BoatSimulator && BoatSimulator.stop)
                                            BoatSimulator.stop() // 停掉旧的圆形模拟
                                        if (root.pageReady) {
                                            const js = `if (typeof applyManualControl === "function") { applyManualControl(${sway}, ${surge}, ${speed}); }`
                                            webmap.runJavaScript(js)
                                        }
                                    }
                                }

                                SamplingControl {
                                    id: samplingControl
                                    Layout.fillWidth: true
                                    routePoints: routePlanner.route
                                    samplingSettings: routePlanner.samplingSettings
                                    onStartSamplingRequested: function(params) {
                                        // TODO: 调用 C++/后端控制采样，使用 params 中的参数
                                        console.log("Start sampling params:", JSON.stringify(params))
                                    }
                                    onStopSamplingRequested: function() {
                                        // TODO: 调用 C++/后端停止采样
                                        console.log("Stop sampling requested")
                                    }
                                }

                                Connections {
                                    target: BoatLocation
                                    function onStatusChanged(status) {
                                        const txt = status && status.length ? status : "待命"
                                        samplingControl.setStatus("状态: " + txt)
                                    }
                                }

                                GroupBox {
                                    title: "杆量输出（归一化）"
                                    Layout.fillWidth: true
                                    Column {
                                        spacing: 4
                                        Text { text: "横向 (Sway): " + manualCtrl.outSway.toFixed(2) }
                                        Text { text: "纵向 (Surge): " + manualCtrl.outSurge.toFixed(2) }
                                    }
                                }
                            }
                        }
                    }
                }

            }

            // 可拖拽分隔条
            Rectangle {
                id: splitter
                visible: leftPanel.visible
                width: 6
                color: "#D8DBE0"
                Layout.fillHeight: true

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.SplitHCursor
                    acceptedButtons: Qt.LeftButton
                    property real pressX: 0
                    property real pressWidth: 0
                    onPressed: function(mouse) {
                        const global = splitter.mapToItem(null, mouse.x, mouse.y)
                        pressX = global.x
                        pressWidth = sidebarWidth
                    }
                    onPositionChanged: function(mouse) {
                        if (!(mouse.buttons & Qt.LeftButton))
                            return
                        const global = splitter.mapToItem(null, mouse.x, mouse.y)
                        const dx = global.x - pressX
                        sidebarWidth = Math.max(sidebarMinWidth, Math.min(sidebarMaxWidth, pressWidth + dx))
                    }
                }
            }

            // 中间地图区域
            Rectangle {
                id: mapPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "white"

                WebEngineView {
                    id: webmap
                    anchors.fill: parent

                    settings.localContentCanAccessRemoteUrls: true
                    settings.localContentCanAccessFileUrls: true
                    webChannel: webChannel

                    url: "qrc:///map_gaode.html" // 三斜杠形式，兼容 Qt WebEngine

                    // ⭐ 在页面加载完成后注入 WebChannel
                    onLoadingChanged: function(loadRequest) {
                    if (loadRequest.status === WebEngineView.LoadSucceededStatus) {
                        root.pageReady = true;
                        console.log("[QML] Page loaded, injecting WebChannel...");
                        webmap.runJavaScript(`
                            new QWebChannel(qt.webChannelTransport, function(channel) {
                                window.RouteSaver = channel.objects.RouteSaver;
                                window.RoutePlanner = channel.objects.RoutePlanner;
                                window.NativeLocator = channel.objects.NativeLocator;
                                window.BoatLocation = channel.objects.BoatLocation;
                                window.SamplingTaskBridge = channel.objects.SamplingTaskBridge;
                                console.log("WebChannel connected (from QML injection)");
                            });
                        `);
                        if (routePlanner && routePlanner.samplingSettings) {
                            const jsSampling = `if (typeof updateSamplingSimConfig === "function") { updateSamplingSimConfig(${JSON.stringify(routePlanner.samplingSettings)}); }`
                            webmap.runJavaScript(jsSampling)
                        }
                        // 页面准备好后，触发一次原生定位，弹出系统授权
                        requestNativeLocation();
                    }
                }

                    // 禁用 Qt 右键
                    onContextMenuRequested: function(request) {
                        request.accepted = true;
                    }

                    // 允许页面使用定位（navigator.geolocation）
                    onFeaturePermissionRequested: function(securityOrigin, feature) {
                        if (feature === WebEngineView.Geolocation) {
                            grantFeaturePermission(securityOrigin, feature, WebEngineView.PermissionGrantedByUser)
                        }
                    }
                }
            }
        }

        // ============================
        // 底部状态栏
        // ============================
        Rectangle {
            id: bottomBar
            Layout.fillWidth: true
            height: 45
            color: "#E6E8EB"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 20

                Row {
                    spacing: 20
                    Text { text: "经度: " + boatStatus.lon.toFixed(4) }
                    Text { text: "纬度: " + boatStatus.lat.toFixed(4) }
                    Text { text: "状态: " + boatStatus.statusText }
                    Text { text: "后端: " + autoTaskState.systemState + " / " + autoTaskState.missionState }
                }

                Item { Layout.fillWidth: true } // 占位撑满，右侧对齐

                Row {
                    spacing: 16
                    Text { text: "速度: " + boatStatus.speed.toFixed(2) + " m/s" }
                    Text { text: "航向: " + boatStatus.heading.toFixed(0) + "°" }
                    Text { text: "卫星数: 12" }
                }
            }
        }
    }

    // 统一采样任务设置
    Dialog {
        id: samplingSettingsDialog
        modal: true
        title: "统一采样任务设置"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: 360
        focus: true

        function openWith(settings) {
            const cfg = settings || {};
            const bottleVal = isFinite(cfg.bottlesPerPoint) ? cfg.bottlesPerPoint : 1;
            const volumeVal = isFinite(cfg.volumePerBottle) ? cfg.volumePerBottle : 0.5;
            const depthVal = isFinite(cfg.depthMeters) ? cfg.depthMeters : 0.5;
            bottleSpin.value = Math.max(1, Math.round(bottleVal));
            volumeField.text = (isFinite(volumeVal) ? volumeVal : 0.5).toFixed(2);
            depthField.text = (isFinite(depthVal) ? depthVal : 0.5).toFixed(2);
            open();
        }

        onAccepted: {
            routePlanner.applySamplingSettings({
                bottlesPerPoint: bottleSpin.value,
                volumePerBottle: parseFloat(volumeField.text),
                depthMeters: parseFloat(depthField.text)
            })
        }

        contentItem: ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10
            Label {
                text: "设置对所有采样点统一生效的采样参数，将随任务保存并下发。"
                wrapMode: Text.Wrap
                color: "#444"
                Layout.fillWidth: true
            }
            RowLayout {
                spacing: 8
                Label { text: "每点采样瓶数"; Layout.fillWidth: true }
                SpinBox {
                    id: bottleSpin
                    from: 1
                    to: 12
                    value: 1
                    editable: true
                    Layout.preferredWidth: 140
                }
            }
            RowLayout {
                spacing: 8
                Label { text: "单瓶采样容量 (升)"; Layout.fillWidth: true }
                TextField {
                    id: volumeField
                    text: "0.50"
                    Layout.preferredWidth: 140
                    validator: DoubleValidator { bottom: 0.0; top: 999.99; decimals: 2 }
                }
            }
            RowLayout {
                spacing: 8
                Label { text: "采样深度 (米)"; Layout.fillWidth: true }
                TextField {
                    id: depthField
                    text: "0.50"
                    Layout.preferredWidth: 140
                    validator: DoubleValidator { bottom: 0.0; top: 999.99; decimals: 2 }
                }
            }
            Text {
                text: "统一参数会应用到所有采样点，方便批量生成采样任务。"
                wrapMode: Text.Wrap
                color: "#666"
                font.pixelSize: 12
                Layout.fillWidth: true
            }
        }
    }

    // 自动返航选择
    Dialog {
        id: autoReturnDialog
        modal: true
        title: "自动返航"
        standardButtons: Dialog.NoButton
        width: 380
        height: 240
        contentItem: ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 12
            Label {
                text: "是否按当前任务终点返航？"
                wrapMode: Text.WrapAnywhere
                color: "#333"
                Layout.fillWidth: true
            }
            Text {
                text: "选择“按终点返回”将直接规划当前位置到任务终点的直线；选择“标记返航点”后，请在地图上点击目标位置。"
                wrapMode: Text.WrapAnywhere
                color: "#666"
                font.pixelSize: 12
                Layout.fillWidth: true
            }
            RowLayout {
                spacing: 12
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                Button {
                    text: "按终点返回"
                    onClicked: {
                        autoReturnDialog.close()
                        if (SamplingTaskBridge && SamplingTaskBridge.requestReturnHome)
                            SamplingTaskBridge.requestReturnHome()
                        autoReturnController.returnToTaskEnd()
                    }
                }
                Button {
                    text: "标记返航点"
                    onClicked: {
                        autoReturnDialog.close()
                        if (SamplingTaskBridge && SamplingTaskBridge.requestReturnHome)
                            SamplingTaskBridge.requestReturnHome()
                        autoReturnController.startCustomMark()
                    }
                }
                Button {
                    text: "取消"
                    onClicked: autoReturnDialog.close()
                }
            }
        }
    }

    // 导入航线弹窗
    Dialog {
        id: importDialog
        modal: true
        title: "选择航线"
        standardButtons: Dialog.Cancel
        width: 320
        height: 320

        contentItem: ListView {
            model: routeListModel
            clip: true
            delegate: Item {
                width: parent.width
                height: 44
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 8
                    Text {
                        text: fileName
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Button {
                        text: "加载"
                        onClicked: loadRouteFromFile(fileName)
                    }
                    Button {
                        text: "删除"
                        onClicked: deleteRouteFile(fileName)
                    }
                }
            }
        }
    }

    // 任务列表弹窗
    Dialog {
        id: taskDialog
        modal: true
        title: "任务列表"
        standardButtons: Dialog.Cancel
        width: 420
        height: 420

        contentItem: Flickable {
            id: taskFlick
            anchors.fill: parent
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

            contentWidth: Math.max(taskFlick.width, taskColumn.implicitWidth + 24)
            contentHeight: taskColumn.y + taskColumn.implicitHeight + 12

            Column {
                id: taskColumn
                x: 12
                y: 12
                width: taskFlick.width - 24
                spacing: 12

                Repeater {
                    model: taskListModel
                    delegate: Item {
                        width: taskColumn.width
                        height: 76
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 12
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4
                                Text {
                                    text: name.length ? name : fileName
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                    font.bold: true
                                    color: "#222"
                                }
                                Text {
                                    text: (location.length ? location : "") + " " + (time.length ? time : "")
                                    color: "#666"
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }
                            Button {
                                text: "加载"
                                enabled: fileName !== "（暂无任务）"
                                Layout.alignment: Qt.AlignVCenter
                                onClicked: loadTaskFromFile(fileName)
                            }
                            Button {
                                text: "删除"
                                enabled: fileName !== "（暂无任务）"
                                Layout.alignment: Qt.AlignVCenter
                                onClicked: deleteTaskFile(fileName)
                            }
                        }
                    }
                }
            }
        }
    }

    FileDialog {
        id: exportTaskDialog
        title: "导出任务"
        fileMode: FileDialog.SaveFile
        nameFilters: [ "Excel 文件 (*.xls)" ]
        onAccepted: routePlanner.exportTask(exportTaskDialog.selectedFile)
    }

    Popup {
        id: planConfirmDialog
        property string message: ""
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn: parent
        width: 320
        background: Rectangle {
            radius: 6
            color: "white"
            border.color: "#cccccc"
        }
        contentItem: Column {
            spacing: 12
            padding: 16
            Label {
                text: planConfirmDialog.message
                wrapMode: Text.Wrap
                color: "#333"
            }
            Row {
                spacing: 12
                Button {
                    text: "取消"
                    onClicked: planConfirmDialog.close()
                }
                Button {
                    text: "确认"
                    highlighted: true
                    onClicked: {
                        planConfirmDialog.close()
                        routePlanner.planRoute()
                    }
                }
            }
        }
    }

    ListModel { id: routeListModel }
    ListModel { id: taskListModel }

    QtObject {
        id: boatStatus
        property real lon: 110.3311
        property real lat: 20.0659
        property real heading: 0
        property real speed: 0
        property string statusText: "待命"
        property double _lastUpdateMs: 0
        property real _lastLon: 110.3311
        property real _lastLat: 20.0659

        function update(lon, lat, heading) {
            const now = Date.now()
            const dt = (now - _lastUpdateMs) / 1000.0
            if (_lastUpdateMs > 0 && dt > 0) {
                const meters = haversine(_lastLat, _lastLon, lat, lon)
                speed = meters / dt
            }
            _lastUpdateMs = now
            _lastLon = lon
            _lastLat = lat
            boatStatus.lon = lon
            boatStatus.lat = lat
            boatStatus.heading = heading
        }
        function haversine(lat1, lon1, lat2, lon2) {
            const R = 6371000.0
            const dLat = (lat2 - lat1) * Math.PI / 180.0
            const dLon = (lon2 - lon1) * Math.PI / 180.0
            const a = Math.sin(dLat/2) * Math.sin(dLat/2) +
                      Math.cos(lat1 * Math.PI/180.0) * Math.cos(lat2 * Math.PI/180.0) *
                      Math.sin(dLon/2) * Math.sin(dLon/2)
            const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a))
            return R * c
        }
    }

    QtObject {
        id: autoReturnController
        function returnToTaskEnd() {
            if (!root.pageReady) {
                console.log("页面尚未加载，无法返航");
                return;
            }
            webmap.runJavaScript("if (typeof startAutoReturnToRouteEnd === 'function') { startAutoReturnToRouteEnd(); } else { alert('地图未准备好，无法返航'); }")
        }
        function startCustomMark() {
            if (!root.pageReady) {
                console.log("页面尚未加载，无法标记返航点");
                return;
            }
            webmap.runJavaScript("if (typeof startAutoReturnMarking === 'function') { startAutoReturnMarking(); } else { alert('地图未准备好，无法标记返航点'); }")
        }
    }

    QtObject {
        id: routePlanner
        property var points: []
        property var route: []
        property string statusText: "点击“进入航线规划”，在地图上选 4 个点框出区域。"
        property string pointsText: "已选点: 无"
        property int selectedCount: 0
        property string taskName: ""
        property string taskLocation: "未计算"
        property string taskTime: ""
        property var samplingPoints: []
        property string samplingText: "采样点: 未选择"
        property var samplingSettings: ({ bottlesPerPoint: 1, volumePerBottle: 0.5, depthMeters: 0.5 })
        property string samplingSettingsText: ""
        Component.onCompleted: updateSamplingSettingsText()

        function reset() {
            points = []
            route = []
            pointsText = "已选点: 无"
            selectedCount = 0
            taskLocation = "未计算"
            taskTime = ""
            samplingPoints = []
            samplingText = "采样点: 未选择"
            applySamplingSettings(defaultSamplingSettings())
        }

        function defaultSamplingSettings() {
            return { bottlesPerPoint: 1, volumePerBottle: 0.5, depthMeters: 0.5 }
        }

        // 接收地图当前航线（包括临时手动规划的航线）
        function updateRouteFromMap(pointsJson) {
            var arr = []
            try { arr = JSON.parse(pointsJson || "[]") } catch(e) { arr = [] }
            route = arr.map(function(p){ return { lon: p.lng, lat: p.lat } })
            updateTaskMeta()
        }

        function updateSamplingSettingsText() {
            const cfg = samplingSettings || defaultSamplingSettings()
            const bottles = isFinite(cfg.bottlesPerPoint) ? Math.max(1, Math.round(cfg.bottlesPerPoint)) : 1
            const volume = isFinite(cfg.volumePerBottle) ? cfg.volumePerBottle : 0.0
            const depth = isFinite(cfg.depthMeters) ? cfg.depthMeters : 0.0
            samplingSettingsText = "统一采样设置: 每点 " + bottles + " 瓶 | 每瓶 " + volume.toFixed(2) + " L | 深度 " + depth.toFixed(2) + " m"
        }

        function applySamplingSettings(cfg) {
            const next = defaultSamplingSettings()
            if (cfg) {
                if (isFinite(cfg.bottlesPerPoint))
                    next.bottlesPerPoint = Math.max(1, Math.round(cfg.bottlesPerPoint))
                if (isFinite(cfg.volumePerBottle))
                    next.volumePerBottle = Math.max(0, cfg.volumePerBottle)
                if (isFinite(cfg.depthMeters))
                    next.depthMeters = Math.max(0, cfg.depthMeters)
            }
            samplingSettings = next
            updateSamplingSettingsText()
            if (root.pageReady) {
                const js = `if (typeof updateSamplingSimConfig === "function") { updateSamplingSimConfig(${JSON.stringify(next)}); }`
                webmap.runJavaScript(js)
            }
        }

        function startPlanning() {
            reset()
            statusText = "请在地图上依次选择 4 个点框出区域。"
            webmap.runJavaScript(`
                if (window.startRouteSelection) {
                    window.startRouteSelection();
                } else {
                    console.log("TODO: 在 map_gaode.html 实现 startRouteSelection，点击地图时调用 RoutePlanner.addPoint(lon, lat)");
                }
            `)
        }

        function receivePoint(lon, lat) {
            if (points.length >= 4)
                return
            points.push({ lon: lon, lat: lat })
            selectedCount = points.length
            pointsText = "已选点: " + points.map(function(p, i){ return (i+1) + ") " + p.lon.toFixed(4) + "," + p.lat.toFixed(4); }).join("  ")
            statusText = "已选 " + points.length + "/4 点"
            if (points.length === 4) {
                statusText = "已选 4 点，请设置点位数并点击“规划航线”。"
            }
        }

        function planRoute() {
            if (points.length !== 4) {
                statusText = "需要先选择 4 个点。"
                return
            }
            const innerCount = Math.max(1, Math.round(waypointCountSpin.value))
            statusText = "正在按照输入的 " + innerCount + " 个点位数规划航线..."

            clearSamplingPoints(false)
            route = Planner.RouteAutoPlanner.computeRoute(points, innerCount)
            if (!route.length) {
                statusText = "航线规划失败，请重试。"
                return
            }
            var startPoint = route[0]
            var endPoint = route[route.length - 1]

            statusText = "规划完成：起点→" + (route.length - 2) + " 个点→终点，共 " + route.length + " 点。"
            pointsText = "起点: " + startPoint.lon.toFixed(4) + "," + startPoint.lat.toFixed(4)
                    + " | 终点: " + endPoint.lon.toFixed(4) + "," + endPoint.lat.toFixed(4)
            updateTaskMeta()

            const js = `
                if (window.showPlannedRoute) {
                    window.showPlannedRoute(${JSON.stringify(route)});
                } else {
                    console.log("TODO: 在 map_gaode.html 实现 showPlannedRoute(routePoints)");
                }
            `
            webmap.runJavaScript(js)
        }
        function requestPlan() {
            if (points.length !== 4) {
                statusText = "需要先选择 4 个点。"
                return
            }
            planConfirmDialog.message = "点位数：" + waypointCountSpin.value + "，确认按当前区域自动规划航线？"
            planConfirmDialog.open()
        }

        function savePlannedRoute() {
            if (!route.length) {
                statusText = "先规划航线再保存。"
                return
            }
            const payload = route.map(function(p){ return { lng: p.lon, lat: p.lat } })
            const ok = RouteSaver.saveRoute(JSON.stringify(payload))
            statusText = ok ? "航线已保存到 routes 目录。" : "保存航线失败。"
            if (ok)
                refreshRoutes()
        }

        function updateTaskMeta() {
            if (!route || !route.length) {
                taskLocation = "未计算"
                taskTime = ""
                return
            }
            var sumLon = 0
            var sumLat = 0
            for (var i = 0; i < route.length; ++i) {
                sumLon += route[i].lon
                sumLat += route[i].lat
            }
            var cLon = sumLon / route.length
            var cLat = sumLat / route.length
            taskLocation = cLon.toFixed(5) + "," + cLat.toFixed(5)
            taskTime = new Date().toISOString()
        }

        function updateSamplingPointsFromMap(pointsJson) {
            var arr = []
            try { arr = JSON.parse(pointsJson || "[]") } catch (e) { arr = [] }
            if (EnvironmentDataProvider && EnvironmentDataProvider.normalize) {
                arr = arr.map(function(p) {
                    if (!p || typeof p !== "object")
                        return p
                    p.env = EnvironmentDataProvider.normalize(p.env || {})
                    return p
                })
            }
            samplingPoints = arr
            samplingText = arr.length
                    ? "采样点: " + arr.map(function(p){ return "#" + (p.order || 0) + " → 航点" + (p.pointIndex || "?"); }).join("，")
                    : "采样点: 未选择"
            if (root.pageReady)
                pushSamplingToMap()
        }

        function clearSamplingPoints(notifyMap) {
            samplingPoints = []
            samplingText = "采样点: 未选择"
            if (notifyMap !== false) {
                webmap.runJavaScript("if (typeof clearSamplingSelection === 'function') { clearSamplingSelection(); }")
            }
        }

        function startSamplingSelection() {
            if (!route || route.length === 0) {
                statusText = "请先规划或加载航线，再选择采样点。"
                return
            }
            const js = `
                if (typeof startSamplingSelection === "function") {
                    startSamplingSelection();
                } else {
                    alert("地图未准备好，无法选择采样点");
                }
            `;
            webmap.runJavaScript(js)
        }

        function pushSamplingToMap() {
            const payload = JSON.stringify(samplingPoints || [])
            const js = `
                if (typeof applySamplingPoints === "function") {
                    applySamplingPoints(${payload}, { skipNotify: true });
                    if (typeof updateSamplingVisual === "function") { updateSamplingVisual(); }
                }
            `;
            webmap.runJavaScript(js)
        }

        function saveTask(name) {
            if (!route.length) {
                statusText = "先规划航线再保存任务。"
                return
            }
            var trimmed = (name || "").trim()
            if (!trimmed.length) {
                statusText = "请输入任务名称。"
                return
            }
            updateTaskMeta()
            applySamplingSettings(samplingSettings) // 确保保存前使用规范化的数值
            const payload = {
                name: trimmed,
                location: taskLocation,
                time: taskTime,
                points: route.map(function(p){ return { lng: p.lon, lat: p.lat } }),
                samplingPoints: samplingPoints || [],
                samplingSettings: samplingSettings || defaultSamplingSettings()
            }
            const ok = RouteSaver.saveTask(trimmed, JSON.stringify(payload))
            statusText = ok ? "任务已保存到 tasks 目录。" : "保存任务失败。"
            if (ok) {
                refreshTasks()
            }
        }

        function exportTask(fileUrl) {
            if (!route.length) {
                statusText = "先规划航线再导出任务。"
                return
            }
            var trimmed = (taskNameInput.text || "").trim()
            if (!trimmed.length) {
                statusText = "请输入任务名称。"
                return
            }
            updateTaskMeta()
            applySamplingSettings(samplingSettings)
            const payload = {
                name: trimmed,
                location: taskLocation,
                time: taskTime,
                points: route.map(function(p){ return { lng: p.lon, lat: p.lat } }),
                samplingPoints: samplingPoints || [],
                samplingSettings: samplingSettings || defaultSamplingSettings()
            }
            const targetPath = fileUrl && fileUrl.toString ? fileUrl.toString() : String(fileUrl || "")
            const ok = RouteSaver.exportTaskToExcel(JSON.stringify(payload), targetPath)
            statusText = ok ? "任务已导出。" : "任务导出失败。"
        }
    }

    function refreshRoutes() {
        routeListModel.clear()
        const files = RouteSaver.listRoutes()
        for (let i = 0; i < files.length; ++i) {
            routeListModel.append({ fileName: files[i] })
        }
        if (files.length === 0) {
            routeListModel.append({ fileName: "（暂无航线）" })
        }
    }

    function loadRouteFromFile(fileName) {
        if (!fileName || fileName === "（暂无航线）")
            return
        const jsonStr = RouteSaver.loadRoute(fileName)
        if (!jsonStr) {
            console.log("加载航线失败: " + fileName)
            return
        }
        try {
            const obj = JSON.parse(jsonStr)
            const pts = obj.points || obj
            if (pts && pts.length) {
                routePlanner.route = pts.map(function(p){ return { lon: p.lng, lat: p.lat } })
                routePlanner.updateTaskMeta()
                routePlanner.clearSamplingPoints(false)
            }
        } catch(e) {
            console.log("航线 JSON 解析失败: " + e)
        }
        const js = `showRouteFromNative(${JSON.stringify(jsonStr)});`
        webmap.runJavaScript(js)
        importDialog.close()
    }

    function deleteRouteFile(fileName) {
        if (!fileName || fileName === "（暂无航线）")
            return
        const ok = RouteSaver.deleteRoute(fileName)
        if (!ok) {
            console.log("删除航线失败: " + fileName)
            return
        }
        refreshRoutes()
    }

    function refreshTasks() {
        taskListModel.clear()
        const files = RouteSaver.listTasks()
        if (!files.length) {
            taskListModel.append({ fileName: "（暂无任务）", name: "", location: "", time: "" })
            return
        }
        for (let i = 0; i < files.length; ++i) {
            const f = files[i]
            const jsonStr = RouteSaver.loadTask(f)
            if (!jsonStr) {
                taskListModel.append({ fileName: f, name: f, location: "", time: "" })
                continue
            }
            try {
                const obj = JSON.parse(jsonStr)
                taskListModel.append({
                    fileName: f,
                    name: obj.name || f,
                    location: obj.location || "",
                    time: obj.time || "",
                    pointCount: obj.points ? obj.points.length : 0
                })
            } catch (e) {
                taskListModel.append({ fileName: f, name: f, location: "", time: "" })
            }
        }
    }

    function loadTaskFromFile(fileName) {
        if (!fileName || fileName === "（暂无任务）")
            return
        const jsonStr = RouteSaver.loadTask(fileName)
        if (!jsonStr) {
            console.log("加载任务失败: " + fileName)
            return
        }
        let obj = {}
        try {
            obj = JSON.parse(jsonStr)
        } catch (e) {
            console.log("任务 JSON 解析失败: " + e)
            return
        }
        const pts = obj.points || []
        if (!pts.length) {
            console.log("任务无航线点: " + fileName)
            return
        }
        routePlanner.route = pts.map(function(p){ return { lon: p.lng, lat: p.lat } })
        routePlanner.updateTaskMeta()
        routePlanner.applySamplingSettings(obj.sampling_settings || obj.samplingSettings)
        routePlanner.samplingPoints = obj.sampling_points || obj.samplingPoints || []
        routePlanner.updateSamplingPointsFromMap(JSON.stringify(routePlanner.samplingPoints))
        taskNameInput.text = obj.name || ""

        const js = `showRouteFromNative(${JSON.stringify(jsonStr)});`
        webmap.runJavaScript(js)
        taskDialog.close()
        routePlanner.statusText = "任务已加载：" + (obj.name || fileName)
    }

    function deleteTaskFile(fileName) {
        if (!fileName || fileName === "（暂无任务）")
            return
        const ok = RouteSaver.deleteTask(fileName)
        if (!ok) {
            console.log("删除任务失败: " + fileName)
            return
        }
        refreshTasks()
    }

}
