#ifndef BOATLOCATIONPROVIDER_H
#define BOATLOCATIONPROVIDER_H

#include <QObject>
#include <QtGlobal>

// 负责把无人船的 GPS 定位推送到 QML/HTML，后端可直接调用 emitLocation / updateStatus。
class BoatLocationProvider : public QObject
{
    Q_OBJECT
public:
    explicit BoatLocationProvider(QObject *parent = nullptr);

    // 供后端或 QML 调用：推送一次定位（默认 heading 为 NaN 表示未知）
    Q_INVOKABLE void emitLocation(double lon, double lat, double heading = qQNaN());

    // 更新状态文本（例如“串口已连接/断开”），前端可订阅显示
    Q_INVOKABLE void updateStatus(const QString &status);

    // 前端请求一次定位（例如按钮触发），后端可连接该信号后立即回复 emitLocation
    Q_INVOKABLE void requestLocation();

signals:
    void locationUpdated(double lon, double lat, double heading);
    void statusChanged(const QString &status);
    void locationRequested();
};

#endif // BOATLOCATIONPROVIDER_H
