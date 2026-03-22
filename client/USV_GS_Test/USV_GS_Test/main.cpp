#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFile>

// ⭐ 必须包含 WebEngine 初始化（不然必崩）
#include <QtWebEngineQuick/QtWebEngineQuick>

#include "RouteSaver.h"
#include "BoatLocationProvider.h"
#include "SamplingTaskBridge.h"
#include "BoatSimulator.h"
#include "EnvironmentDataProvider.h"

int main(int argc, char *argv[])
{
    // 允许 WebEngine 共享 OpenGL 上下文，避免部分平台崩溃
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QGuiApplication app(argc, argv);

    // ⭐ 必须初始化 Qt WebEngine，否则程序会直接崩溃！
    QtWebEngineQuick::initialize();

    QQmlApplicationEngine engine;

    // ⭐ 暴露 RouteSaver 对象到 QML / HTML
    RouteSaver routeSaver;
    routeSaver.setObjectName("RouteSaver"); // WebChannel 通过 objectName 暴露给 JS
    engine.rootContext()->setContextProperty("RouteSaver", &routeSaver);
    // ⭐ 暴露无人船定位提供者
    BoatLocationProvider boatLocation;
    boatLocation.setObjectName("BoatLocation");
    engine.rootContext()->setContextProperty("BoatLocation", &boatLocation);
    // ⭐ 模拟无人船位置，用于前端联调
    BoatSimulator simulator(&boatLocation);
    engine.rootContext()->setContextProperty("BoatSimulator", &simulator);
    // ⭐ 采样任务桥接（预留后端接口）
    SamplingTaskBridge samplingBridge;
    samplingBridge.setObjectName("SamplingTaskBridge");
    engine.rootContext()->setContextProperty("SamplingTaskBridge", &samplingBridge);
    // ⭐ 环境参数（随机模拟/后续后端接入）
    EnvironmentDataProvider envProvider;
    envProvider.setObjectName("EnvironmentDataProvider");
    engine.rootContext()->setContextProperty("EnvironmentDataProvider", &envProvider);

    // ⭐ 防止加载失败导致白屏
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() {
            qCritical("ERROR: QML Load failed, exiting!");
            QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    // ⭐ 使用 QML Module 的方式加载 Main（匹配 CMake 里的 URI USV_GS_Test）
    if (!QFile(":/map_gaode.html").exists()) {
        qCritical("Resource missing: qrc:/map_gaode.html not found");
    }
    engine.loadFromModule("USV_GS_Test", "Main");

    return app.exec();
}
