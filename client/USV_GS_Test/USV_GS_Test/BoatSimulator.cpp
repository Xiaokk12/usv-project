#include "BoatSimulator.h"

#include <QtMath>

#include "BoatLocationProvider.h"

BoatSimulator::BoatSimulator(BoatLocationProvider *provider, QObject *parent)
    : QObject(parent)
    , m_provider(provider)
{
    m_timer.setInterval(1000); // 1s 推送一次（如在前端模拟运行，可调整为 stop）
    connect(&m_timer, &QTimer::timeout, this, &BoatSimulator::tick);
}

void BoatSimulator::startCircle(double centerLon, double centerLat, double radiusMeters, double speedMetersPerSec)
{
    m_centerLon = centerLon;
    m_centerLat = centerLat;
    m_radiusMeters = qMax(1.0, radiusMeters);
    m_speedMetersPerSec = qMax(0.1, speedMetersPerSec);
    m_angleRad = 0.0;
    m_timer.start();
}

void BoatSimulator::stop()
{
    m_timer.stop();
} 

void BoatSimulator::tick()
{
    if (!m_provider)
        return;

    // 粗略的经纬度到米换算
    const double metersPerDegLat = 111000.0;
    const double metersPerDegLon = 111000.0 * qCos(qDegreesToRadians(m_centerLat));

    const double omega = m_speedMetersPerSec / m_radiusMeters; // rad/s
    m_angleRad += omega * (m_timer.interval() / 1000.0);

    const double dx = m_radiusMeters * qCos(m_angleRad);
    const double dy = m_radiusMeters * qSin(m_angleRad);

    const double lon = m_centerLon + dx / metersPerDegLon;
    const double lat = m_centerLat + dy / metersPerDegLat;
    const double headingDeg = qRadiansToDegrees(qAtan2(dy, dx));

    m_provider->emitLocation(lon, lat, headingDeg);
}
