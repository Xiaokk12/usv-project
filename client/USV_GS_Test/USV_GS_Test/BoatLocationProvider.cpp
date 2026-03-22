#include "BoatLocationProvider.h"

BoatLocationProvider::BoatLocationProvider(QObject *parent)
    : QObject(parent)
{
}

void BoatLocationProvider::emitLocation(double lon, double lat, double heading)
{
    emit locationUpdated(lon, lat, heading);
}

void BoatLocationProvider::updateStatus(const QString &status)
{
    emit statusChanged(status);
}

void BoatLocationProvider::requestLocation()
{
    emit locationRequested();
}
