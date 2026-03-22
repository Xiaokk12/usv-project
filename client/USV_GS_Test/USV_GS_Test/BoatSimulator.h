#ifndef BOATSIMULATOR_H
#define BOATSIMULATOR_H

#include <QObject>
#include <QTimer>

class BoatLocationProvider;

// 简单的模拟器：按圆形轨迹周期性推送位置，用于前端联调
class BoatSimulator : public QObject
{
    Q_OBJECT
public:
    explicit BoatSimulator(BoatLocationProvider *provider, QObject *parent = nullptr);

    // 开始绕指定中心点、半径的圆形运动
    Q_INVOKABLE void startCircle(double centerLon, double centerLat, double radiusMeters, double speedMetersPerSec);
    // 停止模拟
    Q_INVOKABLE void stop();

private slots:
    void tick();

private:
    BoatLocationProvider *m_provider;
    QTimer m_timer;
    double m_centerLon = 110.3311;
    double m_centerLat = 20.0659;
    double m_radiusMeters = 50.0;
    double m_speedMetersPerSec = 2.0;
    double m_angleRad = 0.0;
};

#endif // BOATSIMULATOR_H
